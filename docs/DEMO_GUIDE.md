# ProcGuard Demonstration Guide

Use this order for a short class demonstration.

## 1. Build

```bash
make clean
make
```

Explain that the Makefile compiles the supervisor and test applications.

## 2. Show successful execution

```bash
./procguard ./tests/normal_exit 3
```

Point out that exit status 0 is successful, so no restart occurs.

## 3. Show automatic restart

```bash
./procguard ./tests/error_exit 2
```

Point out that there is one initial execution plus two restarts, for three total executions.

## 4. Show signal detection

```bash
./procguard ./tests/crash 1
```

Point out that the supervisor identifies SIGSEGV through `WIFSIGNALED()` and `WTERMSIG()`.

## 5. Show target arguments

```bash
./procguard ./tests/arguments 0 hello 42
```

Point out that ProcGuard's restart-limit argument is not passed to the target.

## 6. Run automated tests

```bash
make test
```

## 7. Optional signal-forwarding demo

```bash
./procguard ./tests/infinite_loop 0
```

Press Ctrl+C once. Explain that ProcGuard receives the signal, forwards it to the target process group, and waits for the child to terminate.

## Presentation note

Use screenshots from your own successful run. Do not claim test results that have not actually been run on the demonstration machine.
