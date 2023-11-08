#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <stdbool.h>
#include <fcntl.h>



#define PORT "9000"
#define TMP_FILE "/var/tmp/aesdsocketdata"

#define BACKLOG 10
#define MEMSIZE 1024*1024

void *get_in_addr (struct sockaddr *sa);

void signal_handler (int s);

size_t read_line (int client_sock, char *buffer, size_t buffer_size);


// Global variables: must be closed or freed on signal handler

int loop_sock, client_sock;
FILE *output_file;
char *buffer;
struct addrinfo *servinfo;
int fd;

int
main (int argc, char **argv)
{


  struct addrinfo hints, *p;
  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  int yes = 1;
  char ip[INET6_ADDRSTRLEN];	// ip address
  int rv;			// error value
  struct sigaction sa;
  bool dflag = false;
  char c;
  pid_t pid;
  char *PIDFILE;

  openlog (NULL, 0, LOG_USER);

  // aesdsocket -d $PIDFILE
  while ((c = getopt (argc, argv, "d:")) != -1)
    switch (c)
      {
      case 'd':
	dflag = true;
	PIDFILE = optarg;
	break;

      default:
	dflag = false;
      }


  memset (&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;	// ipv4 or ipv6
  hints.ai_socktype = SOCK_STREAM;	// stream socket
  hints.ai_flags = AI_PASSIVE;	// my ip

  // get server address info
  if ((rv = getaddrinfo (NULL, PORT, &hints, &servinfo)) != 0)
    {
      syslog (LOG_INFO, "getaddrinfo: %s\n", gai_strerror (rv));
      return -1;
    }

  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next)
    {
      if ((loop_sock = socket (p->ai_family, p->ai_socktype,
			       p->ai_protocol)) == -1)
	{
	  syslog (LOG_ERR, "server: socket\n");
	  continue;
	}

      if (setsockopt (loop_sock, SOL_SOCKET, SO_REUSEADDR, &yes,
		      sizeof (int)) == -1)
	{
	  syslog (LOG_ERR, "setsockopt\n");
	  exit (-1);
	}

      if (bind (loop_sock, p->ai_addr, p->ai_addrlen) == -1)
	{
	  close (loop_sock);
	  syslog (LOG_ERR, "server: bind");
	  continue;
	}

      break;
    }

  freeaddrinfo (servinfo);

  if (p == NULL)
    {
      syslog (LOG_ERR, "server: failed to bind\n");
      exit (-1);
    }

  if (listen (loop_sock, BACKLOG) == -1)
    {
      syslog (LOG_ERR, "Server can't listen\n");
      exit (-1);
    }

  // register signal handler

  sa.sa_handler = signal_handler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction (SIGINT, &sa, NULL) == -1)
    {
      perror ("sigaction");
      exit (1);
    }
  if (sigaction (SIGTERM, &sa, NULL) == -1)
    {
      perror ("sigaction");
      exit (1);
    }

  syslog (LOG_INFO, "server: waiting for connections...\n");

  if (dflag)
    {
      // became daemon
      if (daemon (0, 0) == -1)
	{
	  syslog (LOG_ERR, "can't become daemon: %s\n", strerror (errno));
	}
      else
	{
	  pid = getpid();
	  FILE *pidfile = fopen(PIDFILE,"w");
	  fprintf(pidfile,"%d", pid);
	  fclose(pidfile);
	  syslog (LOG_INFO, "server: I am a daemon ...\n");
	}
    }
  while (true)
    {
      sin_size = sizeof client_addr;
      client_sock =
	accept (loop_sock, (struct sockaddr *) &client_addr, &sin_size);
      if (client_sock == -1)
	{
	  perror ("accept");
	  exit (-1);
	}

      inet_ntop (client_addr.ss_family,
		 get_in_addr ((struct sockaddr *) &client_addr),
		 ip, sizeof ip);
      syslog (LOG_INFO, "Accepted connection from %s\n", ip);

      // open output file
      output_file = fopen (TMP_FILE, "a");
      if (output_file == NULL)
	{
	  syslog (LOG_ERR, "fopen: %s", strerror (errno));
	  exit (-1);
	}

      buffer = (char *) malloc (MEMSIZE * sizeof (char));
      if (buffer != NULL)
	{
	  //while((rv = read_line(client_sock, buffer, 1024)) > 0){
	  //fprintf(output_file,"%s",buffer);
	  //}
	  rv = read_line (client_sock, buffer, MEMSIZE);
	  fprintf (output_file, "%s", buffer);
	  if (rv < 0)
	    {
	      syslog (LOG_ERR, "Error on read_line\n");
	    }
	}
      else
	{
	  syslog (LOG_ERR, "Error on malloc\n");
	}
      // close output file
      fclose (output_file);


      // send back file content
      fd = open (TMP_FILE, O_RDONLY);
      int total = 0;
      if (fd < 0)
	{
	  perror ("cant open file back");
	}
      while ((rv = read_line (fd, buffer, MEMSIZE)) > 0)
	{
	  send (client_sock, buffer, rv, 0);
	  printf ("%s", buffer);
	}
      if (rv < 0)
	{
	  syslog (LOG_ERR, "Error on read_line\n");
	}
      close (fd);


      syslog (LOG_INFO, "Closed connection from %s\n", ip);
      close (client_sock);
      free (buffer);

    }

  return 0;
}


void
signal_handler (int s)
{
  // close connections
  close (client_sock);
  close (loop_sock);

  // delete file
  if (unlink (TMP_FILE))
    {
      syslog (LOG_ERR, "Could not delete %s: %s", TMP_FILE, strerror (errno));
    }


  syslog (LOG_INFO, "Caught signal, exiting\n");
  closelog ();

  exit (0);
}

size_t
read_line (int client_sock, char *buffer, size_t buffer_size)
{
  char c;
  int numbytes, total_bytes = 0;

  while (true)
    {
      numbytes = read (client_sock, &c, 1);
      if (numbytes == -1)
	{
	  if (errno == EINTR)	/* Interrupted --> restart read() */
	    continue;
	  else
	    return -1;		/* unkown error */
	}
      else if (numbytes == 0)
	{			/* EOF -  closed connection */
	  if (total_bytes == 0)	/* No bytes read; return 0 */
	    return 0;
	  else			/* Some bytes read; add '\0' */
	    break;

	}
      else
	{

	  if (total_bytes < buffer_size - 1)
	    {			// last byte must be '\0', 
	      buffer[total_bytes] = c;	// discard overflow input 
	      total_bytes++;
	    }

	  if (c == '\n')
	    break;

	}
    }

  buffer[total_bytes] = '\0';
  return total_bytes;
}

// get sockaddr, IPv4 or IPv6:
void *
get_in_addr (struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
    {
      return &(((struct sockaddr_in *) sa)->sin_addr);
    }

  return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}
