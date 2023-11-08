/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2023.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/

/* become_daemon.c

   A function encapsulating the steps in becoming a daemon.
*/
#include <sys/stat.h>
#include <fcntl.h>
#include "become_daemon.h"
#include <unistd.h>
#include <stdlib.h>

int                                     /* Returns 0 on success, -1 on error */
becomeDaemon()
{
    int maxfd, fd;

    switch (fork()) {                   /* Become background process */
    case -1: return -1;
    case 0:  break;                     /* Child falls through... */
    default: _exit(EXIT_SUCCESS);       /* while parent terminates */
    }

    if (setsid() == -1)                 /* Become leader of new session */
        return -1;

    switch (fork()) {                   /* Ensure we are not session leader */
    case -1: return -1;
    case 0:  break;
    default: _exit(EXIT_SUCCESS);
    }


    
	close(STDIN_FILENO);            /* Reopen standard fd's to /dev/null */

	fd = open("/dev/null", O_RDWR);

	if (fd != STDIN_FILENO)         /* 'fd' should be 0 */
		return -1;
	if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
		return -1;
	if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
		return -1;
    

    return 0;
}
