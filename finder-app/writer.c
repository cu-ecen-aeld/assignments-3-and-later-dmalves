#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  int fd;
  
  openlog (NULL, 0, LOG_USER);

  if (argc != 3)
    {
      syslog (LOG_ERR, "Invalid Number of arguments: %d", argc);
      return 1;
    }

  fd = open (argv[1], O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR);

  syslog (LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);

  dprintf (fd, "%s\n", argv[2]);
  
  
  close(fd);

  closelog ();

  return 0;

}
