#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define HTTP_REQUEST_MAX_SIZE 4096
#define HOSTNAME_MAX_SIZE 512
#define PORT_MAX_SIZE 6
#define URI_MAX_SIZE 4096
#define METHOD_SIZE 32
#define MAX_TOTAL_BYTES_READ 16384

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection = "Connection: close\r\n";
static const char *proxyConnection = "Proxy-Connection: close\r\n";

/* 
 * Determine whether or not the HTTP request contained in the request argument
 * is complete in the sense that the request has been received in its entirety.
 * Hint: this can be as simple as seeing if the request ends with the
 * end-of-headers character sequence.
 *
 * Input: request, a string containing an HTTP request.  Note that the way this
 *        function is currently defined, request must be a null-terminated
 *        string. As such, you can use string functions (strcp(), strstr(),
 *        strlen()) on request.  Alternatively (i.e., not null-terminated), you
 *        would need to modify the function definition to additionally pass in
 *        a length.
 * Output: 1 if request is a complete HTTP request, 0 otherwise
 * */
int is_complete_request(const char *request) {
    int lastIndex = strlen(request) - 1;
    if(request[lastIndex] == '\n' && request[lastIndex - 2] == '\n' && request[lastIndex - 1] == '\r' && request[lastIndex - 3] == '\r') {
           return 1;
    }
	return 0;
}

/* Parse an HTTP request, and copy each parsed value into the
 * appropriate string argument as a null-terminated string (remember the null
 * terminator!). Call is_complete_request() first to make sure the client has
 * sent the full request.
 * Input: method, hostname, port, uri - strings to which the
 *        corresponding parts parsed from the request should be
 *        copied, as strings.  The uri is the "path" part of the requested URL,
 *        including query string. If no port is specified, the port string
 *        should be populated with a string specifying the default HTTP port,
 *        i.e., "80".
 * Output: 1 if request is a complete HTTP request, 0 otherwise
 * */
int parse_request(const char *request, char *method,
		char *hostname, char *port, char *uri) {
	
    if(is_complete_request(request)) {
        int i = 0;

        // Get the Request Method
        const char* POST = "POST";
        const char* GET = "GET";
        if(strstr(request, POST) != NULL || strstr(request, GET) != NULL) { // It has a method
            while(request[i] != ' ') {
                method[i] = request[i];
                i++;
            }
            method[i] = '\0'; // Add the null terminator
        } 

        // Get the Port and URI
        i = 0;
        const char* PORT_IN_URL = ".com:";
        if(strstr(request, PORT_IN_URL) != NULL) {
            // Get the port
            while(1) { // Port starts at the pointer + 5
                if(request[i] == '.' && request[i + 1] == 'c' && request[i + 2] == 'o' && request[i + 3] == 'm' && request[i + 4] == ':') {
                    i += 5;
                    int j = 0;
                    while(j < 4) {
                        port[j] = request[i];
                        j++;
                        i++;
                    }
                    port[j] = '\0'; // Add the null terminator
                    break;
                }
                i++;
            }
            // Ge the URI
            int j = 0;
            while(request[i] != ' ') {
                uri[j] = request[i];
                i++;
                j++;
            }
            uri[j] = '\0'; // Add the null terminator
        } else {
            port[0] = '8';
            port[1] = '0';
            port[2] = '\0';
            
            // Ge the URI
            int j = 0;
            i = 0;
            while(1) {
                if(request[i] == '/' && request[i - 1] == 'm' && request[i - 2] == 'o' && request[i - 3] == 'c' && request[i - 4] == '.') {
                    while(request[i] != ' ') {
                        uri[j] = request[i];
                        i++;
                        j++;
                    }
                    uri[j] = '\0'; // Add the null terminator
                    break;
                }
                i++;
            }
            uri[j] = '\0'; // Add the null terminator
        }

        // Get the hostname
        const char* HOST = "Host: ";
        i = 0;
        if(strstr(request, HOST) != NULL) {
            while(1) { 
                if(request[i] == 'H' && request[i + 1] == 'o' && request[i + 2] == 's' && request[i + 3] == 't' && request[i + 4] == ':' && request[i + 5] == ' ') {
                    i += 6; // Port starts at the pointer + 6
                    int j = 0;
                    while(request[i] != '/' && request[i] != ':' && request[i] != '\r') {
                        hostname[j] = request[i];
                        j++;
                        i++;
                    }
                    hostname[j] = '\0'; // Add the null terminator
                    break;
                }
                i++;
            }
        }
        return 1;
    } else {
        return 0;
    }
}

