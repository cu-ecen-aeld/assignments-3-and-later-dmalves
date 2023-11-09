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
#include "become_daemon.h"


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


  struct addrinfo hints;
  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  int yes = 1;
  char ip[INET6_ADDRSTRLEN];	// ip address
  int rv;			// error value
  struct sigaction sa;
  bool dflag = false;		// daemon flag
  
  openlog (NULL, 0, LOG_USER);
  
  syslog(LOG_DEBUG, "processing argv");

  // aesdsocket -d 
  if (argc > 1 && strcmp(argv[1], "-d") == 0) {
	dflag = true;
	syslog(LOG_DEBUG, "dflag is true");
  } else {
	  dflag = false;
	 syslog(LOG_DEBUG, "dflag is false");
  }
 


  memset (&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;;	// ipv4 only
  hints.ai_socktype = SOCK_STREAM;	// stream socket
  hints.ai_flags = AI_PASSIVE;	// my ip

   syslog(LOG_DEBUG, "processing getaddrinfo()");
  // get server address info
  if ((rv = getaddrinfo (NULL, PORT, &hints, &servinfo)) != 0)
    {
      perror ("getaddrinfo");
      exit(-1);
    }

  syslog(LOG_DEBUG, "processing socket()");
  if ((loop_sock = socket (servinfo->ai_family, servinfo->ai_socktype,
			   servinfo->ai_protocol)) == -1)
    {
      perror ("server: socket\n");
      exit(-1);
    }
  syslog(LOG_DEBUG, "processing setsockopt()");
  if (setsockopt (loop_sock, SOL_SOCKET, SO_REUSEADDR, &yes,
		  sizeof (int)) == -1)
    {
      perror ("setsockopt\n");
      exit (-1);
    }
    
  syslog(LOG_DEBUG, "processing bind()");
  if (bind (loop_sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
      close (loop_sock);
      perror ("server: bind");
      exit(-1);
    }

  syslog(LOG_DEBUG, "processing freeaddrinfo()");
  freeaddrinfo (servinfo);
 
 if(dflag)
    { 
	  syslog(LOG_DEBUG, "processing becomeDaemon()");

	  int result = becomeDaemon ();
	  syslog (LOG_INFO, "server: daemon monde\n");
	  if (result == -1)
	  {
        perror ("Can't become daemon!");
        exit(-1);
	  }
    }
  syslog(LOG_DEBUG, "processing listen()");
  if (listen (loop_sock, BACKLOG) == -1)
    {
      perror ("Server can't listen\n");
      exit (-1);
    }

  syslog(LOG_DEBUG, "processing sigaction()");

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

  
  while (true)
    {
	  syslog(LOG_DEBUG, "processing sigaction()");
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
	  exit(-1);
	}
      // close output file
      fclose (output_file);


      // send back file content
      fd = open (TMP_FILE, O_RDONLY);
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
