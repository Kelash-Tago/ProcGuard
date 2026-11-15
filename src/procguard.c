#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*
 * ProcGuard
 * A small Linux process supervisor that launches one program, waits for it,
 * classifies its termination, and restarts unsuccessful executions up to a
 * user-configured limit.
 *
 * Usage:
 *   ./procguard <program> <max_restarts> [program_arguments ...]
 *
 * max_restarts counts additional executions after the initial execution.
 */

#define MAX_RESTARTS_LIMIT 1000000L
#define RESTART_DELAY_SECONDS 1U

static volatile sig_atomic_t g_received_signal = 0;
static volatile sig_atomic_t g_supervisor_signal = 0;

static void handle_supervisor_signal(int signal_number)
{
    /* Keep the handler async-signal-safe: only assign a sig_atomic_t value. */
    g_received_signal = signal_number;
    g_supervisor_signal = signal_number;
}

static int install_signal_handlers(void)
{
    const int handled_signals[] = { SIGINT, SIGTERM, SIGHUP, SIGQUIT };
    struct sigaction action;
    size_t i;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_supervisor_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0; /* waitpid() should be interruptible. */

    for (i = 0; i < sizeof(handled_signals) / sizeof(handled_signals[0]); ++i) {
        if (sigaction(handled_signals[i], &action, NULL) == -1) {
            perror("ProcGuard: sigaction");
            return -1;
        }
    }

    return 0;
}

static int reset_child_signal_handlers(void)
{
    const int handled_signals[] = { SIGINT, SIGTERM, SIGHUP, SIGQUIT };
    struct sigaction action;
    size_t i;

    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);

    for (i = 0; i < sizeof(handled_signals) / sizeof(handled_signals[0]); ++i) {
        if (sigaction(handled_signals[i], &action, NULL) == -1) {
            return -1;
        }
    }

    return 0;
}

static int parse_restart_limit(const char *text, long *value_out)
{
    char *end = NULL;
    long value;

    if (text == NULL || text[0] == '\0' || value_out == NULL) {
        return -1;
    }

    errno = 0;
    value = strtol(text, &end, 10);

    if (errno == ERANGE || end == text || *end != '\0' || value < 0 ||
        value > MAX_RESTARTS_LIMIT) {
        return -1;
    }

    *value_out = value;
    return 0;
}

static void print_usage(const char *program_name)
{
    fprintf(stderr,
            "Usage: %s <program> <max_restarts> [program_arguments ...]\n"
            "Example: %s ./tests/error_exit 3\n"
            "Example: %s /bin/echo 2 Hello from ProcGuard\n",
            program_name, program_name, program_name);
}

static void print_timestamp(void)
{
    time_t now = time(NULL);
    struct tm local_time;
    char buffer[32];

    if (now == (time_t)-1 || localtime_r(&now, &local_time) == NULL ||
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time) == 0) {
        printf("[ProcGuard]");
        return;
    }

    printf("[%s]", buffer);
}

static void log_message(const char *message)
{
    print_timestamp();
    printf(" %s\n", message);
    fflush(stdout);
}

static void log_number(const char *label, long value)
{
    print_timestamp();
    printf(" %s%ld\n", label, value);
    fflush(stdout);
}

static void log_pid(const char *label, pid_t pid)
{
    print_timestamp();
    printf(" %s%ld\n", label, (long)pid);
    fflush(stdout);
}

static void forward_pending_signal(pid_t child_pid)
{
    int signal_number = (int)g_received_signal;

    if (signal_number == 0) {
        return;
    }

    /* Clear before kill(), so a second signal can be recorded by the handler. */
    g_received_signal = 0;
    print_timestamp();
    printf(" ProcGuard received signal %d; forwarding it to child process group.\n",
           signal_number);
    fflush(stdout);

    if (kill(-child_pid, signal_number) == -1 && errno != ESRCH) {
        perror("ProcGuard: forwarding signal");
    }
}

static int wait_for_child(pid_t child_pid, int *status_out)
{
    pid_t result;

    for (;;) {
        if (g_received_signal != 0) {
            forward_pending_signal(child_pid);
        }

        result = waitpid(child_pid, status_out, 0);

        if (result == child_pid) {
            return 0;
        }

        if (result == -1 && errno == EINTR) {
            continue;
        }

        if (result == -1) {
            perror("ProcGuard: waitpid");
            return -1;
        }
    }
}

static void child_exec(char *const target_argv[])
{
    /* Put the target in its own process group so signals can be forwarded. */
    if (setpgid(0, 0) == -1) {
        perror("ProcGuard: child setpgid");
        _exit(126);
    }

    if (reset_child_signal_handlers() == -1) {
        perror("ProcGuard: resetting child signal handlers");
        _exit(126);
    }

    execvp(target_argv[0], target_argv);

    /* execvp() returns only on failure. */
    {
        int saved_errno = errno;
        errno = saved_errno;
        perror("ProcGuard: execvp");
        _exit(saved_errno == ENOENT ? 127 : 126);
    }
}

