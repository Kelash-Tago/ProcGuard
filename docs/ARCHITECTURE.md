# ProcGuard Architecture

## 1. Overview

ProcGuard is a single-parent, single-target process supervisor. It launches one target program, waits for its termination, examines the status returned by the kernel, and decides whether another execution is permitted.

## 2. Process model

```text
                   User / Shell
                        |
                        v
                 ProcGuard (parent)
                        |
                      fork()
                 _______|_______
                |               |
                v               v
          Parent process    Child process
          (supervisor)      (target launcher)
                |               |
             waitpid()       setpgid()
                |           reset signals
                |            execvp()
                |               |
                |          Target program
                |               |
                +---- status ---+
                        |
                 inspect status
                        |
             +----------+----------+
             |                     |
         exit status 0       non-zero / signal
             |                     |
          Stop            restart allowance?
                              |       |
                             yes      no
                              |       |
                          fork again  Stop
```

## 3. Main components

### Argument handling

`main()` checks that a target and restart limit were supplied. `parse_restart_limit()` uses `strtol()` and rejects empty input, trailing characters, negative numbers, overflow, and values above 1,000,000. The extra target arguments are copied into a new `argv` array so the restart-limit argument is not passed to the target.

### Child creation and execution

`run_one_attempt()` calls `fork()`. The child calls `setpgid(0, 0)`, restores default handling for the supervisor's signals, and calls `execvp()`. Successful `execvp()` replaces the child's process image; it does not create a second child. If execution fails, the child prints the operating-system error and calls `_exit()` with status 127 (not found) or 126 (other execution failure).

### Parent monitoring

The parent calls `waitpid(child_pid, &status, 0)`. The call is retried after `EINTR`, and the child is reaped before another attempt begins. This prevents the supervisor from leaving a terminated child as a zombie.

### Termination classification

- `WIFEXITED(status)` identifies normal process termination.
- `WEXITSTATUS(status)` retrieves the normal exit code.
- `WIFSIGNALED(status)` identifies termination by a signal.
- `WTERMSIG(status)` retrieves the signal number.

A zero exit code is success. A non-zero exit code or signal termination is treated as failure and can trigger a restart.

### Restart control

`restart_count` counts additional launches, not total launches. Therefore, a restart limit of `N` permits up to `N + 1` executions. A one-second pause separates attempts.

### Signal forwarding

The supervisor catches SIGINT, SIGTERM, SIGHUP, and SIGQUIT. Its handler only records the signal in a `volatile sig_atomic_t` variable. The child runs in its own process group. When interrupted, the supervisor forwards the recorded signal to the child process group with `kill(-child_pid, signal_number)` and continues waiting to reap the child.

## 4. Important invariants

1. ProcGuard supervises one target child at a time.
2. The parent waits for the child before starting another attempt.
3. Every successfully forked child is waited for on the normal supervision path.
4. A successful target exit (status 0) is never restarted.
5. The number of additional restarts never exceeds the supplied limit.
6. The target receives its own arguments, not ProcGuard's restart-limit parameter.

## 5. Out-of-scope behavior

The current implementation does not measure CPU usage, memory consumption, I/O throughput, or enforce resource quotas. It also does not detect a hung process by timeout. Its responsibility is process execution monitoring and restart recovery.
