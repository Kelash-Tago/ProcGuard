# ProcGuard Verification Record

## Verification environment

The packaged source was compiled and tested in the following preparation environment on 6 November 2026:

- Operating system: Linux x86_64 (kernel 6.18.57)
- Compiler: GCC 14.2.0
- Build tool: GNU Make 4.4.1
- Compiler flags: `-std=c11 -Wall -Wextra -Wpedantic -g`

This is the build environment used to prepare the archive; it is not a claim that the project has already been run on every team member's computer.

## Build result

Command:

```bash
make clean
make
```

Result: successful. The supervisor and all test programs compiled without compiler warnings.

A static diagnostic pass using GCC `-fanalyzer -fsyntax-only` also completed without diagnostics.

## Automated test result

Command:

```bash
make test
```

Result:

```text
[PASS] normal exit stops without restarting
[PASS] non-zero exit restarts exactly to the configured limit
[PASS] signal termination is detected and restarted
[PASS] missing target is reported and bounded by restart limit
[PASS] arguments are forwarded to the target program
[PASS] zero restart limit allows only the initial execution
[PASS] invalid restart count is rejected
[PASS] negative restart count is rejected
[PASS] missing command-line arguments display usage
[PASS] supervisor forwards SIGTERM and reaps the child

Automated test results: 10 passed, 0 failed
```

## Before submission

Run `make clean && make test` on the team's own Ubuntu machine. Save screenshots of the successful build, representative runtime tests, and final test summary in `docs/screenshots/`. Confirm the team names and course details match the submitted proposal.
