#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p logs
pass=0
check(){ label="$1"; shift; if "$@"; then echo "PASS: $label"; pass=$((pass+1)); else echo "FAIL: $label" >&2; exit 1; fi; }
check 'FCFS scheduling' bash -c './campuscompute schedule fcfs data/jobs.csv | grep -q "Average: waiting=5.75"'
check 'Round Robin scheduling' bash -c './campuscompute schedule rr data/jobs.csv 2 | grep -q "Average: waiting=9.75"'
check 'Priority scheduling' bash -c './campuscompute schedule priority data/jobs.csv | grep -q "Average: waiting=5.25"'
check 'Compare output' bash -c './campuscompute compare data/jobs.csv | grep -q "Same 4 jobs"'
check 'Producer/consumer total' bash -c './campuscompute logger 6 | grep -q "produced=12 consumed=12"'
check 'Correct log lines' bash -c '[ "$(wc -l < logs/events.log)" -eq 12 ]'
check 'Shared-memory IPC exchange' bash -c './campuscompute ipc | grep -q "read parent.s reply"'
check 'IPC objects unlinked' bash -c './campuscompute ipc | grep -q "shm gone=yes, semaphores gone=yes"'
check 'No IPC left in /dev/shm' bash -c '! ls /dev/shm | grep -q campuscompute'
check 'Safe state' bash -c './campuscompute banker data/banker_safe.txt | grep -q "Result: SAFE"'
check 'Unsafe state' bash -c './campuscompute banker data/banker_unsafe.txt | grep -q "Result: UNSAFE"'
check 'Unsafe state (small)' bash -c './campuscompute banker data/banker_unsafe_small.txt | grep -q "Result: UNSAFE"'
check 'Worker exit' bash -c './campuscompute worker normal | grep -q "status 0"'
check 'Simulated failure detection' bash -c './campuscompute worker fail | grep -q "status 42"'
check 'Resource limit' bash -c './campuscompute limit | grep -q "File too large"'
check 'Invalid quantum rejects' bash -c '! ./campuscompute schedule rr data/jobs.csv 0 >/dev/null 2>&1'
check 'Invalid command rejects' bash -c '! ./campuscompute unknown >/dev/null 2>&1'
check 'Invalid job file rejects' bash -c '! ./campuscompute schedule fcfs data/jobs_invalid.csv >/dev/null 2>&1'
check 'Cleanup verification' bash -c './campuscompute cleanup | grep -q "Result: CLEAN"'
check 'Integrated demo' bash -c './campuscompute all | grep -q "ALL DEMONSTRATIONS COMPLETED"'
echo "ALL $pass TESTS PASSED"
