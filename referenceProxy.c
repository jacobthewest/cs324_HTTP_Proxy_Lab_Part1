#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
// #include <pthread.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

// void *thread(void *vargp);

int main(int argc, char *argv[])
{

    ////////////////////////////////
    // Read in request
    ////////////////////////////////
    struct sockaddr_in ip4addr;
    int sfd;

    // Ignore SISPIPE signal
    signal(SIGPIPE, SIG_IGN);

    ip4addr.sin_family = AF_INET;
    printf("port: %s\n", argv[1]);
    ip4addr.sin_port = htons(atoi(argv[1]));
    ip4addr.sin_addr.s_addr = INADDR_ANY;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    if (bind(sfd, (struct sockaddr *)&ip4addr, sizeof(struct sockaddr_in)) < 0) {
        close(sfd);
        perror("bind error");
        exit(EXIT_FAILURE);
    }
    if (listen(sfd, 100) < 0) {
        close(sfd);
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    int cfd;
    // pthread_t tid;

    while(1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len;
        // cfd = malloc(sizeof(int));

        cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_len);
        // pthread_create(&tid, NULL, thread, cfd);

        // pthread_detach(pthread_self());
    // int cfd = *((int *)vargp);
    // free(vargp);
    int nread = 0;
    int total_bytes_read = 0;
    char buf[MAX_OBJECT_SIZE];
    memset(buf, 0, MAX_OBJECT_SIZE);

    for (;;) {
        nread = read(cfd, buf + total_bytes_read, MAX_OBJECT_SIZE);
        total_bytes_read += nread;

        // Exit loop if reached end of request
        if (nread == 0)
            break;   
        // Re-loop if there is not enough data yet, and there is more
        if (total_bytes_read < 4)
            continue;

        // printf("\nbuffer: \n%s\n", buf);
        if (!memcmp(buf + total_bytes_read - 4, "\r\n\r\n", 4)) {
            break;
        }
        
    }

    char request[4000];



    ////////////////////////////////////
    // Parse client request 
    // and build proxy request
    ////////////////////////////////////

    // This extracts 'GET' and copies it to the request we're building
    char host[400];
    char directory[400];
    char port[40];
    char *token = strtok(buf, " ");
    strcpy(request, token);
    // This extracts the url
    char *url = strtok(NULL, " ");
    char strToParse[800];
    strcpy(strToParse, url);
    char* rest = strToParse;
    // If the website starts with https://, then strtok it again
    // to get the host
    token = strtok_r(rest, "/", &rest);
    rest = rest + 1;
    if(strstr(token, "http:") != NULL) {
        token = strtok_r(rest, " ", &rest); 
    }

    char firstAttempt[400];
    char plainHost[400];
    strcpy(firstAttempt, token);
    rest = firstAttempt;
    if (strstr(token, ":")) {
        token = strtok_r(rest, "/", &rest);
        strcpy(host, token);
        token = strtok_r(rest, "\0", &rest);
        strcpy(directory, token);


        char hostToParse[400];
        strcpy(hostToParse, host);
        rest = hostToParse;
        token = strtok_r(rest, ":", &rest);
        strcpy(plainHost, token);
        strcpy(port, rest);
    }
    else {
        token = strtok_r(rest, "/", &rest);
        strcpy(host, token);
        strcpy(directory, rest);
        strcpy(port, "80");
    }


    if (port[strlen(port) - 1] == '/') {
        port[strlen(port) - 1] = '\0';
    }

    // Add the directory to the request we're building
    // token = strtok(NULL, " ");
    int length = strlen(request);
    request[length] = ' ';
    request[length + 1] = '/';
    strcpy(request + length + 2, directory);
    // Add 'HTTP/1.0' to the request
    length = strlen(request);
    request[length] = ' ';
    strcpy(request + length + 1, "HTTP/1.0\r\n");
    // Find the host line if it exists
    token = strtok(NULL, "Host:");
    if (token != NULL) {
        token = strtok(NULL, " ");
        token = strtok(NULL, "\r\n");
        strcpy(host, token);
    }
    // Add host name to the request
    strcpy(request + strlen(request), "Host: ");
    strcpy(request + strlen(request), host);
    strcpy(request + strlen(request), "\r\n");
    // Add the other things to the request
    strcpy(request + strlen(request), user_agent_hdr);
    strcpy(request + strlen(request), "Connection: close\r\n");
    strcpy(request + strlen(request), "Proxy-Connection: close\r\n");

    // Add on any additional request headers
    while ((token = strtok(NULL, "\n")) != NULL) {
        // Don't add any of these
        if (strstr(token, "Proxy-Connection:") != NULL
            || strstr(token, "Connection:") != NULL)
            continue;
            
        strcpy(request + strlen(request), token);
        strcpy(request + strlen(request), "\n");
    }
    // Terminate request
    strcpy(request + strlen(request), "\r\n");

    // printf("request:%s\n", request);



    /////////////////////////////
    // Forward request to server
    /////////////////////////////

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, fd;

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0; /* Any protocol */
    if (port == NULL || strstr(port, " ") != NULL) {
        strcpy(port, "80");
    }
    printf("HOST: %s\n", host);
    printf("DIRECTORY: %s\n", directory);
    printf("PORT: %s\n", port);

    // If the host has been changed to have the port appended to it,
    // change it back to how it is without the port appended to it
    if (plainHost != NULL && strstr(plainHost, " ") == NULL) {
        strcpy(host, plainHost);
    }
    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    /* getaddrinfo() returns a list of address structures.
    Try each address until we successfully connect(2).
    If socket(2) (or connect(2)) fails, we (close the socket
    and) try the next address. */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1)
            continue;
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; /* Success */
        close(fd);
    }
    if (rp == NULL) { /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
    }
    freeaddrinfo(result); /* No longer needed */



    // Write to server and read back response
    int bytes_read = strlen(request);
    int bytes_sent = 0;
    int current_bytes_received = 0;
    do {
        current_bytes_received = write(fd, request + bytes_sent, bytes_read - bytes_sent);
        if (current_bytes_received < 0) {
            fprintf(stderr, "partial/failed write on line 231\n");
            break;
        }
        bytes_sent += current_bytes_received;

    } while( bytes_sent < bytes_read );

    // char *outputBuffer = malloc( sizeof(char) * ( 40096 + 1 ) );
    char *outputBuffer = calloc(MAX_OBJECT_SIZE, sizeof(char));
    bytes_read = 0;

    do {
        nread = read(fd, outputBuffer + bytes_read, MAX_OBJECT_SIZE);
        if (nread == -1) {
            perror("read");
            break;
        }
        bytes_read += nread;
    } while( nread != 0 );

    // printf("Return response:\n%s", outputBuffer);



    ///////////////////////////////////
    // Forward response back to client
    ///////////////////////////////////

    bytes_sent = 0;
    current_bytes_received = 0;
    do {
        current_bytes_received = write(cfd, outputBuffer + bytes_sent, bytes_read - bytes_sent);
        if (current_bytes_received < 0) {
            fprintf(stderr, "partial/failed write on line 263\n");
            break;
        }
        bytes_sent += current_bytes_received;


    } while( bytes_sent < bytes_read );
    // close(fd);
    // close(cfd);
    free(outputBuffer);
    }

    return 0;
}







// // void *thread(void *vargp) {

    

//     return 0;
// }