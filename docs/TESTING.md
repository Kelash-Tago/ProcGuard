# ProcGuard Testing Guide

## Build

```bash
make clean
make
```

## Automated tests

```bash
make test
```

The test script captures each command's output and checks exit codes and key summary messages.

## Test matrix

| ID | Scenario | Command / method | Expected behavior |
|---|---|---|---|
| T01 | Normal exit | `./procguard ./tests/normal_exit 3` | One execution, zero restarts, exit 0 |
| T02 | Non-zero exit | `./procguard ./tests/error_exit 2` | Three executions total, two restarts, final exit 1 |
| T03 | Signal termination | `./procguard ./tests/crash 1` | SIGSEGV (11) detected, two executions total, final exit 139 |
| T04 | Missing executable | `./procguard ./tests/no_such_program 1` | `execvp` error, two attempts, final exit 127 |
| T05 | Argument forwarding | `./procguard ./tests/arguments 0 hello 42` | Target receives `hello` and `42` |
| T06 | Zero restarts | `./procguard ./tests/error_exit 0` | One execution, no restart |
| T07 | Invalid restart count | `./procguard ./tests/normal_exit abc` | Rejected with usage error code 2 |
| T08 | Negative restart count | `./procguard ./tests/normal_exit -1` | Rejected with usage error code 2 |
| T09 | Missing arguments | `./procguard` | Usage instructions and exit code 2 |
| T10 | Manual interrupt | `./procguard ./tests/infinite_loop 0`, then Ctrl+C | Signal forwarded; target terminates and is reaped |

## Manual evidence to capture

Create screenshots in `docs/screenshots/` showing:

1. `make` completing successfully.
2. Normal execution.
3. Error execution and restart limit reached.
4. Signal/crash detection.
5. `make test` passing.
6. Optional: Ctrl+C forwarded to the infinite-loop test.

Screenshots should be captured on the team's own Linux machine. The repository intentionally does not include fabricated screenshots or claim that a test was run on a student's computer.

## Notes

- The shell may display a diagnostic when a process is killed by a signal. ProcGuard's automated test checks ProcGuard's own exit code and captured log.
- `139` is the conventional shell-style code `128 + 11` used by ProcGuard to report that the last target was terminated by SIGSEGV.
- `infinite_loop` is a manual test; do not run it with a positive restart limit unless you intend to stop it manually.
