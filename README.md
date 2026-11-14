# CampusCompute — Operating Systems Level 3 Mini-Project

A Linux/WSL Ubuntu C implementation of the eight functional requirements: job lifecycle and validation, FCFS/RR/non-preemptive priority scheduling, a bounded POSIX-threads logger, POSIX shared-memory + named-semaphore IPC, Banker's safety test, fork/waitpid worker-failure detection, RLIMIT_FSIZE, and an independent command-line interface.


## 1. Install Ubuntu/WSL and required tools

On Windows PowerShell **as Administrator**, if you do not already have Ubuntu:

```powershell
wsl --install -d Ubuntu
```

Restart Windows if prompted, open the **Ubuntu** app, and complete the first-run Linux username/password setup. These steps are unnecessary if Ubuntu is already installed.

Inside the Ubuntu terminal:

```bash
sudo apt update
sudo apt install -y build-essential zip unzip python3 python3-reportlab
```

## Transfer and extract the project archive

Download `CampusCompute_Project.zip` to your Windows `Downloads` folder. In Ubuntu, replace `WINDOWS_USER` with your real Windows account folder name:

```bash
cd ~
cp /mnt/c/Users/WINDOWS_USER/Downloads/CampusCompute_Project.zip .
unzip CampusCompute_Project.zip
cd CampusCompute
```

Alternatively, open the ZIP using Windows Explorer and copy the `CampusCompute` folder into your WSL Linux home folder. Run all commands below **from inside `~/CampusCompute`**.



## Build and run the entire project

```bash
pwd
ls -la
make clean
make
./campuscompute help
make test
./campuscompute all
```

`make test` should report **ALL 20 TESTS PASSED**. `./campuscompute all` should conclude **ALL DEMONSTRATIONS COMPLETED**. Concurrency means the log event ordering can vary, but the totals are deterministic.

## 5. Individual commands

All commands below run from `~/CampusCompute`:

```bash
./campuscompute schedule fcfs data/jobs.csv
./campuscompute schedule rr data/jobs.csv 2
./campuscompute schedule priority data/jobs.csv
./campuscompute compare data/jobs.csv
./campuscompute logger 5
./campuscompute ipc
./campuscompute banker data/banker_safe.txt
./campuscompute banker data/banker_unsafe.txt
./campuscompute worker normal
./campuscompute worker fail
./campuscompute limit
./campuscompute cleanup
./campuscompute nonexistent
./campuscompute schedule rr data/jobs.csv 0
cat logs/events.log
```

FCFS runs ready jobs by arrival then ID. Priority chooses the smallest priority integer among ready jobs, then arrival/ID. RR uses FIFO ready queue and quantum 2 for the integrated demo; newly arrived jobs go before an expiring job's requeue. Each scheduler prints a Gantt-like timeline, each job's completion, turnaround (`completion-arrival`), waiting (`turnaround-burst`), response (`first_start-arrival`), averages and makespan. The logger has two producers, one consumer, a queue of 4 slots, mutex and condition variables; `logs/events.log` is intentionally retained as evidence. IPC uses POSIX `shm_open` + `ftruncate` + `mmap` and two **named** semaphores (`sem_open`): the child attaches by name, writes a request, the parent replies (two-way exchange). Names contain the PID (`/campuscompute_shm_<pid>` etc.), and everything is released with `munmap`, `sem_close`, `sem_unlink` and `shm_unlink` on every exit path; the program then verifies the `/dev/shm` entries are gone. The `cleanup` command performs real checks (no unreaped children, thread count = 1, no `campuscompute_*` objects in `/dev/shm` or `/tmp`) and only ever touches names with our own prefix whose owner PID is dead. Worker failure uses an **intentional, simulated exit code 42**, not an actual crash. The resource limit changes **only the child process**; its temporary file is removed.


| Shot | Command(s) to run  in terminal |
|---|---|
| S01 | `pwd; ls; uname -a` |
| S02 | `gcc --version; make --version` |
| S03 | `find . -maxdepth 2 -type f \| sort` |
| S04 | `make clean && make` |
| S05 | `./campuscompute help` |
| S06 | `./campuscompute schedule fcfs data/jobs.csv` |
| S07 | `./campuscompute schedule rr data/jobs.csv 2` |
| S08 | `./campuscompute schedule priority data/jobs.csv` |
| S09 | `./campuscompute compare data/jobs.csv` |
| S10 | `./campuscompute logger 5` |
| S11 | `./campuscompute logger 3` (shows "Joined producer/consumer" and the summary line) |
| S12 | `./campuscompute ipc` |
| S13 | `ls /dev/shm \| grep campuscompute \|\| echo "none before"; ./campuscompute ipc; ls /dev/shm \| grep campuscompute \|\| echo "none left after"` |
| S14 | `./campuscompute banker data/banker_safe.txt` |
| S15 | `./campuscompute banker data/banker_unsafe.txt` |
| S16 | `./campuscompute worker normal` |
| S17 | `./campuscompute worker fail` |
| S18 | `./campuscompute limit` |
| S19 | `./campuscompute foo; ./campuscompute schedule rr data/jobs.csv 0` |
| S20 | `./campuscompute cleanup; pgrep -a campuscompute \|\| echo "no campuscompute processes"; ls /dev/shm \| grep campuscompute \|\| echo "no campuscompute IPC objects"` |
| S21 | `./campuscompute all \| tail -40` (shows the integrated result line) |
| S22 | `ls -l logs/; wc -l logs/events.log; head -10 logs/events.log` |
| S23 | `ls -l src/ Makefile campuscompute` |
| S24 | `zip -r YOUR_FULL_SRN_source.zip src data scripts Makefile README.md; unzip -t YOUR_FULL_SRN_source.zip` |


## Cleanup

`make clean` removes only the built executable and the program's `logs/events.log`. The app releases its own IPC resources (`shm_unlink`, `sem_unlink`) during each run and waits for child processes. `./campuscompute cleanup` removes only stale `campuscompute_*` objects whose owning PID is dead; neither it nor the Makefile touches any other `/dev/shm` object or unrelated file.
