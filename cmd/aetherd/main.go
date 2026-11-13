// aetherd is the process supervisor and health endpoint for the native bot.
// It deliberately does not duplicate game logic or hold VK credentials.
package main

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"sync/atomic"
	"syscall"
	"time"
)

func main() {
	binary := flag.String("binary", "/usr/local/bin/vkaether", "path to the C++ bot executable")
	config := flag.String("config", "/app/config.json", "path to the bot configuration file")
	listen := flag.String("listen", ":8080", "health server listen address")
	flag.Parse()

	if _, err := os.Stat(*binary); err != nil {
		log.Fatalf("bot executable unavailable: %v", err)
	}
	if _, err := os.Stat(*config); err != nil {
		log.Fatalf("configuration unavailable: %v", err)
	}

	cmd := exec.Command(*binary, *config)
	cmd.Stdout, cmd.Stderr = os.Stdout, os.Stderr
	if err := cmd.Start(); err != nil {
		log.Fatalf("start bot: %v", err)
	}

	var alive atomic.Bool
	alive.Store(true)
	exited := make(chan error, 1)
	go func() {
		err := cmd.Wait()
		alive.Store(false)
		exited <- err
	}()

	mux := http.NewServeMux()
	health := func(w http.ResponseWriter, _ *http.Request) {
		if !alive.Load() {
			http.Error(w, "bot process is not running", http.StatusServiceUnavailable)
			return
		}
		w.Header().Set("Content-Type", "text/plain; charset=utf-8")
		w.WriteHeader(http.StatusOK)
		_, _ = fmt.Fprintln(w, "ok")
	}
	mux.HandleFunc("/healthz", health)
	mux.HandleFunc("/readyz", health)
	server := &http.Server{
		Addr: *listen, Handler: mux,
		ReadHeaderTimeout: 3 * time.Second,
	}

	serverErr := make(chan error, 1)
	go func() {
		err := server.ListenAndServe()
		if !errors.Is(err, http.ErrServerClosed) {
			serverErr <- err
		}
	}()

	signals := make(chan os.Signal, 1)
	signal.Notify(signals, syscall.SIGINT, syscall.SIGTERM)
	select {
	case sig := <-signals:
		log.Printf("received %s; stopping bot", sig)
	case err := <-exited:
		if err != nil {
			log.Printf("bot exited: %v", err)
		} else {
			log.Printf("bot exited normally")
		}
	case err := <-serverErr:
		log.Printf("health server failed: %v", err)
	}

	_ = server.Shutdown(context.Background())
	if cmd.ProcessState == nil {
		_ = cmd.Process.Signal(os.Interrupt)
		select {
		case <-exited:
		case <-time.After(10 * time.Second):
			_ = cmd.Process.Kill()
			<-exited
		}
	}
}
