package main

import (
	"compress/gzip"
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"github.com/pkg/errors"
	"io/ioutil"
	"log"
	"net/http"
	"regexp"
	"strings"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

type BenchmarkResult struct {
	MachineId string
	ExtraInfo string
	UserNote  string

	BenchmarkVersion   int
	MachineDataVersion int

	BenchmarkResult float64
	ElapsedTime     float64
	UsedThreads     int

	Board       string
	MachineType string
	CpuName     string
	CpuDesc     string
	CpuConfig   string
	NumCpus     int
	NumCores    int
	NumThreads  int

	MemoryInKiB         int
	PhysicalMemoryInMiB int
	MemoryTypes         string

	OpenGlRenderer string
	GpuDesc        string

	PointerBits int

	DataFromSuperUser bool

	Legacy bool
}

func handlePost(database *sql.DB, w http.ResponseWriter, req *http.Request) (int, error) {
	if req.URL.Path != "/benchmark.json" {
		return http.StatusForbidden, fmt.Errorf("Can't POST to " + req.URL.Path)
	}

	body, err := ioutil.ReadAll(req.Body)
	req.Body.Close()
	if err != nil {
		return http.StatusBadRequest, fmt.Errorf("Error while reading body: " + err.Error())
	}

	benches := make(map[string]BenchmarkResult)
	if err = json.Unmarshal(body, &benches); err != nil {
		return http.StatusBadRequest, fmt.Errorf("Error while parsing JSON: " + err.Error())
	}

	tx, err := database.Begin()
	if err != nil {
		return http.StatusInternalServerError, fmt.Errorf("Could not create transaction")
	}

	stmt, err := tx.Prepare(`INSERT INTO benchmark_result (benchmark_type,
		benchmark_result, extra_info, machine_id, board, cpu_name, cpu_desc, cpu_config,
		num_cpus, num_cores, num_threads, memory_in_kib, physical_memory_in_mib,
		memory_types, opengl_renderer, gpu_desc, pointer_bits,
		data_from_super_user, used_threads, benchmark_version, user_note,
		elapsed_time, machine_data_version, legacy, machine_type, timestamp)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,
		strftime('%s', 'now'))`)
	if err != nil {
		return http.StatusInternalServerError, fmt.Errorf("Couldn't prepare statement: " + err.Error())
	}

	defer stmt.Close()

	for benchType, bench := range benches {
		if bench.MachineId == "" || !strings.Contains(bench.MachineId, ";") {
			return http.StatusBadRequest, fmt.Errorf("MachineId looks invalid")
		}

		if !bench.Legacy {
			if bench.PointerBits != 32 && bench.PointerBits != 64 {
				return http.StatusBadRequest, fmt.Errorf("Unknown PointerBits value")
			}

			if bench.NumCpus < 1 || bench.NumCores < 1 || bench.NumThreads < 1 {
				return http.StatusBadRequest, fmt.Errorf("Number of CPUs, cores, or threads is invalid")
			}

			if bench.MemoryInKiB < 4*1024 {
				return http.StatusBadRequest, fmt.Errorf("Total memory value is too low to be true")
			}

			if bench.PhysicalMemoryInMiB != 0 && bench.PhysicalMemoryInMiB < 4 {
				return http.StatusBadRequest, fmt.Errorf("Physical memory value is too low to be true")
			}
		}

		if bench.BenchmarkResult < 0 {
			return http.StatusBadRequest, fmt.Errorf("Benchmark results can't be negative")
		}

		_, err = stmt.Exec(
			benchType,
			bench.BenchmarkResult,
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
			bench.PointerBits,
			bench.DataFromSuperUser,
			bench.UsedThreads,
			bench.BenchmarkVersion,
			bench.UserNote,
			bench.ElapsedTime,
			bench.MachineDataVersion,
			bench.Legacy,
			bench.MachineType)
		if err != nil {
			return http.StatusInternalServerError, fmt.Errorf("Could not publish benchmark result: " + err.Error())
		}
	}

	if err := tx.Commit(); err != nil {
		return http.StatusInternalServerError, fmt.Errorf("Could not commit transaction: " + err.Error())
	}

	return http.StatusOK, nil
}

func handleGet(database *sql.DB, updateCacheRequest chan string, w http.ResponseWriter, req *http.Request) (int, error) {
	stmt, err := database.Prepare("SELECT blob, timestamp FROM cached_blobs WHERE name=?")
	if err != nil {
		return http.StatusInternalServerError, err
	}

	defer stmt.Close()

	var blob string
	var timeStamp int64
	err = stmt.QueryRow(req.URL.Path).Scan(&blob, &timeStamp)
	if err != nil {
		return http.StatusNotFound, fmt.Errorf("%s: file not found", req.URL.Path)
	}

	if time.Now().Sub(time.Unix(timeStamp, 0)) > 12*time.Hour {
		updateCacheRequest <- req.URL.Path
	}

	if !strings.Contains(req.Header.Get("Accept-Encoding"), "gzip") {
		w.Write([]byte(blob))
	} else {
		w.Header().Set("Content-Encoding", "gzip")
		gz := gzip.NewWriter(w)
		defer gz.Close()
		gz.Write([]byte(blob))
	}

	return http.StatusOK, nil
}

func fetch(url string) ([]byte, error) {
	log.Printf("Fetching %s", url)

	ctx, cancel := context.WithTimeout(context.Background(), 60*time.Second)
	defer cancel()

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, err
	}

	resp, err := http.DefaultClient.Do(req.WithContext(ctx))
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
	origUrl := URL

	switch URL {
	case "/pci.ids":
		URL = "https://pci-ids.ucw.cz/v2.2/pci.ids"
	case "/usb.ids":
		URL = "http://www.linux-usb.org/usb.ids"
	default:
		URL = "https://raw.githubusercontent.com/lpereira/hardinfo/master/data" + URL
	}

	blob, err := fetch(URL)
	if err != nil {
		return err
	}

	return updateCache(database, origUrl, string(blob))
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
			gpu_desc, pointer_bits, data_from_super_user,
			used_threads, benchmark_version, user_note, elapsed_time, machine_data_version,
			legacy, machine_type
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
				&result.PointerBits,
				&result.DataFromSuperUser,
				&result.UsedThreads,
				&result.BenchmarkVersion,
				&result.UserNote,
				&result.ElapsedTime,
				&result.MachineDataVersion,
				&result.Legacy,
				&result.MachineType)
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

