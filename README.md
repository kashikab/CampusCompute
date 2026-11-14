# CampusCompute — Operating Systems Level 3 Mini-Project

A Linux/WSL Ubuntu C implementation of the eight functional requirements: job lifecycle and validation, FCFS/RR/non-preemptive priority scheduling, a bounded POSIX-threads logger, POSIX shared-memory + named-semaphore IPC, Banker's safety test, fork/waitpid worker-failure detection, RLIMIT_FSIZE, and an independent command-line interface.

**Important submission rule:** Capture **real, unedited** terminal screenshots with your **own complete SRN in the terminal prompt**. Do not submit screenshots produced on someone else's machine. The PDF script only combines images you capture; it does not generate terminal screenshots. No standalone project report is required.

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

## 2. Transfer and extract the project archive

Download `CampusCompute_Project.zip` to your Windows `Downloads` folder. In Ubuntu, replace `WINDOWS_USER` with your real Windows account folder name:

```bash
cd ~
cp /mnt/c/Users/WINDOWS_USER/Downloads/CampusCompute_Project.zip .
unzip CampusCompute_Project.zip
cd CampusCompute
```

Alternatively, open the ZIP using Windows Explorer and copy the `CampusCompute` folder into your WSL Linux home folder. Run all commands below **from inside `~/CampusCompute`**.

## 3. Configure the required SRN prompt BEFORE screenshots

**Replace `YOUR_FULL_SRN` with your actual full student registration number:**

```bash
export SRN='YOUR_FULL_SRN'
export PS1="[$SRN] \\w\\$ "
```

Check the prompt visibly begins with `[YOUR_ACTUAL_SRN]`. You must run these two commands in **each new Ubuntu terminal** before taking screenshots. A variable or printed text alone is insufficient: the full SRN needs to be visible in the **prompt** before each command.

## 4. Build and run the entire project

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

## 6. Capture genuine screenshots S01–S24

Every screenshot must show the **full SRN in the shell prompt**, **the command**, and enough meaningful output. Use Ubuntu's/window screenshot functionality (for example Windows `Win+Shift+S`), and save images as `screenshots/S01.png` through `screenshots/S24.png`. A Windows file picker can access Ubuntu files by opening `\\wsl$\\Ubuntu\\home\\YOUR_LINUX_USER\\CampusCompute\\screenshots`. You can also copy images from Windows Downloads using `/mnt/c/Users/WINDOWS_USER/Downloads/`.

For lengthy output, expand the terminal window or capture the relevant section with command + output visible. Never edit or fabricate terminal results.

Tip: maximise the terminal window and run `clear` before every command so each screenshot shows the full command, the full output and the SRN prompt, with no leftover text above.

| Shot | Command(s) to display in terminal |
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

For **S24**, substitute your actual SRN when naming the archive. Every PDF page will contain one screenshot and its matching one-sentence explanation.

## 7. Generate the mandatory SRN.pdf

After all 24 screenshots are saved, from inside `~/CampusCompute`:

```bash
python3 scripts/make_evidence_pdf.py --srn YOUR_FULL_SRN
```

Replace with your real SRN. The script refuses to create a PDF if any screenshot is missing, and creates `YOUR_FULL_SRN.pdf` using only the images you provide. Check the PDF visually: the screenshot prompt must show the actual SRN in every image. If any command is cut off, recapture that screenshot and regenerate.

## 8. Final submission

Submit:
- Your `YOUR_FULL_SRN.pdf` evidence PDF containing S01–S24 and one sentence each.
- Your `YOUR_FULL_SRN_source.zip` if the portal requests a source archive (built using S24 instructions).
- The source folder/Makefile/README if source files are requested individually.

The provided `CampusCompute_Project.zip` is the starter/buildable project archive; **do not submit it instead of your own SRN PDF**. Git/GitHub is not required. Do not submit a separate report.

## Cleanup

`make clean` removes only the built executable and the program's `logs/events.log`. The app releases its own IPC resources (`shm_unlink`, `sem_unlink`) during each run and waits for child processes. `./campuscompute cleanup` removes only stale `campuscompute_*` objects whose owning PID is dead; neither it nor the Makefile touches any other `/dev/shm` object or unrelated file.
