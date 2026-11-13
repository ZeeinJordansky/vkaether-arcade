package main

import (
	"net/http/httptest"
	"sync/atomic"
	"testing"
)

func TestProcessHealthTracksChildLifetime(t *testing.T) {
	var alive atomic.Bool
	alive.Store(true)
	handler := processHealthHandler(&alive)

	response := httptest.NewRecorder()
	handler(response, httptest.NewRequest("GET", "/healthz", nil))
	if response.Code != 200 {
		t.Fatalf("healthy child: expected 200, got %d", response.Code)
	}

	alive.Store(false)
	response = httptest.NewRecorder()
	handler(response, httptest.NewRequest("GET", "/healthz", nil))
	if response.Code != 503 {
		t.Fatalf("exited child: expected 503, got %d", response.Code)
	}
}

func TestReadinessRequiresAliveAndReadyChild(t *testing.T) {
	var alive, ready atomic.Bool
	alive.Store(true)
	ready.Store(true)
	handler := readinessHandler(&alive, &ready)

	response := httptest.NewRecorder()
	handler(response, httptest.NewRequest("GET", "/readyz", nil))
	if response.Code != 200 {
		t.Fatalf("ready child: expected 200, got %d", response.Code)
	}

	ready.Store(false)
	response = httptest.NewRecorder()
	handler(response, httptest.NewRequest("GET", "/readyz", nil))
	if response.Code != 503 {
		t.Fatalf("shutting-down child: expected 503, got %d", response.Code)
	}

	alive.Store(false)
	ready.Store(true)
	response = httptest.NewRecorder()
	handler(response, httptest.NewRequest("GET", "/readyz", nil))
	if response.Code != 503 {
		t.Fatalf("dead child: expected 503, got %d", response.Code)
	}
}
