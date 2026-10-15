/**
 * @file main.c
 * @brief Coordinator module for Linux Process Management System
 * @author Leader (tit-coder) - Linux System Programming Team
 */

#define _POSIX_C_SOURCE 200809L

#include "process_manager.h"
#include "signal_handler.h"
#include "monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

static void print_banner(void) {
    printf("\n=================================================================\n");
    printf("   LINUX PROCESS MANAGEMENT SYSTEM (C11 / POSIX.1-2008)          \n");
    printf("   Subject: System Programming - HCMUT / Linux Dev Team          \n");
    printf("   Team Leader: tit-coder | Repo: linux-process-control         \n");
    printf("=================================================================\n");
}

static void print_menu(pid_t worker_pid) {
    int alive = (worker_pid > 0) ? pm_is_worker_alive(worker_pid) : 0;
    
    printf("\n----------------------- [ CONTROL MENU ] ------------------------\n");
    printf(" Active Worker PID: %d [%s]\n", 
           worker_pid, 
           (alive ? "ACTIVE" : "TERMINATED / NONE"));
    printf("-----------------------------------------------------------------\n");
    printf("  1. Suspend Process   (Send SIGSTOP - Pause execution)\n");
    printf("  2. Resume Process    (Send SIGCONT - Resume execution)\n");
    printf("  3. Show Process Status (Inspect /proc/<PID>/status)\n");
    printf("  4. Terminate Process (Send SIGTERM & reap child)\n");
    printf("  5. Exit              (Safely terminate worker & quit)\n");
    printf("-----------------------------------------------------------------\n");
    printf("Enter your choice [1-5]: ");
    fflush(stdout);
}

/**
 * @brief Safely read an integer choice from user input, avoiding infinite loops on bad input.
 */
static int read_user_choice(void) {
    char buffer[128];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        // EOF encountered (e.g. piped input ended or Ctrl+D)
        return 5;
    }

    // Trim trailing newline or whitespace
    buffer[strcspn(buffer, "\r\n")] = '\0';

    if (strlen(buffer) == 0) {
        return -1; // Empty line
    }

    char *endptr = NULL;
    long val = strtol(buffer, &endptr, 10);
    if (*endptr != '\0' || val < 1 || val > 5) {
        return -1; // Not a valid integer in [1-5]
    }

    return (int)val;
}

/**
 * @brief CLI direct test dispatcher for automated unit/integration testing.
 */
static int handle_cli_mode(int argc, char *argv[]) {
    if (argc >= 3 && strcmp(argv[1], "--test-signal") == 0) {
        // Syntax: --test-signal <SIGSTOP|SIGCONT|SIGTERM> <PID>
        const char *sig_name = argv[2];
        char *endptr = NULL;
        long target_pid_l = (argc >= 4) ? strtol(argv[3], &endptr, 10) : -1;
        pid_t target_pid = (pid_t)target_pid_l;

        if (argc < 4 || *endptr != '\0') {
            fprintf(stderr, "[Error] Missing or invalid target PID: '%s'\n", (argc >= 4) ? argv[3] : "EMPTY");
            return 1;
        }

        // Set worker PID for validation test
        pm_set_worker_pid(target_pid);

        if (strcmp(sig_name, "SIGSTOP") == 0) {
            return (sig_send_suspend(target_pid) == 0) ? 0 : 1;
        } else if (strcmp(sig_name, "SIGCONT") == 0) {
            return (sig_send_resume(target_pid) == 0) ? 0 : 1;
        } else if (strcmp(sig_name, "SIGTERM") == 0) {
            return (sig_send_terminate(target_pid) == 0) ? 0 : 1;
        } else {
            fprintf(stderr, "[Error] Unknown signal option: %s\n", sig_name);
            return 1;
        }
    } else if (argc >= 3 && strcmp(argv[1], "--test-status") == 0) {
        // Syntax: --test-status <PID>
        char *endptr = NULL;
        long target_pid_l = strtol(argv[2], &endptr, 10);
        if (*endptr != '\0' || target_pid_l <= 0) {
            fprintf(stderr, "[Error] Invalid PID '%s' for status check.\n", argv[2]);
            return 1;
        }
        return (monitor_show_process_status((pid_t)target_pid_l) == 0) ? 0 : 1;
    } else if (argc >= 2 && strcmp(argv[1], "--test-invalid-pid") == 0) {
        // Syntax: --test-invalid-pid <PID_STR>
        const char *pid_str = (argc >= 3) ? argv[2] : "-999";
        char *endptr = NULL;
        long pid_l = strtol(pid_str, &endptr, 10);
        if (*endptr != '\0' || pid_l <= 0) {
            fprintf(stderr, "[Validation] Successfully rejected malformed/non-positive PID: '%s'\n", pid_str);
            return 0; // Test passes by successfully rejecting invalid PID
        }
        // Attempt operation on non-managed PID
        pm_set_worker_pid(12345); // Pretend managed PID is 12345
        if (sig_send_suspend((pid_t)pid_l) != 0) {
            fprintf(stderr, "[Validation] Correctly denied operation on unmanaged/invalid PID: %ld\n", pid_l);
            return 0;
        }
        return 1;
    }
    return -1; // Not in CLI test mode
}

