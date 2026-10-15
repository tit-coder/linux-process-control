/**
 * @file monitor.c
 * @brief Implementation of process status reader from Linux /proc filesystem
 * @author TV4 - Member of Linux System Programming Team (tit-coder)
 */

#define _POSIX_C_SOURCE 200809L

#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

const char *monitor_explain_state(char state) {
    switch (state) {
        case 'T':
            return "Stopped (Suspended by signal e.g. SIGSTOP - Execution paused)";
        case 't':
            return "Tracing stop (Suspended by debugger during ptrace)";
        case 'R':
            return "Running / Runnable (Actively executing on CPU or waiting in run-queue)";
        case 'S':
            return "Sleeping (Interruptible sleep - waiting for event/timer; NOT actively using CPU)";
        case 'D':
            return "Disk sleep (Uninterruptible sleep - waiting for disk/hardware I/O)";
        case 'Z':
            return "Zombie (Terminated but parent has not yet called waitpid() to reap)";
        case 'X':
            return "Dead (Process completely terminated and being removed by kernel)";
        default:
            return "Unknown process state";
    }
}

int monitor_read_proc_status(pid_t pid, process_info_t *info) {
    if (pid <= 0 || info == NULL) {
        fprintf(stderr, "[Monitor] Invalid arguments provided to monitor_read_proc_status.\n");
        return -1;
    }

    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            fprintf(stderr, "[Monitor] Process %d does not exist (cannot access %s).\n", pid, path);
        } else {
            perror("[Monitor] Failed to open /proc file");
        }
        return -1;
    }

    memset(info, 0, sizeof(*info));
    info->pid = pid;
    strncpy(info->vm_size, "N/A", sizeof(info->vm_size) - 1);
    strncpy(info->vm_rss, "N/A", sizeof(info->vm_rss) - 1);

    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Strip trailing newline
        line[strcspn(line, "\r\n")] = '\0';

        if (strncmp(line, "Name:", 5) == 0) {
            char *val = line + 5;
            while (*val == ' ' || *val == '\t') val++;
            strncpy(info->name, val, sizeof(info->name) - 1);
        } else if (strncmp(line, "State:", 6) == 0) {
            char *val = line + 6;
            while (*val == ' ' || *val == '\t') val++;
            strncpy(info->state_raw, val, sizeof(info->state_raw) - 1);
            info->state_char = val[0];
            info->state_desc = monitor_explain_state(info->state_char);
        } else if (strncmp(line, "PPid:", 5) == 0) {
            char *val = line + 5;
            info->ppid = (pid_t)strtol(val, NULL, 10);
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            char *val = line + 7;
            while (*val == ' ' || *val == '\t') val++;
            strncpy(info->vm_size, val, sizeof(info->vm_size) - 1);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            char *val = line + 6;
            while (*val == ' ' || *val == '\t') val++;
            strncpy(info->vm_rss, val, sizeof(info->vm_rss) - 1);
        } else if (strncmp(line, "Threads:", 8) == 0) {
            char *val = line + 8;
            info->threads = (int)strtol(val, NULL, 10);
        }
    }

    fclose(fp);
    return 0;
}

int monitor_show_process_status(pid_t pid) {
    process_info_t info;
    if (monitor_read_proc_status(pid, &info) != 0) {
        return -1;
    }

    printf("\n============================================================\n");
    printf("              PROCESS STATUS (/proc/%d/status)              \n", pid);
    printf("============================================================\n");
    printf("  %-16s : %d\n", "Process ID (PID)", info.pid);
    printf("  %-16s : %s\n", "Process Name", info.name);
    printf("  %-16s : %d\n", "Parent PID (PPid)", info.ppid);
    printf("  %-16s : %s\n", "Raw State", info.state_raw);
    printf("  %-16s : [%c] %s\n", "Interpreted State", info.state_char, info.state_desc);
    printf("  %-16s : %s\n", "Virtual Memory", info.vm_size);
    printf("  %-16s : %s\n", "Resident Set (RSS)", info.vm_rss);
    printf("  %-16s : %d\n", "Thread Count", info.threads);
    printf("============================================================\n");

    return 0;
}
