/**
 * @file process_manager.c
 * @brief Implementation of process management module (Worker lifecycle & reaping)
 * @author TV2 - Member of Linux System Programming Team (tit-coder)
 */

#define _POSIX_C_SOURCE 200809L

#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

static pid_t g_worker_pid = 0;
static volatile sig_atomic_t g_worker_running = 1;

/**
 * @brief Signal handler inside the Worker child process.
 */
static void worker_sigterm_handler(int sig) {
    (void)sig;
    g_worker_running = 0;
}

/**
 * @brief Worker process execution loop.
 */
static void run_worker_loop(void) {
    // Reset signal mask and install SIGTERM handler for graceful termination
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = worker_sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("[Worker] Failed to setup SIGTERM handler");
        exit(EXIT_FAILURE);
    }

    // Restore default behavior for SIGINT in child so parent handles Ctrl+C cleanly
    signal(SIGINT, SIG_IGN);

    pid_t pid = getpid();
    pid_t ppid = getppid();
    printf("\n>>> [Worker PID %d] Worker process started successfully (Parent PID: %d).\n", pid, ppid);
    printf(">>> [Worker PID %d] Incrementing counter every 1s. Suspend (SIGSTOP) will pause it.\n", pid);
    fflush(stdout);

    unsigned long counter = 0;
    while (g_worker_running) {
        sleep(1);
        if (!g_worker_running) {
            break;
        }
        counter++;
        time_t now = time(NULL);
        char time_str[32];
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm_info);

        printf("\n[Worker PID %d] [%s] Heartbeat tick: count = %lu\n", pid, time_str, counter);
        fflush(stdout);
    }

    printf("\n>>> [Worker PID %d] Received SIGTERM. Cleaning up and exiting cleanly...\n", pid);
    fflush(stdout);
    exit(EXIT_SUCCESS);
}

pid_t pm_spawn_worker(void) {
    if (g_worker_pid > 0 && pm_is_worker_alive(g_worker_pid)) {
        printf("[Process Manager] Worker already running with PID: %d.\n", g_worker_pid);
        return g_worker_pid;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("[Process Manager] fork() failed");
        return -1;
    }

    if (pid == 0) {
        // Child process
        run_worker_loop();
        _exit(EXIT_SUCCESS); // Should not reach here
    }

    // Parent process
    g_worker_pid = pid;
    printf("[Process Manager] Spawned worker process PID: %d\n", g_worker_pid);
    return g_worker_pid;
}

pid_t pm_get_worker_pid(void) {
    return g_worker_pid;
}

void pm_set_worker_pid(pid_t pid) {
    g_worker_pid = pid;
}

int pm_is_worker_alive(pid_t pid) {
    if (pid <= 0) {
        return -1;
    }

    int status = 0;
    pid_t res = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

    if (res == pid) {
        if (WIFEXITED(status)) {
            printf("[Process Manager] Process %d exited normally with code %d.\n", pid, WEXITSTATUS(status));
            if (pid == g_worker_pid) {
                g_worker_pid = 0;
            }
            return 0;
        } else if (WIFSIGNALED(status)) {
            printf("[Process Manager] Process %d was killed by signal %d (%s).\n",
                   pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
            if (pid == g_worker_pid) {
                g_worker_pid = 0;
            }
            return 0;
        } else if (WIFSTOPPED(status)) {
            // Process is alive but currently stopped (T)
            return 1;
        } else if (WIFCONTINUED(status)) {
            // Process resumed
            return 1;
        }
    } else if (res == 0) {
        // Process status has not changed, still running
        return 1;
    } else {
        // waitpid error or process not our child
        if (errno == ECHILD) {
            // Might not be direct child, verify via kill(pid, 0)
            if (kill(pid, 0) == 0) {
                return 1;
            } else if (errno == ESRCH) {
                if (pid == g_worker_pid) {
                    g_worker_pid = 0;
                }
                return 0;
            } else if (errno == EPERM) {
                return 1; // Process exists but owned by someone else
            }
        }
    }

    // Final sanity check with kill(pid, 0)
    if (kill(pid, 0) == 0) {
        return 1;
    } else if (errno == ESRCH) {
        if (pid == g_worker_pid) {
            g_worker_pid = 0;
        }
        return 0;
    }

    return 0;
}

int pm_terminate_and_cleanup(pid_t pid) {
    if (pid <= 0) {
        fprintf(stderr, "[Process Manager] Invalid PID %d provided for termination.\n", pid);
        return -1;
    }

    if (!pm_is_worker_alive(pid)) {
        printf("[Process Manager] Process %d is not active or has already terminated.\n", pid);
        if (pid == g_worker_pid) {
            g_worker_pid = 0;
        }
        return 0;
    }

    printf("[Process Manager] Sending SIGTERM to process %d...\n", pid);
    if (kill(pid, SIGTERM) == -1) {
        perror("[Process Manager] kill(SIGTERM) failed");
        return -1;
    }

    // Wait up to 1 second (10 * 100ms) for graceful exit
    int status = 0;
    int reaped = 0;
    for (int i = 0; i < 10; ++i) {
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) {
            reaped = 1;
            if (WIFEXITED(status)) {
                printf("[Process Manager] Process %d terminated gracefully with exit status %d.\n",
                       pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[Process Manager] Process %d terminated by signal %d (%s).\n",
                       pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
            }
            break;
        } else if (r == -1 && errno == ECHILD) {
            // Child already reaped
            reaped = 1;
            break;
        }
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000L }; // 100ms
        nanosleep(&ts, NULL);
    }

    // If still alive, escalate to SIGKILL
    if (!reaped && pm_is_worker_alive(pid)) {
        printf("[Process Manager] Process %d did not respond to SIGTERM. Sending SIGKILL...\n", pid);
        if (kill(pid, SIGKILL) == -1) {
            perror("[Process Manager] kill(SIGKILL) failed");
        } else {
            waitpid(pid, &status, 0); // Blocking reap after SIGKILL
            printf("[Process Manager] Process %d forcibly killed and reaped.\n", pid);
        }
    }

    if (pid == g_worker_pid) {
        g_worker_pid = 0;
    }

    return 0;
}

void pm_reap_zombies(void) {
    int status = 0;
    pid_t reaped;
    while ((reaped = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("\n[Process Manager] Reaped zombie child process PID %d.\n", reaped);
        if (reaped == g_worker_pid) {
            g_worker_pid = 0;
        }
    }
}

void pm_cleanup_on_exit(void) {
    if (g_worker_pid > 0) {
        printf("[Process Manager] Safe exit: cleaning up worker process PID %d...\n", g_worker_pid);
        pm_terminate_and_cleanup(g_worker_pid);
        g_worker_pid = 0;
    }
}
