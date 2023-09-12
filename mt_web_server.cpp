//====================================================== file = weblite.c =====
//=  A super light weight HTTP server                                         =
//=============================================================================
//=  Notes:                                                                   =
//=    1) Compiles for Winsock only since uses Windows threads. Generates     =
//=       one warning about unreachable code in main.  Ignore this warning.   =
//=    2) Serves HTML and GIF only.                                           =
//=    3) Sometimes the browser drops a connection when doing a refresh.      =
//=       This is handled by checking the recv() return code in the function  =
//=       that handles GETs.                                                  =
//=    4) The 404 HTML message does not always display in Explorer.           =
//=---------------------------------------------------------------------------=
//=  Execution notes:                                                         =
//=   1) Execute this program in the directory which will be the root for     =
//=      all file references (i.e., the directory that is considered at       =
//=      "public.html").                                                      =
//=   2) Open a Web browser and surf to http://xxx.xxx.xxx.xxx/yyy where      =
//=      xxx.xxx.xxx.xxx is the IP address or hostname of the machine that    =
//=      weblite is executing on and yyy is the requested object.             =
//=   3) The only output (to stdout) from weblite is a message with the       =
//=      of the file currently being sent                                     =
//=---------------------------------------------------------------------------=
//=  Build: bcc32 weblite.c, cl weblite.c wsock32.lib (or ws2_32.lib)         =
//=         gcc weblite.c -lsocket -lnsl for BSD                              =
//=---------------------------------------------------------------------------=
//=  Execute: weblite                                                         =
//=---------------------------------------------------------------------------=
//=  History:  KJC (12/29/00) - Genesis (from server.c)                       =
//=            HF (01/25/01) - Ported to multi-platform environment           =
//=============================================================================
#define WIN    // WIN for Windows environment, UNIX for BSD or LINUX env.    

//----- Include files ---------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <stdlib.h>         // Needed for exit()
#include <string.h>         // Needed for strcpy() and strlen()
#include <fcntl.h>          // Needed for file i/o constants
#include <sys/stat.h>       // Needed for file i/o constants  

/* FOR BSD UNIX/LINUX  ---------------------------------------------------- */
#ifdef UNIX 
#include <sys/types.h>      //   
#include <netinet/in.h>     //   
#include <sys/socket.h>     // 
#include <arpa/inet.h>      //   
#endif
/* ------------------------------------------------------------------------ */

/* FOR WIN ---------------------------------------------------------------- */
#ifdef WIN
#include <stddef.h>         // Needed for _threadid
#include <process.h>        // Needed for _beginthread() and _endthread()
#include <io.h>             // Needed for open(), close(), and eof()
#include <windows.h>        // Needed for all Winsock stuff
#endif
/* ------------------------------------------------------------------------ */

//----- HTTP response messages ----------------------------------------------
#define OK_IMAGE    "HTTP/1.0 200 OK\nContent-Type:image/gif\n\n"
#define OK_TEXT     "HTTP/1.0 200 OK\nContent-Type:text/html\n\n"
#define NOTOK_404   "HTTP/1.0 404 Not Found\nContent-Type:text/html\n\n"
#define MESS_404    "<html><body><h1>FILE NOT FOUND</h1></body></html>"

//----- Defines -------------------------------------------------------------
#define BUF_SIZE            4096     // Buffer size (big enough for a GET)
#define PORT_NUM            9080     // Port number for a Web server

//----- Function prototypes -------------------------------------------------
/* FOR WIN --------------------------------------------------------------- */
#ifdef WIN
void handle_get(void *in_arg);       // Thread function to handle GET
#endif
/* ----------------------------------------------------------------------- */

/* FOR UNIX/LINUX -------------------------------------------------------- */
#ifdef UNIX
void child_proc(int server_s, int client_s); // Fork function for GET
#endif
/* ----------------------------------------------------------------------- */

//===== modeule main ========================================================
int main(void)
{
    /* FOR WIN ------------------------------------------------------------- */
#ifdef WIN
	WORD wVersionRequested = MAKEWORD(1, 1);   // Stuff for WSA functions
	WSADATA wsaData;                           // Stuff for WSA functions
#endif
    /* --------------------------------------------------------------------- */

	unsigned int         server_s;             // Server socket descriptor
	struct sockaddr_in   server_addr;          // Server Internet address
	unsigned int         client_s;             // Client socket descriptor
	struct sockaddr_in   client_addr;          // Client Internet address
	struct in_addr       client_ip_addr;       // Client IP address
	int                  addr_len;             // Internet address length

    /* FOR UNIX/LINUX ------------------------------------------------------ */
#ifdef UNIX
	int                  nChild_proc_id;       // New child process ID.
#endif
    /* --------------------------------------------------------------------- */

    /* FOR WIN ------------------------------------------------------------- */
#ifdef WIN
											   // Initialize winsock
	WSAStartup(wVersionRequested, &wsaData);
#endif
	/* --------------------------------------------------------------------- */

	// Create a socket, fill-in address information, and then bind it
	server_s = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));

	// Main loop to listen, accept, and then spin-off a thread to handle the GET
	while (1)
	{
		// Listen for connections and then accept
		listen(server_s, 100);
		addr_len = sizeof(client_addr);

		client_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len);
		if (client_s == 0)
		{
			printf("ERROR - Unable to create socket \n");
			exit(1);
		}

		/* FOR UNIX/LINUX ---------------------------------------------------- */