static int run_one_attempt(char *const target_argv[], long attempt_number,
                           int *status_out, pid_t *child_pid_out)
{
    pid_t child_pid;

    log_number("Starting execution #", attempt_number);
    fflush(NULL);

    child_pid = fork();
    if (child_pid == -1) {
        perror("ProcGuard: fork");
        return -1;
    }

    if (child_pid == 0) {
        child_exec(target_argv);
        _exit(126); /* Defensive; child_exec() does not return. */
    }

    *child_pid_out = child_pid;

    /*
     * Also set the group from the parent to close the small scheduling race
     * before the child calls setpgid(). EACCES/ESRCH can occur if the child
     * has already exec'd or exited, and are harmless here.
     */
    if (setpgid(child_pid, child_pid) == -1 &&
        errno != EACCES && errno != ESRCH) {
        perror("ProcGuard: parent setpgid");
    }

    log_pid("Created child PID: ", child_pid);
    log_message("Monitoring child process...");

    if (wait_for_child(child_pid, status_out) == -1) {
        return -1;
    }

    return 0;
}

static int classify_child_status(int status, int *should_restart,
                                 int *final_exit_code)
{
    *should_restart = 0;
    *final_exit_code = 1;

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        print_timestamp();
        printf(" Child terminated normally; exit status: %d\n", exit_code);
        fflush(stdout);

        *final_exit_code = exit_code;
        if (exit_code != 0) {
            *should_restart = 1;
        }
        return 0;
    }

    if (WIFSIGNALED(status)) {
        int signal_number = WTERMSIG(status);
        print_timestamp();
        printf(" Child terminated by signal %d", signal_number);
#ifdef WCOREDUMP
        if (WCOREDUMP(status)) {
            printf(" (core dumped)");
        }
#endif
        printf(".\n");
        fflush(stdout);

        *final_exit_code = 128 + signal_number;
        *should_restart = 1;
        return 0;
    }

    log_message("Child ended in an unrecognized state; treating it as a failure.");
    *final_exit_code = 1;
    *should_restart = 1;
    return 0;
}

static void print_summary(long executions, long restarts, int final_exit_code)
{
    printf("\n==================================================\n");
    printf("                 PROCGUARD SUMMARY\n");
    printf("==================================================\n");
    printf("Total executions : %ld\n", executions);
    printf("Total restarts   : %ld\n", restarts);
    printf("Final exit code  : %d\n", final_exit_code);
    printf("==================================================\n");
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    long max_restarts;
    long restart_count = 0;
    long execution_count = 0;
    int final_exit_code = 1;
    char **target_argv;
    size_t target_argc;
    size_t i;

    if (argc < 3) {
        print_usage(argv[0]);
        return 2;
    }

    if (argv[1][0] == '\0') {
        fprintf(stderr, "ProcGuard: target program cannot be empty.\n");
        return 2;
    }

    if (parse_restart_limit(argv[2], &max_restarts) == -1) {
        fprintf(stderr,
                "ProcGuard: max_restarts must be an integer from 0 to %ld.\n",
                MAX_RESTARTS_LIMIT);
        return 2;
    }

    /*
     * Build target argv, excluding ProcGuard's restart-count argument.
     * Example: procguard ./app 2 alpha -> {"./app", "alpha", NULL}.
     */
    target_argc = (size_t)(argc - 2);
    target_argv = calloc(target_argc + 1U, sizeof(*target_argv));
    if (target_argv == NULL) {
        perror("ProcGuard: calloc");
        return 1;
    }

    target_argv[0] = argv[1];
    for (i = 3U; i < (size_t)argc; ++i) {
        target_argv[i - 2U] = argv[i];
    }
    target_argv[target_argc] = NULL;

    if (install_signal_handlers() == -1) {
        free(target_argv);
        return 1;
    }

    printf("==================================================\n");
    printf("          PROCESS RESOURCE GUARDIAN\n");
    printf("==================================================\n");
    printf("Target program  : %s\n", target_argv[0]);
    printf("Maximum restarts: %ld\n", max_restarts);
    printf("Restart policy  : restart non-zero exits and signals\n");
    printf("==================================================\n");
    fflush(stdout);

    for (;;) {
        int status = 0;
        int should_restart = 0;
        pid_t child_pid = -1;

        if (g_supervisor_signal != 0) {
            int signal_number = (int)g_supervisor_signal;
            g_received_signal = 0;
            g_supervisor_signal = 0;
            print_timestamp();
            printf(" Supervisor stopping after signal %d.\n", signal_number);
            final_exit_code = 128 + signal_number;
            break;
        }

        ++execution_count;
        if (run_one_attempt(target_argv, execution_count, &status,
                            &child_pid) == -1) {
            final_exit_code = 1;
            break;
        }

        /* Classify the reaped child before deciding how the supervisor exits. */
        (void)classify_child_status(status, &should_restart, &final_exit_code);

        /* A signal can arrive just as waitpid() reaps the child. */
        if (g_supervisor_signal != 0) {
            int signal_number = (int)g_supervisor_signal;
            g_received_signal = 0;
            g_supervisor_signal = 0;
            print_timestamp();
            printf(" Supervisor received signal %d; stopping.\n", signal_number);
            final_exit_code = 128 + signal_number;
            break;
        }

        if (!should_restart) {
            log_message("Target completed successfully; no restart needed.");
            break;
        }

        if (restart_count >= max_restarts) {
            log_message("Maximum restart limit reached; stopping supervision.");
            break;
        }

        ++restart_count;
        print_timestamp();
        printf(" Restart %ld of %ld scheduled.\n", restart_count, max_restarts);
        fflush(stdout);

        if (RESTART_DELAY_SECONDS > 0U) {
            unsigned int remaining = sleep(RESTART_DELAY_SECONDS);
            (void)remaining;
        }
    }

    print_summary(execution_count, restart_count, final_exit_code);
    free(target_argv);

    return final_exit_code;
}
