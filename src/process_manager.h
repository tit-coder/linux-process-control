/**
 * @file process_manager.h
 * @brief Header for process management module (Worker spawn, lifecycle, cleanup)
 * @author TV2 - Member of Linux System Programming Team (tit-coder)
 */

#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>
#include <stdbool.h>

/**
 * @brief Spawn a new background worker process using fork().
 * 
 * Worker periodically increments a counter and prints a heartbeat every second.
 * 
 * @return pid_t The PID of the newly spawned worker, or -1 on error.
 */
pid_t pm_spawn_worker(void);

/**
 * @brief Get the PID of the current managed worker process.
 * 
 * @return pid_t Worker PID, or 0 if no worker is active.
 */
pid_t pm_get_worker_pid(void);

/**
 * @brief Set or register the active worker PID.
 * 
 * @param pid PID to register.
 */
void pm_set_worker_pid(pid_t pid);

/**
 * @brief Check if a worker process is still alive and update its status.
 * 
 * Uses kill(pid, 0) and non-blocking waitpid() to accurately determine existence
 * and reap any zombie state without blocking the caller.
 * 
 * @param pid PID to check.
 * @return int 1 if alive and running/stopped, 0 if terminated/dead/non-existent, -1 on invalid argument.
 */
int pm_is_worker_alive(pid_t pid);

/**
 * @brief Safely terminate and reap the worker process.
 * 
 * Sends SIGTERM, waits gracefully, and escalates to SIGKILL if necessary to prevent zombies.
 * 
 * @param pid PID to terminate.
 * @return int 0 on success, -1 on failure.
 */
int pm_terminate_and_cleanup(pid_t pid);

/**
 * @brief Reap any terminated child processes without blocking.
 * 
 * Calls waitpid(-1, &status, WNOHANG) in a loop to eliminate potential zombies.
 */
void pm_reap_zombies(void);

/**
 * @brief Clean up all managed resources and terminate active worker before program exit.
 */
void pm_cleanup_on_exit(void);

#endif /* PROCESS_MANAGER_H */
