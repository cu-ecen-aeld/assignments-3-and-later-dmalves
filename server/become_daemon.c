#include <sys/stat.h>
#include <fcntl.h>
#include "become_daemon.h"
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>

int				/* Returns 0 on success, -1 on error */
becomeDaemon ()
{
  int fd;
  pid_t pid;

  pid = fork();

  if (pid < 0)
  {
	  syslog (LOG_ERR,"fork()");
	  return -1;
  }
  
  if (pid > 0) 
  {
	  syslog(LOG_INFO,"parent exiting...");
	  exit(0);
  }

  umask (0);

  if (setsid () == -1)
    {
      syslog (LOG_ERR, "setsid()");
      return -1;
    }
  if (chdir ("/") == -1)
    {
      syslog (LOG_ERR, "can not change to dir /");
      return -1;
    }

  close (STDIN_FILENO);
  close (STDOUT_FILENO);
  close (STDERR_FILENO);


  fd = open ("/dev/null", O_RDWR);

  if (fd == -1)
    {
      syslog (LOG_ERR, "can not open /dev/null");
      return -1;
    }

  if (dup2 (fd, STDIN_FILENO) == -1)
    {
      syslog (LOG_ERR, "stdin redirect failed");
      return -1;
    }

  if (dup2 (fd, STDOUT_FILENO) == -1)
    {
      syslog (LOG_ERR, "stdout redirect failed");
      return -1;
    }

  if (dup2 (fd, STDERR_FILENO) == -1)
    {
      syslog (LOG_ERR, "stderr redirect failed");
      return -1;
    }

  if (fd > 2) {
	  close(fd);
  }
  return 0;

}
