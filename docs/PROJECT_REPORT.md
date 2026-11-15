# Project Report: Process Resource Guardian (ProcGuard)

**Course:** CSC-251L Operating Systems Lab  
**Semester:** Fall 2026  
**Implementation language:** C  
**Platform:** Linux / Ubuntu  
**Compiler:** GCC

**Team members**
- Kelash — process-management implementation and integration
- Ubedullah Mahar — testing, process recovery and system-call handling
- Abdul Moiz — documentation, testing and presentation

---

## Abstract

Process Guardian (ProcGuard) is a C-based Linux process supervisor that launches a target application, monitors its termination, and automatically restarts it when it exits unsuccessfully. The supervisor creates a child process with `fork()`, replaces the child's program image using `execvp()`, and waits for the child using `waitpid()`. It interprets the wait status with the standard process-status macros and applies a user-defined restart limit. The project demonstrates process creation, program execution, parent-child synchronization, exit-status handling, signal handling, and zombie prevention.

## 1. Introduction

Operating systems provide mechanisms for creating processes, executing programs, coordinating parent and child processes, and collecting termination information. Applications may terminate successfully, return an error status, or be terminated by a signal. A supervisor can use these mechanisms to detect unsuccessful termination and attempt recovery by restarting the application.

ProcGuard is a small educational implementation of this idea. It is intended to demonstrate Linux process-management concepts in a focused, inspectable C program.

## 2. Problem Statement

When an application terminates unexpectedly or returns an error, a user may need to start it again manually. This project implements a supervisor that monitors a selected program and automatically retries unsuccessful executions within a configured limit.

## 3. Objectives

1. Accept a target executable and restart limit from the command line.
2. Create a child process using `fork()`.
3. Execute the requested application using `execvp()`.
4. Wait for and reap the child using `waitpid()`.
5. Distinguish normal exit from signal-based termination.
6. Restart unsuccessful executions up to the configured limit.
7. Display process activity and a final execution summary.
8. Demonstrate safe handling of Ctrl+C and selected termination signals.

## 4. Scope

The current project supervises one target program at a time. It handles a zero exit as success and non-zero exits or signal termination as failure. It supports target command-line arguments and a bounded restart count.

The project does not currently monitor CPU or RAM consumption, enforce resource limits, detect hangs with a timeout, or write persistent logs to a file. The name ProcGuard refers to process supervision and recovery in this implementation, not a full resource-monitoring suite.
 
## 5. Tools and Technologies

- C programming language
- Linux process APIs and POSIX signal APIs
- GCC compiler
- GNU Make
- Bash for the automated test script

No third-party libraries are required.

## 6. System Design

The program reads the target path, restart count, and optional target arguments. It validates the restart count and prepares the argument array passed to the target. For each attempt, the parent calls `fork()`. The child enters its own process group, restores default handling for selected signals, and calls `execvp()`. The parent waits for the child and inspects the status returned by `waitpid()`.

If the target exits with status zero, supervision ends successfully. If it exits non-zero or is terminated by a signal, ProcGuard checks whether another restart is available. If so, it waits briefly and starts another attempt. If the limit is exhausted, it prints a summary and exits with a code representing the last target result.

## 7. Important System Calls and Functions

| Function / macro | Purpose |
|---|---|
| `fork()` | Creates a child process |
| `execvp()` | Replaces the child process image with the target program |
| `waitpid()` | Waits for a specific child and collects its status |
| `setpgid()` | Places the target in a separate process group |
| `kill()` | Forwards a signal to the target process group |
| `sigaction()` | Installs signal handling behavior |
| `_exit()` | Terminates the child after setup or execution failure |
| `WIFEXITED()` | Tests whether the child exited normally |
| `WEXITSTATUS()` | Retrieves the normal exit status |
| `WIFSIGNALED()` | Tests whether the child was terminated by a signal |
| `WTERMSIG()` | Retrieves the terminating signal number |

## 8. Restart Policy

The restart limit means additional attempts after the initial execution. A limit of zero permits one execution. A limit of two permits at most three executions. Exit status zero stops the supervisor. A non-zero exit or signal termination triggers a restart while the restart allowance remains.

The current implementation waits one second before each restart to avoid an immediate tight loop.

## 9. Testing Plan

The test programs cover normal termination, non-zero termination, deliberate SIGSEGV, a long-running process, and command-line argument forwarding. The automated test script checks success, restart counts, signal classification, invalid executables, invalid restart limits, and missing arguments.

The test matrix and commands are documented in `TESTING.md`. In the preparation environment, the source compiled without warnings and the automated suite completed with 10 passed and 0 failed; details and environment are recorded in `TEST_RESULTS.md`. Run the tests and take screenshots on the team's machine before submission.

## 10. Expected Results

- A successful target is executed once and is not restarted.
- A failing target is restarted no more than the configured number of times.
- Signal-based termination is identified using the wait-status macros.
- A missing executable is reported and does not cause an unbounded restart loop.
- The parent waits for each child before launching another attempt.
- The final summary reports total executions, restarts, and the final exit code.

## 11. Limitations

1. Only one target is supervised by each ProcGuard instance.
2. Any non-zero exit code is treated as failure, even if the target assigns it another meaning.
3. There is no runtime timeout for hung applications.
4. CPU, memory, disk, and network resource usage are not monitored.
5. Logs are printed to the terminal rather than stored in a persistent log file.

## 12. Future Enhancements

- CPU and memory usage monitoring.
- Configurable restart delay or exponential backoff.
- Maximum execution-time limits.
- Persistent log files and log rotation.
- Configurable restart policies for selected exit codes.
- Multiple simultaneously supervised processes.

## 13. Conclusion

ProcGuard demonstrates core operating-system process concepts through a practical Linux application. It uses process creation, program replacement, parent-child synchronization, termination-status analysis, signal forwarding, and bounded automatic restart. The project provides a foundation for further exploration of process supervision and recovery.
