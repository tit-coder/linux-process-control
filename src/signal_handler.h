/**
 * @file signal_handler.h
 * @brief Header for signal control module (POSIX signal dispatching and validation)
 * @author TV3 - Member of Linux System Programming Team (tit-coder)
 */

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <sys/types.h>

/**
 * @brief Initialize parent signal handlers for clean teardown on SIGINT/SIGTERM.
 */
void sig_init_parent_handlers(void);

/**
 * @brief Suspend the specified worker process using SIGSTOP.
 * 
 * Validates PID ownership, verifies target existence, and checks system call result.
 * 
 * @param pid Target worker PID.
 * @return int 0 on success, -1 on failure.
 */
int sig_send_suspend(pid_t pid);

/**
 * @brief Resume the specified worker process using SIGCONT.
 * 
 * Validates PID ownership, verifies target existence, and checks system call result.
 * 
 * @param pid Target worker PID.
 * @return int 0 on success, -1 on failure.
 */
int sig_send_resume(pid_t pid);

/**
 * @brief Request termination of the worker process using SIGTERM.
 * 
 * Validates PID ownership, verifies target existence, and checks system call result.
 * 
 * @param pid Target worker PID.
 * @return int 0 on success, -1 on failure.
 */
int sig_send_terminate(pid_t pid);

#endif /* SIGNAL_HANDLER_H */
