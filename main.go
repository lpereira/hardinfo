package main

import (
	"database/sql"
	"encoding/json"
	"github.com/pkg/errors"
	"io/ioutil"
	"log"
	"net/http"
	"regexp"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

type BenchmarkResult struct {
	MachineId          string
	BenchmarkResult    float64
	MachineDataVersion int
	ExtraInfo          string

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

	PointerBits       int
	DataFromSuperUser bool
}

type BenchmarkResultInput struct {
	BenchmarkResult
	BenchmarkType string
}

func handlePost(database *sql.DB, w http.ResponseWriter, req *http.Request) {
	if req.URL.Path != "/benchmark.json" {
		http.Error(w, "Can't POST to "+req.URL.Path, http.StatusForbidden)
		return
	}

	body, err := ioutil.ReadAll(req.Body)
	req.Body.Close()
	if err != nil {
		http.Error(w, "Error while reading body: "+err.Error(), http.StatusBadRequest)
		return
	}

	stmt, err := database.Prepare(`INSERT INTO benchmark_result (benchmark_type,
		benchmark_result, extra_info, machine_id, board, cpu_name, cpu_desc, cpu_config,
		num_cpus, num_cores, num_threads, memory_in_kib, physical_memory_in_mib,
		memory_types, opengl_renderer, gpu_desc, machine_data_version, pointer_bits,
		data_from_super_user, timestamp)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, strftime('%s', 'now'))`)
	if err != nil {
		http.Error(w, "Couldn't prepare statement: "+err.Error(), http.StatusInternalServerError)
		return
	}

	defer stmt.Close()

	var benches []BenchmarkResultInput
	if err = json.Unmarshal(body, &benches); err != nil {
		http.Error(w, "Error while parsing JSON: "+err.Error(), http.StatusBadRequest)
		return
	}

	for _, bench := range benches {
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

		if bench.MemoryInKiB < 4 * 1024 || bench.PhysicalMemoryInMiB < 4 {
			http.Error(w, "Total memory value is too low to be true", http.StatusBadRequest)
			return
		}

		if bench.BenchmarkResult.BenchmarkResult < 0 {
			http.Error(w, "Benchmark results can't be negative", http.StatusBadRequest)
			return
		}

		_, err = stmt.Exec(
			bench.BenchmarkType,
			bench.BenchmarkResult.BenchmarkResult,
			bench.ExtraInfo,
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
			http.Error(w, "Could not publish benchmark result: "+err.Error(), http.StatusInternalServerError)
			return
		}

		w.Write([]byte("Benchmark results for " + bench.BenchmarkType + " published\n"))
	}
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
		http.Error(w, req.URL.Path+": file not found", http.StatusNotFound)
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
		return nil, err
	}

	if resp.StatusCode != http.StatusOK {
		return nil, errors.New("Non-200 response")
	}

	body, err := ioutil.ReadAll(resp.Body)
	resp.Body.Close()
	if err != nil {
		return nil, err
	}

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

func updateBenchmarkJsonCache(database *sql.DB) error {
	var benchmarkTypes []string

	// TODO: fetch from the cached table (more below)
	types, err := database.Query(`SELECT DISTINCT benchmark_type FROM benchmark_result`)
	if err != nil {
		return err
	}
	for types.Next() {
		var benchType string
		err = types.Scan(&benchType)
		if err != nil {
			return err
		}
		benchmarkTypes = append(benchmarkTypes, benchType)
	}
	types.Close()

	resultMap := make(map[string][]BenchmarkResult)

	// TODO: spawn goroutine to average values for the same machines every day
	// TODO: spawn goroutine to create a cached table with random items,
	//       using select distinct to not serialize the same machine id twice
	stmt, err := database.Prepare(`
		SELECT extra_info, machine_id, AVG(benchmark_result) AS benchmark_result,
			board, cpu_name, cpu_desc, cpu_config, num_cpus, num_cores, num_threads,
			memory_in_kib, physical_memory_in_mib, memory_types, opengl_renderer,
			gpu_desc, machine_data_Version, pointer_bits, data_from_super_user
		FROM benchmark_result
		WHERE benchmark_type=?
		GROUP BY machine_id, pointer_bits
		ORDER BY RANDOM()
		LIMIT 50`)
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
				&result.ExtraInfo,
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
	var database *sql.DB
	var err error

	if database, err = sql.Open("sqlite3", "./hardinfo-database.db"); err != nil {
		log.Fatal(err)
	}
	defer database.Close()
	if err := database.Ping(); err != nil {
		log.Fatal(err)
	}

	updateCacheRequest := make(chan string)

	go func() {
		onceEveryDay := time.NewTicker(time.Hour * 24)
		lastUpdate := make(map[string]time.Time)

		for {
			select {
			case <-onceEveryDay.C:
				database.Exec("VACUUM")

			case URL := <-updateCacheRequest:
				if last, ok := lastUpdate[URL]; ok && time.Now().Sub(last) < time.Hour {
					// Avoids repeated requests to refresh the cache from actually doing so
					continue
				}

				if URL == "/benchmark.json" {
					err = updateBenchmarkJsonCache(database)
				} else {
					err = fetchUrlIntoCache(database, URL)
				}

				if err == nil {
					log.Printf("Cache updated for %q", URL)
					lastUpdate[URL] = time.Now()
				} else {
					log.Printf("Error while updating cache: %q", err)
				}
			}
		}
	}()

	http.HandleFunc("/", func(w http.ResponseWriter, req *http.Request) {
		if matched, err := regexp.MatchString(`\/[a-zA-Z0-9_-]+\.[a-z]+`, req.URL.Path); !matched || err != nil {
			http.Error(w, "Invalid file name: "+req.URL.Path, http.StatusBadRequest)
			return
		}

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
