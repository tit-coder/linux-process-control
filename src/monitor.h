/**
 * @file monitor.h
 * @brief Header for process monitor module (reading and interpreting /proc filesystem)
 * @author TV4 - Member of Linux System Programming Team (tit-coder)
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <sys/types.h>

/**
 * @brief Structure containing parsed process status information from /proc/<PID>/status.
 */
typedef struct {
    pid_t pid;
    char name[256];
    char state_raw[64];
    char state_char;
    const char *state_desc;
    pid_t ppid;
    char vm_size[64];
    char vm_rss[64];
    int threads;
} process_info_t;

/**
 * @brief Read and parse /proc/<PID>/status for a given process.
 * 
 * @param pid Process ID to query.
 * @param info Output structure to populate.
 * @return int 0 on success, -1 if process does not exist or file cannot be opened.
 */
int monitor_read_proc_status(pid_t pid, process_info_t *info);

/**
 * @brief Display a formatted status summary of the target process to stdout.
 * 
 * Accurately interprets Linux kernel process states (R, S, D, T, t, Z).
 * 
 * @param pid Target Process ID.
 * @return int 0 on success, -1 on failure.
 */
int monitor_show_process_status(pid_t pid);

/**
 * @brief Return human-readable explanation of Linux process state character.
 * 
 * @param state Character code (R, S, D, T, t, Z, X).
 * @return const char* Detailed interpretation string.
 */
const char *monitor_explain_state(char state);

#endif /* MONITOR_H */