func connectDB() (database *sql.DB, err error) {
	if database, err = sql.Open("sqlite3", "./hardinfo-database.db"); err != nil {
		return nil, err
	}

	if err := database.Ping(); err != nil {
		return nil, err
	}

	return database, nil
}

func main() {
	var database *sql.DB

	database, err := connectDB()
	if err != nil {
		log.Fatalf("Could not connect to database: %s", err)
	}

	defer database.Close()

	updateCacheRequest := make(chan string)

	go func() {
		onceEveryDay := time.NewTicker(time.Hour * 24)
		lastUpdate := make(map[string]time.Time)

		var err error

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
					lastUpdate[URL] = time.Now()
				} else {
					log.Printf("Error while updating URL %q: %q", URL, err)
				}
			}
		}
	}()

	http.HandleFunc("/", func(w http.ResponseWriter, req *http.Request) {
		if matched, err := regexp.MatchString(`\/[a-zA-Z0-9_-]+\.[a-z]+`, req.URL.Path); !matched || err != nil {
			http.Error(w, "Invalid file name: "+req.URL.Path, http.StatusBadRequest)
			return
		}

		var err error
		var responseCode int
		switch req.Method {
		case "POST":
			responseCode, err = handlePost(database, w, req)
		case "GET":
			responseCode, err = handleGet(database, updateCacheRequest, w, req)
		default:
			responseCode = http.StatusMethodNotAllowed
			err = fmt.Errorf("Invalid method: %q", req.Method)
		}

		if err != nil {
			log.Printf("Error: %q", err.Error())
			http.Error(w, err.Error(), responseCode)
		}
	})

	log.Fatal(http.ListenAndServe(":1234", nil))
}
