/**
 * @file signal.c
 * @brief rtas_errd signal handler
 *
 * Copyright (C) 2012 IBM Corporation
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "rtas_errd.h"

/**
 * sighup_handler
 * @brief signal handler for SIGHUP
 *
 * The SIGHUP signal will cause the rtas_errd daemon to re-read
 * the configuration file.  If it is currently safe to re-configure
 * ourselves we do, otherwise we set a flag to indicate that a
 * re-configuration needs to occur at the next "safe" place
 */
void
sighup_handler(int sig, siginfo_t siginfo, void *context)
{
	if (d_cfg.flags & RE_CFG_RECFG_SAFE)
		diag_cfg(1, &cfg_log);
	else
		d_cfg.flags |= RE_CFG_RECEIVED_SIGHUP;
}

/**
 * sigchld_handler
 * @brief SIGCHLD handler.
 *
 * Cleanup child process when it exited.
 */
static void
sigchld_handler(int sig)
{
	/* Wait for all dead processes.
	 * We use a non-blocking call to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program.
	 */
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
}

/**
 * restore_sigchld_default
 * @brief restore child signal handler
 *
 * Restores child signal handler to default action.
 */
void
restore_sigchld_default(void)
{
	struct sigaction old_sigact;
	struct sigaction sigact;

	sigaction(SIGCHLD, NULL, &old_sigact);
	if (old_sigact.sa_handler != SIG_DFL ) {
		sigact.sa_handler = SIG_DFL;
		sigemptyset(&sigact.sa_mask);
		sigact.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sigact, NULL)) {
			log_msg(NULL, "Could not restore SIGCHLD signal "
				"handler to default action, %s",
				strerror(errno));
		}
	}
}

/**
 * setup_sigchld_handler
 * @brief setup child signal handler
 *
 * Setup custom child signal handler to cleanup
 * child processes when it exited.
 */
void
setup_sigchld_handler(void)
{
	struct sigaction sigact;
	sigact.sa_handler = (void *)sigchld_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sigact, NULL)) {
		log_msg(NULL, "Could not initialize signal handler for"
			"cleaning up child processes, %s", strerror(errno));
	}
}
