/***********************************/
// Maxwell R. Zinzilieta
// CS 447 Project 1 Basic Security Proxy 
// September 2023
/***********************************/

/* Basic Libraries */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

/* Unix Libraries */
#include <pthread.h>
#include <unistd.h>

/* Networking Libraries */
#include <sys/types.h>      
#include <netinet/in.h>     
#include <sys/socket.h>     
#include <arpa/inet.h>       

/* Defines */
#define CLIENT_REQUEST_PORT 8080
#define WEB_SERVER_PORT 9080

/* Function Declarations */
int socketCreation();

int main() {

    return 0;
}

int socketCreation() {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in proxy_accept_addr;
    int proxy_accept_addrlen = sizeof(proxy_accept_addr);

    int n;
    //socket
    //int socket(int domain, int type, int protocol)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("error with socket creation");
    }

    //bind
    //int bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
    proxy_accept_addr.sin_family = AF_INET;
    proxy_accept_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_accept_addr.sin_port = htons(CLIENT_REQUEST_PORT);
    bind(sockfd, (sockaddr*)&proxy_accept_addr, proxy_accept_addrlen);

    //listen
    //int listen(int fd, int backlog_queue_size)
    listen(sockfd, 3);

    while(true) {
        //accept
        //int accept(int fd, struct sockaddr *remote_host, socklen_t addr_length)
        int connection = accept(sockfd, (struct sockaddr*)&proxy_accept_addr, (socklen_t*)&proxy_accept_addrlen);
        
        //socket to talk to web server
        int proxy_to_server_fd = socket(AF_INET, SOCK_STREAM, 0);

        //connect to server port
        struct sockaddr_in proxy_to_server;
        proxy_to_server.sin_family = AF_INET;
        proxy_to_server.sin_addr.s_addr = inet_addr("146.163.112.5");
        proxy_to_server.sin_port = htons(WEB_SERVER_PORT);

        connect(proxy_to_server_fd, (struct sockaddr *)&proxy_to_server, sizeof(proxy_to_server));
        
    }

    return 0;
}
