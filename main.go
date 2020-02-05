package main

import (
	"database/sql"
	"encoding/json"
	"github.com/pkg/errors"
	"io/ioutil"
	"log"
	"net/http"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

type BenchmarkResult struct {
	BenchmarkType string

	MachineId       string
	BenchmarkResult float64

	Board      string
	CpuName    string
	CpuDesc    string
	CpuConfig  string
	NumCpus    int
	NumCores   int
	NumThreads int

	MemoryInKiB         int
	PhysicalMemoryInMiB int
	MemoryTypes         string

	OpenGlRenderer string
	GpuDesc        string

	MachineDataVersion int

	PointerBits       int
	DataFromSuperUser bool
}

func handlePost(database *sql.DB, w http.ResponseWriter, req *http.Request) {
	if req.URL.Path != "/benchmark.json" {
		http.Error(w, "Can't POST to "+req.URL.Path, http.StatusBadRequest)
		return
	}

	body, err := ioutil.ReadAll(req.Body)
	req.Body.Close()
	if err != nil {
		http.Error(w, "Error while reading body: "+err.Error(), http.StatusBadRequest)
		return
	}

	var bench BenchmarkResult
	if err = json.Unmarshal(body, &bench); err != nil {
		http.Error(w, "Error while parsing JSON: "+err.Error(), http.StatusBadRequest)
		return
	}

	if bench.BenchmarkType == "" {
		http.Error(w, "BenchmarkType not provided", http.StatusBadRequest)
		return
	}

	if bench.PointerBits != 32 && bench.PointerBits != 64 {
		http.Error(w, "Unknown PointerBits value", http.StatusBadRequest)
		return
	}

	if bench.NumCpus < 1 || bench.NumCores < 1 || bench.NumThreads < 1 {
		http.Error(w, "Number of CPUs, cores, or threads is invalid", http.StatusBadRequest)
		return
	}

	stmt, err := database.Prepare(`INSERT INTO benchmark_result (benchmark_type,
		benchmark_result, machine_id, board, cpu_name, cpu_desc, cpu_config,
		num_cpus, num_cores, num_threads, memory_in_kib, physical_memory_in_mib,
		memory_types, opengl_renderer, gpu_desc, machine_data_version, pointer_bits,
		data_from_super_user, timestamp)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, strftime('%s', 'now'))`)
	if err != nil {
		http.Error(w, "Couldn't prepare statement: " + err.Error(), http.StatusInternalServerError)
		return
	}

	_, err = stmt.Exec(
		bench.BenchmarkType,
		bench.BenchmarkResult,
		bench.MachineId,
		bench.Board,
		bench.CpuName,
		bench.CpuDesc,
		bench.CpuConfig,
		bench.NumCpus,
		bench.NumCores,
		bench.NumThreads,
		bench.MemoryInKiB,
		bench.PhysicalMemoryInMiB,
		bench.MemoryTypes,
		bench.OpenGlRenderer,
		bench.GpuDesc,
		bench.MachineDataVersion,
		bench.PointerBits,
		bench.DataFromSuperUser)
	if err != nil {
		http.Error(w, "Could not publish benchmark result: " + err.Error(), http.StatusInternalServerError)
		return
	}

	w.Write([]byte("Benchmark " + bench.BenchmarkType + " updated"))
}

func handleGet(database *sql.DB, updateCacheRequest chan string, w http.ResponseWriter, req *http.Request) {
	stmt, err := database.Prepare("SELECT blob, timestamp FROM cached_blobs WHERE name=?")
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	defer stmt.Close()

	var blob string
	var timeStamp int64
	err = stmt.QueryRow(req.URL.Path).Scan(&blob, &timeStamp)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	if time.Now().Sub(time.Unix(timeStamp, 0)) > 12*time.Hour {
		updateCacheRequest <- req.URL.Path
	}

	w.Write([]byte(blob))
}

func fetch(url string) ([]byte, error) {
	log.Printf("Fetching %s", url)

	resp, err := http.Get(url)
	if err != nil {
		log.Printf("Error while fetching %s: %v", url, err)
		return nil, err
	}

	if resp.StatusCode != http.StatusOK {
		log.Printf("Got non-OK response for %q (%d)", url, resp.StatusCode)
		return nil, errors.New("Non-200 response")
	}

	body, err := ioutil.ReadAll(resp.Body)
	resp.Body.Close()
	if err != nil {
		log.Printf("Error while reading %s body: %v", url, err)
		return nil, err
	}

	log.Printf("Got %d bytes of response", len(body))
	return body, nil
}

func updateCache(database *sql.DB, URL, contents string) error {
	stmt, err := database.Prepare("INSERT OR REPLACE INTO cached_blobs (name, blob, timestamp) VALUES (?, ?, strftime('%s', 'now'))")
	if err != nil {
		return err
	}
	defer stmt.Close()

	_, err = stmt.Exec(URL, contents)
	return err
}

func fetchUrlIntoCache(database *sql.DB, URL string) error {
	blob, err := fetch("https://raw.githubusercontent.com/lpereira/hardinfo/master/data/" + URL)
	if err != nil {
		return err
	}

	return updateCache(database, URL, string(blob))
}

func updateBenchmarkJsonCache(database *sql.DB, benchmarkTypes []string) error {
	resultMap := make(map[string][]BenchmarkResult)

	stmt, err := database.Prepare("SELECT machine_id, benchmark_result, board, cpu_name, cpu_desc, cpu_config, num_cpus, num_cores, num_threads, memory_in_kib, physical_memory_in_mib, memory_types, opengl_renderer, gpu_desc, machine_data_Version, pointer_bits, data_from_super_user FROM benchmark_result WHERE benchmark_type=? ORDER BY RANDOM() LIMIT 100")
	if err != nil {
		return err
	}

	defer stmt.Close()

	for _, benchType := range benchmarkTypes {
		rows, err := stmt.Query(benchType)
		if err != nil {
			continue
		}

		for rows.Next() {
			var result BenchmarkResult

			err = rows.Scan(
				&result.MachineId,
				&result.BenchmarkResult,
				&result.Board,
				&result.CpuName,
				&result.CpuDesc,
				&result.CpuConfig,
				&result.NumCpus,
				&result.NumCores,
				&result.NumThreads,
				&result.MemoryInKiB,
				&result.PhysicalMemoryInMiB,
				&result.MemoryTypes,
				&result.OpenGlRenderer,
				&result.GpuDesc,
				&result.MachineDataVersion,
				&result.PointerBits,
				&result.DataFromSuperUser)
			if err == nil {
				resultMap[benchType] = append(resultMap[benchType], result)
			} else {
				log.Print(err)
			}
		}

		rows.Close()
	}

	marshaled, err := json.Marshal(resultMap)
	if err == nil {
		return updateCache(database, "/benchmark.json", string(marshaled))
	}

	return err
}

func main() {
	var benchmarkTypes []string
	var database *sql.DB
	var err error

	if database, err = sql.Open("sqlite3", "./hardinfo-database.db"); err != nil {
		log.Fatal(err)
	}
	defer database.Close()
	if err := database.Ping(); err != nil {
		log.Fatal(err)
	}

	go func() {
		for {
			if _, err := database.Exec("VACUUM"); err != nil {
				log.Printf("Vacuuming database failed, will try again in an hour")
				time.Sleep(1 * time.Hour)
			} else {
				log.Printf("Vacuuming database succeeded, will try again tomorrow")
				time.Sleep(1 * time.Hour * 24)
			}
		}
	}()

	// TODO: spawn goroutine to average values for the same machines every day
	// TODO: spawn goroutine to create a cached table with random items

	// TODO: this will have to either go, or query the cached table
	types, err := database.Query("SELECT benchmark_type FROM benchmark_result GROUP BY benchmark_type")
	if err != nil {
		log.Fatal(err)
	}
	for types.Next() {
		var benchType string
		err = types.Scan(&benchType)
		if err != nil {
			log.Fatal(err)
		}
		benchmarkTypes = append(benchmarkTypes, benchType)
		log.Printf("Found benchmark type: %s", benchType)
	}
	types.Close()

	updateCacheRequest := make(chan string)
	go func() {
		for {
			select {
			case URL := <-updateCacheRequest:
				if URL == "/benchmark.json" {
					err = updateBenchmarkJsonCache(database, benchmarkTypes)
				} else {
					err = fetchUrlIntoCache(database, URL)
				}

				if err == nil {
					log.Printf("Cache updated for %q", URL)
				} else {
					log.Printf("Error while updating cache: %q", err)
				}
			}
		}
	}()

	http.HandleFunc("/", func(w http.ResponseWriter, req *http.Request) {
		switch req.Method {
		case "POST":
			handlePost(database, w, req)
		case "GET":
			handleGet(database, updateCacheRequest, w, req)
		default:
			http.Error(w, "Invalid method: "+req.Method, http.StatusMethodNotAllowed)
		}
	})

	log.Fatal(http.ListenAndServe(":1234", nil))
}