int main(int argc, char *argv[]) {

    signal(SIGPIPE, SIG_IGN);   // The proxy must ignore SIGPIPE signals

	struct sockaddr_in address;
    char* cmdLinePort = argv[1];
    int socketFd;

    address.sin_family = AF_INET; // Set it to IPV4 for like...ever.
    address.sin_port = htons(atoi(cmdLinePort)); // Convert our port to a network order integer
    address.sin_addr.s_addr = INADDR_ANY; // We are open to anything.

    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}

	if (bind(socketFd, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind");
		exit(EXIT_FAILURE);
	}

    /////////////////////////////////////////////////////////
    // Listen for incoming connections on a port whose number will be specified
    // on the command line.
    /////////////////////////////////////////////////////////
    int BACKLOG_VALUE = 100;
    if (listen(socketFd, BACKLOG_VALUE) < 0) {
		perror("Error listening to socket");
		exit(EXIT_FAILURE);
	}

    /////////////////////////////////////////////////////////
    // Once a connection is established, your proxy should read the entirety of the request
    // from the client and parse the request.
    /////////////////////////////////////////////////////////
    int clientFd;

    while(1) {

        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len;

        if ((clientFd = accept(socketFd, (struct sockaddr *)&peer_addr, &peer_addr_len)) < 0) {
            perror("Error accepting the socket");
            exit(EXIT_FAILURE);
        }

        // Read the entirety of the request on the port.
        int totalBytesRead = 0;
        int numberOfBytesRead;
        char clientRequest[MAX_OBJECT_SIZE];
        memset(clientRequest, 0, MAX_OBJECT_SIZE); // Initialize the clientRequest to be a ton of zeros.
        for(;;) {
            numberOfBytesRead = read(clientFd, clientRequest + totalBytesRead, MAX_OBJECT_SIZE); // Write to the clientRequest buffer
            totalBytesRead += numberOfBytesRead;
            if(numberOfBytesRead == 0) { // This is an indicator that the server has closed 
                                            // its end of the connection and is effectively EOF.
                break;
            }
            if(numberOfBytesRead < 4) { /* Ignore failed request */
                continue;
            }
            // If we have read to the end of the request.
            if(!memcmp(clientRequest + totalBytesRead - 4, "\r\n\r\n", 4)) {
                break;
            }
        }
        // We are done with reading the request. 

        /////////////////////////////////////////////////////////
        // Parse the client request.
        /////////////////////////////////////////////////////////
        char method[METHOD_SIZE];
        char hostname[HOSTNAME_MAX_SIZE];
        char port[PORT_MAX_SIZE];
        char uri[URI_MAX_SIZE];
        if(!parse_request(clientRequest, method, hostname, port, uri)) {
            sleep(0); // Bad request. Idk what to do hehee.
        }

        /////////////////////////////////////////////////////////
        // Build the request to forward
        /////////////////////////////////////////////////////////
        char proxyRequest[HTTP_REQUEST_MAX_SIZE];

        // Add the Host to proxyRequest header
        strcpy(proxyRequest + strlen(proxyRequest), "Host: ");
        strcpy(proxyRequest + strlen(proxyRequest), hostname);
        strcpy(proxyRequest + strlen(proxyRequest), "\r\n");

        // Add the User-Agent to proxyRequest header
        strcpy(proxyRequest + strlen(proxyRequest), user_agent_hdr);

        // Add Connection to proxyRequest header
        strcpy(proxyRequest + strlen(proxyRequest), connection);

        // Add Proxy-Connection to proxyRequest header
        strcpy(proxyRequest + strlen(proxyRequest), proxyConnection);

        // Add any additional request headers as part of the received HTTP request
        char* additionalHeaderItem;
        while((additionalHeaderItem = strtok(clientRequest, "\n")) != NULL) { // strtok returns a Null pointer when it's done.
            if(strstr(additionalHeaderItem, connection) == 0 || strstr(additionalHeaderItem, proxyConnection) == 0) {
                continue; // Don't add these headers. We've already added them.
            } else {
                strcpy(proxyRequest + strlen(proxyRequest), additionalHeaderItem);
                strcpy(proxyRequest + strlen(proxyRequest), "\n"); // Add a \n because we delimited by \n.
            }
        }

        // Add the final carraige return & newline characters to terminate the proxyRequest
        strcpy(proxyRequest + strlen(proxyRequest), "\r\n"); // Add a \n because we delimited by \n.

        /////////////////////////////////////////////////////////
        // Forward the request to the server
        /////////////////////////////////////////////////////////
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int serverFd, s;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC; // IPV4 or IPV6
        hints.ai_socktype = SOCK_STREAM; // TCP, not UDP
        hints.ai_flags = 0;
        hints.ai_protocol = 0; // Any protocol

        fprintf(stdout, "Here is my host: %s\n", hostname);
        fprintf(stdout, "Here is my port: %s\n", port);
        
        s = getaddrinfo(hostname, port, &hints, &result);
        if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
            exit(EXIT_FAILURE);
        }

        /* getaddrinfo() returns a list of address structures.
        Try each address until we successfully connect(2).
        If socket(2) (or connect(2)) fails, we (close the socket
        and) try the next address. */

        for (rp = result; rp != NULL; rp = rp->ai_next) {
            serverFd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
            if (serverFd == -1)
                continue;

            if (connect(serverFd, rp->ai_addr, rp->ai_addrlen) != -1)
                break;                  /* Success */

            close(serverFd);
        }

        if (rp == NULL) {               /* No address succeeded */
            fprintf(stderr, "Could not connect\n");
            exit(EXIT_FAILURE);
        }

        freeaddrinfo(result);           /* No longer needed */

        // Forward the request to the server
        char forwardRequestBuffer[HTTP_REQUEST_MAX_SIZE];
        int numBytesToWrite = strlen(proxyRequest);
        int bytesWritten = 0;
        int temp;
        while((temp = write(serverFd, forwardRequestBuffer + bytesWritten, (numBytesToWrite - bytesWritten))) > 0) {
            bytesWritten += temp;
            if(bytesWritten >= numBytesToWrite) {
                break;
            }
        }

        // Receive the response from the server and put the response into our serverResponse buffer.    
        char serverResponse[MAX_TOTAL_BYTES_READ];
        int bytesRead = 0;
        temp = 0;
        while((temp = read(serverFd, serverResponse + bytesRead, HOSTNAME_MAX_SIZE)) != 0){
            bytesRead += temp;
            if(bytesRead >= MAX_TOTAL_BYTES_READ) {
                break;
            }
        }

        // The data you have read is not guaranteed to be null-terminated, so after all the content has been read, 
        // if you want to use it with printf(), you should add the null termination yourself (i.e., at the index 
        // indicated by the total bytes read).
        // serverResponse[bytesRead] = '\0'; // Null terminator.


        /////////////////////////////////////////////////////////
        // Forward the serverResponse to the client
        /////////////////////////////////////////////////////////
        numBytesToWrite = strlen(serverResponse);
        bytesWritten = 0;
        while((temp = write(clientFd, serverResponse + bytesWritten, (numBytesToWrite - bytesWritten))) > 0) {
            bytesWritten += temp;
            if(bytesWritten >= numBytesToWrite) {
                break;
            }
        }
    }

    return 0;
}