int main(int argc, char *argv[]) {
    // Check if invoked in CLI automated test mode
    int cli_ret = handle_cli_mode(argc, argv);
    if (cli_ret != -1) {
        return cli_ret;
    }

    // Set up signal handlers for parent process to avoid leaving orphan/zombie processes
    sig_init_parent_handlers();

    print_banner();

    // Fork and spawn worker process as required
    printf("\n[Initiating] Creating child worker process via fork()...\n");
    pid_t worker_pid = pm_spawn_worker();
    if (worker_pid <= 0) {
        fprintf(stderr, "[Error] Failed to initialize worker process. Exiting.\n");
        return EXIT_FAILURE;
    }

    printf("[Info] Worker process running with PID: %d.\n", worker_pid);
    sleep(1); // Give child time to print its initial greeting

    int running = 1;
    while (running) {
        // Periodically reap zombies without blocking
        pm_reap_zombies();

        print_menu(worker_pid);
        int choice = read_user_choice();

        switch (choice) {
            case 1: // Suspend Process
                printf("\n--- [Action 1: Suspend Process] ---\n");
                if (worker_pid <= 0 || !pm_is_worker_alive(worker_pid)) {
                    printf("[Warning] Worker process is not alive or has terminated.\n");
                } else {
                    sig_send_suspend(worker_pid);
                }
                break;

            case 2: // Resume Process
                printf("\n--- [Action 2: Resume Process] ---\n");
                if (worker_pid <= 0 || !pm_is_worker_alive(worker_pid)) {
                    printf("[Warning] Worker process is not alive or has terminated.\n");
                } else {
                    sig_send_resume(worker_pid);
                }
                break;

            case 3: // Show Process Status
                printf("\n--- [Action 3: Show Process Status] ---\n");
                if (worker_pid <= 0) {
                    printf("[Warning] No active worker process PID to inspect.\n");
                } else {
                    monitor_show_process_status(worker_pid);
                }
                break;

            case 4: // Terminate Process
                printf("\n--- [Action 4: Terminate Process] ---\n");
                if (worker_pid <= 0 || !pm_is_worker_alive(worker_pid)) {
                    printf("[Warning] Worker process is already inactive or terminated.\n");
                } else {
                    pm_terminate_and_cleanup(worker_pid);
                    worker_pid = pm_get_worker_pid();
                }
                break;

            case 5: // Exit
                printf("\n--- [Action 5: Exit Program] ---\n");
                pm_cleanup_on_exit();
                running = 0;
                printf("[Shutdown] Linux Process Management System exited cleanly.\n");
                break;

            default:
                printf("\n[Invalid Input] Please enter a valid option number between 1 and 5.\n");
                break;
        }

        // Small pause between actions for clean output readability
        struct timespec pause_ts = { .tv_sec = 0, .tv_nsec = 300000000L }; // 300ms
        nanosleep(&pause_ts, NULL);
    }

    return EXIT_SUCCESS;
}
