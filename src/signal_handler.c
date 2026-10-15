/**
 * @file signal_handler.c
 * @brief Implementation of signal dispatching with robust validation
 * @author TV3 - Member of Linux System Programming Team (tit-coder)
 */

#define _POSIX_C_SOURCE 200809L

#include "signal_handler.h"
#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Signal handler for parent termination signals (SIGINT, SIGTERM).
 */
static void parent_termination_handler(int sig) {
    (void)sig;
    const char msg[] = "\n[Signal Controller] Parent caught interrupt/termination signal. Cleaning up...\n";
    // write is async-signal-safe
    ssize_t written = write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    (void)written;
    pm_cleanup_on_exit();
    _exit(EXIT_SUCCESS);
}

void sig_init_parent_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = parent_termination_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("[Signal Controller] sigaction(SIGINT) failed");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("[Signal Controller] sigaction(SIGTERM) failed");
    }
}

/**
 * @brief Validate that the given PID is strictly the managed worker and currently alive.
 */
static int validate_target_pid(pid_t pid) {
    if (pid <= 0) {
        fprintf(stderr, "[Signal Controller] Error: Invalid PID %d (PID must be positive).\n", pid);
        return -1;
    }

    pid_t managed_pid = pm_get_worker_pid();
    if (managed_pid <= 0) {
        fprintf(stderr, "[Signal Controller] Error: No active worker process currently managed.\n");
        return -1;
    }

    if (pid != managed_pid) {
        fprintf(stderr, "[Signal Controller] Security Violation: Target PID %d is not managed by this system (Managed PID: %d). Signal rejected!\n",
                pid, managed_pid);
        return -1;
    }

    if (!pm_is_worker_alive(pid)) {
        fprintf(stderr, "[Signal Controller] Error: Process %d is no longer alive or does not exist.\n", pid);
        return -1;
    }

    return 0;
}

int sig_send_suspend(pid_t pid) {
    if (validate_target_pid(pid) != 0) {
        return -1;
    }

    printf("[Signal Controller] Dispatching SIGSTOP to PID %d (Suspend/Pause)...\n", pid);
    if (kill(pid, SIGSTOP) == -1) {
        perror("[Signal Controller] Failed to send SIGSTOP");
        return -1;
    }

    printf("[Signal Controller] Success: SIGSTOP sent to PID %d. Process is now SUSPENDED (State: T).\n", pid);
    return 0;
}

int sig_send_resume(pid_t pid) {
    if (validate_target_pid(pid) != 0) {
        return -1;
    }

    printf("[Signal Controller] Dispatching SIGCONT to PID %d (Resume/Continue)...\n", pid);
    if (kill(pid, SIGCONT) == -1) {
        perror("[Signal Controller] Failed to send SIGCONT");
        return -1;
    }

    printf("[Signal Controller] Success: SIGCONT sent to PID %d. Process is now RESUMED (State: S/R).\n", pid);
    return 0;
}

int sig_send_terminate(pid_t pid) {
    if (validate_target_pid(pid) != 0) {
        return -1;
    }

    printf("[Signal Controller] Dispatching SIGTERM to PID %d (Graceful Termination Request)...\n", pid);
    if (kill(pid, SIGTERM) == -1) {
        perror("[Signal Controller] Failed to send SIGTERM");
        return -1;
    }

    printf("[Signal Controller] Success: SIGTERM sent to PID %d.\n", pid);
    return 0;
}
