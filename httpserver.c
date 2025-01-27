#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "./logger_module.h"

#define BUF_SIZE 100
#define BACKLOG 100
#define RES_SIZE 1000000

int current_log_level;

/* Handler for signals established using SA_SIGINFO */
static void
siginfoHandler (int sig, siginfo_t * si, void *ucontext)
{

  current_log_level = si->si_value.sival_int;

}

void
send_directory_listing (DIR * dir, int client_socket,
			const char *directory_path)
{
  char response[RES_SIZE];
  snprintf (response, sizeof (response),
	    "HTTP/1.1 200 OK\r\n"
	    "Content-Type: text/html\r\n"
	    "\r\n"
	    "<html><body><h1>Directory Listing of: %s</h1><ul>",
	    directory_path);
  // Add directory entries
  if (dir == NULL)
    {
      printf ("OPEN: open directory failed (%s)\n%s", strerror (errno),
	      directory_path);
      exit (1);
    }
  struct dirent *entry;
  while ((entry = readdir (dir)) != NULL)
    {
      strcat (response, "<li><a href=\"");
      strcat (response, entry->d_name);
      strcat (response, "\">");
      strcat (response, entry->d_name);
      strcat (response, "</a></li>");
    }

  closedir (dir);

  strcat (response, "</ul></body></html>");
  write (client_socket, response, strlen (response));
}

void
cat_file (int fd, int client_socket, char *file_path)
{
  char response[RES_SIZE];
  char buf[BUF_SIZE];
  snprintf (response, sizeof (response),
	    "HTTP/1.1 200 OK\r\n"
	    "Content-Type: text/html\r\n"
	    "\r\n" "<html><body><h1>File content of: %s</h1><ul>", file_path);

  while (read (fd, buf, BUF_SIZE) > 0)
    {
      strcat (response, "<p>");
      strcat (response, buf);
    }
  strcat (response, "</p>");

  close (fd);

  strcat (response, "</ul></body></html>");
  write (client_socket, response, strlen (response));
}

void
send_error (int cfd, int status_code, const char *reason, const char *msg)
{
  char response[RES_SIZE];
  snprintf (response, RES_SIZE,
	    "HTTP/1.1 %d %s\r\n"
	    "Content-Type: text/html\r\n"
	    "Connection: close\r\n"
	    "\r\n"
	    "<html>"
	    "<head><title>%d %s</title></head>"
	    "<body><h1>%d %s</h1><p>%s</p></body>"
	    "</html>", status_code, reason, status_code, reason, status_code,
	    reason, msg);

  write (cfd, response, strlen (response));
}

void
run_cgi_script (int cfd, const char *path, const char *query)
{
  pid_t pid;
  int pipefd[2];
  char script_path[BUF_SIZE];
  snprintf (script_path, BUF_SIZE, ".%s", path);	// Map URL to local script path
  int outfd = dup (STDOUT_FILENO);
  if (access (script_path, X_OK) != 0)
    {
      log_error("CGI script not found or not executable.");   
      send_error (cfd, 404, "Not Found",
		  "CGI script not found or not executable.");
      return;
    }

  if (pipe (pipefd) == -1)
    {
      perror ("pipe");
      exit (1);
    }

  if ((pid = fork ()) == 0)
    {				// Child process
      close (pipefd[0]);	// Close read end of pipe
      dup2 (pipefd[1], STDOUT_FILENO);	// Redirect script output to pipe
      close (pipefd[1]);

      if (!query)
	{
	  query = "";		// Assign an empty string as default
	}

      // Set CGI environment variables
      setenv ("REQUEST_METHOD", "GET", 1);
      setenv ("QUERY_STRING", query, 1);	// Example query string
      setenv ("SCRIPT_NAME", path, 1);
      setenv ("SERVER_PROTOCOL", "HTTP/1.1", 1);

      // Execute the script
      execl (script_path, script_path, NULL);

      // If exec fails
      perror ("execl");
      exit (1);
    }
  else if (pid > 0)
    {				// Parent process
      close (pipefd[1]);	// Close write end of pipe

      char buffer[BUF_SIZE];
      ssize_t num_read;
      while ((num_read = read (pipefd[0], buffer, BUF_SIZE)) > 0)
	{
	  write (cfd, buffer, num_read);	// Send script output to client
	}

      close (pipefd[0]);
      int status = 0;
      int childPid = wait (&status);
      dup2 (STDOUT_FILENO, outfd);	// Redirect script output to pipe
      close (outfd);

    }
  else
    {
      perror ("fork");
      send_error (cfd, 500, "Internal Server Error",
		  "Failed to execute CGI script.");
    }
}