#ifdef UNIX
		// Spin-off a child process by fork
		nChild_proc_id = fork();

		// Separate the parent and child process here ...
		if (nChild_proc_id == -1) // if I am a new child, go to the child module.
		{
			child_proc(server_s, client_s);
		}
#endif
		/* ------------------------------------------------------------------- */

		/* FOR WIN ----------------------------------------------------------- */
#ifdef WIN
		// Spin-off a thread to handle this request (pass only client_s)
		if (_beginthread(handle_get, 4096, (void *)client_s) < 0)
		{
			printf("ERROR - Unable to create thread \n");
			exit(1);
		}
#endif
		/* ------------------------------------------------------------------- */
	}

	/* FOR UNIX/LINUX ------------------------------------------------------ */
#ifdef UNIX
	// Close the server socket
	close(server_s);
#endif
	/* --------------------------------------------------------------------- */

	/* FOR WIN ------------------------------------------------------------- */
#ifdef WIN
	// Close the server socket and clean-up winsock
	printf("this web server is shutting down .....\a\n");

	closesocket(server_s);
	WSACleanup();
#endif
	/* --------------------------------------------------------------------- */

	// To make sure this "main" returns an integer.
	return (1);
}

/* FOR WIN --------------------------------------------------------------- */
#ifdef WIN
//===========================================================================
//=  This is is the thread function to handle the GET                       =
//=   - It is assumed that the request is a GET                             =
//===========================================================================
void handle_get(void *in_arg)
{
	unsigned int   client_s;             // Client socket descriptor
	char           in_buf[BUF_SIZE];     // Input buffer for GET resquest
	char           out_buf[BUF_SIZE];    // Output buffer for HTML response
	char           *file_name;           // File name
	unsigned int   fh;                   // File handle
	unsigned int   buf_len;              // Buffer length for file reads
	unsigned int   retcode;              // Return code

										 // Set client_s to in_arg
	client_s = (unsigned int)in_arg;

	// Receive the GET request from the Web browser
	retcode = recv(client_s, in_buf, BUF_SIZE, 0);

	// Handle the GET if there is one (see note #3 in the header)
	if (retcode != -1)
	{
		// Parse out the filename from the GET request
		strtok(in_buf, " ");
		file_name = strtok(NULL, " ");

		// Open the requested file
		//  - Start at 2nd char to get rid of leading "\"
		fh = open(&file_name[1], O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);

		// Generate and send the response (404 if could not open the file)
		if (fh == -1)
		{
			printf("File %s not found - sending an HTTP 404 \n", &file_name[1]);
			strcpy(out_buf, NOTOK_404);
			send(client_s, out_buf, strlen(out_buf), 0);
			strcpy(out_buf, MESS_404);
			send(client_s, out_buf, strlen(out_buf), 0);
		}
		else
		{
			printf("File %s is being sent \n", &file_name[1]);
			if (strstr(file_name, ".gif") != NULL)
				strcpy(out_buf, OK_IMAGE);
			else
				strcpy(out_buf, OK_TEXT);
			send(client_s, out_buf, strlen(out_buf), 0);
			while (!eof(fh))
			{
				buf_len = read(fh, out_buf, BUF_SIZE);
				send(client_s, out_buf, buf_len, 0);
			}
			close(fh);
		}
	}

	// Close the client socket and end the thread
	closesocket(client_s);
	_endthread();
}
#endif
/* ----------------------------------------------------------------------- */

/* FOR UNIX/LINUX -------------------------------------------------------- */
#ifdef UNIX
void child_proc(int server_s, int client_s)
{
	char           in_buf[BUF_SIZE];     // Input buffer for GET resquest
	char           out_buf[BUF_SIZE];    // Output buffer for HTML response
	char           *file_name;           // File name
	unsigned int   fh;                   // File handle
	unsigned int   buf_len;              // Buffer length for file reads
	unsigned int   retcode;              // Return code

										 // Shut down the parent pipe
	close(server_s);

	// Receive the GET request from the Web browser
	retcode = recv(client_s, in_buf, BUF_SIZE, 0);

	// Handle the GET if there is one (see note #3 in the header)
	if (retcode != -1)
	{
		// Parse out the filename from the GET request
		strtok(in_buf, " ");
		file_name = strtok(NULL, " ");

		// Open the requested file
		//  - Start at 2nd char to get rid of leading "\"
		// fh = open(&file_name[1], O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
		fh = open(&file_name[1], O_RDONLY, S_IREAD | S_IWRITE);

		// Generate and send the response (404 if could not open the file)
		if (fh == -1)
		{
			printf("File %s not found - sending an HTTP 404 \n", &file_name[1]);
			strcpy(out_buf, NOTOK_404);
			send(client_s, out_buf, strlen(out_buf), 0);
			strcpy(out_buf, MESS_404);
			send(client_s, out_buf, strlen(out_buf), 0);
		}
		else
		{
			printf("File %s is being sent \n", &file_name[1]);
			if (strstr(file_name, ".gif") != NULL)
				strcpy(out_buf, OK_IMAGE);
			else
				strcpy(out_buf, OK_TEXT);
			send(client_s, out_buf, strlen(out_buf), 0);

			while (fh != EOF)
			{
				buf_len = read(fh, out_buf, BUF_SIZE);
				send(client_s, out_buf, buf_len, 0);
			}
			close(fh);
		}
	}

	// Shut down my (the child) pipe
	close(client_s);
}
#endif
/* ----------------------------------------------------------------------- */