int
main (int argc, char *argv[])
{

  struct sigaction sa;


/* Establish handler for the real signal. */
  sa.sa_sigaction = siginfoHandler;
  sa.sa_flags = SA_SIGINFO;

  sigaction (SIGRTMIN, &sa, NULL);

  pause();
  struct sockaddr_in addr;
  int server_fd, cfd;
  ssize_t numRead;
  char buf[BUF_SIZE];
  char *cpbuf = NULL;
  char *request;
  char *path;
  char *query;
  DIR *dir = NULL;
  int file_fd = 0;
  int fork_fd;
  int opt = 1;

  log_debug("signal handled and server program started");
  if (argc < 2 || strcmp (argv[1], "--help") == 0)
    {
      log_error("Wrong usage");
      printf ("Usage: %s port\n", argv[0]);
      exit (1);
    }
  // Creating socket file descriptor
  log_debug ("Creating socket file descriptor");
  if ((server_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf ("SERVER: socket create failed (%s)\n", strerror (errno));
      exit (EXIT_FAILURE);
    }

  log_info ("Socket fd created successfully");
#if 1
  // Forcefully attaching socket to the port
  log_debug ("Attaching socket to the port");
  if (setsockopt (server_fd, SOL_SOCKET,
		  SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof (opt)))
    {
      perror ("setsockopt");
      exit (EXIT_FAILURE);
    }
#endif

  log_info ("port attached successfully");
  /* Construct server socket address, bind socket to it,
     and make this a listening socket */
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (atoi (argv[1]));

  if (bind (server_fd, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
      printf ("SERVER: bind failed (%s)\n", strerror (errno));
      exit (1);
    }

  if (listen (server_fd, BACKLOG) == -1)
    {
      printf ("SERVER: listen failed (%s)\n", strerror (errno));
      exit (1);
    }

  for (;;)
    {				/* Handle client connections iteratively */

      /* Accept a connection. The connection is returned on a new
         socket, 'cfd'; the listening socket ('server_fd') remains open
         and can be used to accept further connections. */

      cfd = accept (server_fd, NULL, NULL);

      fork_fd = fork ();
      
      if (fork_fd == 0)
	{
	  log_debug("forked after acceptance");
	  //close(server_fd);
	  cpbuf = NULL;
	  memset (buf, 0, BUF_SIZE);
	  if (cfd == -1)
	    {
	      printf ("SERVER: accept failed (%s)\n", strerror (errno));
	      exit (1);
	    }
	 
	 log_info("server accepted request");
	
	 if ((numRead = read (cfd, buf, BUF_SIZE)) == 0)
	 	log_warning("server accepted request with empty buffer");
	 else{
	 	do
	 	{
	 		if (strncmp (buf, "GET ", 4) == 0)
			cpbuf = strdup (buf);
		 }while((numRead = read (cfd, buf, BUF_SIZE)) == BUF_SIZE);
	 }
	  

	  if (cpbuf == NULL)
	    send_error (cfd, 405, "Method Not supported",
			"Only GET method is supported.");
	  else
	    {
	      strtok (cpbuf, " ");
	      request = strtok (NULL, " ");
	      //write (STDOUT_FILENO, request, strlen (request));
	      log_info(request);
	      path = strtok (request, "?");
	      query = strtok (NULL, "?");
	      if ((dir = opendir (request)) != NULL)
		send_directory_listing (dir, cfd, request);
	      else if ((file_fd = open (request, O_RDONLY)) != -1)
		cat_file (file_fd, cfd, request);
	      else if (strncmp (request, "/cgi-bin/", 9) == 0)
		run_cgi_script (cfd, request, query);
	      else{
	      	log_error("Invalid request");
		send_error (cfd, 404, "Not Found",
			    "The requested resource was not found on this server.");
			    }
	    }
	
	  if (close (cfd) == -1)
	    {
	      printf ("SERVER: close failed (%s)\n", strerror (errno));
	      exit (1);
	    }
	  log_debug("Closing a connection with one client (reached getchar() line)");
	  getchar();
	}
      else if (fork_fd > 0)
	{
	  close (cfd);
	}
      else
	{
	  perror ("Fork failed");
	  close (cfd);
	}
    }
  log_debug("closing the server socket");
  close (server_fd);		// Close the listening socket
  return 0;
}
