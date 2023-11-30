#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include<request.h>

#define PORT 8080
#define BACKLOG 10
#define BUFSIZE 1024

void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}

int main(int argc, char const* argv[]){
    int server_fd, client_fd;
    struct sockaddr_in addr;
    struct sigaction sa;
    int sin_size = sizeof(struct sockaddr_in);
    int yes = 1;

    // Creating socket file descriptor
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Reuse port 
    if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY; // use my IP address
    memset(&(addr.sin_zero), '\0', 8);

    if(bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    if(listen(server_fd, BACKLOG) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while(1) {

        printf("Waiting for connections...\n");

        if((client_fd = accept(server_fd, (struct sockaddr *)&addr, &sin_size)) < 0) {
            perror("accept failed");
            continue;
        }

        printf("Connected\n");

        handle_connection(client_fd);
    }
    
    void handle_connection(int clntSocket) {
        char buffer[BUFSIZE];

        // Receive from client
        size_t bytes_rcvd = recv(clntSocket, buffer, BUFSIZE, 0);
        if(bytes_rcvd < 0) {
            perror("recieve failed");
            exit(EXIT_FAILURE);
        }

        // Parse the HTTP request
        struct Request req = parse_request(buffer);

        // Print the request
        printf("REQUEST:\n%s %s %s\n%s\n", req->method, req->url, req->version, req->options);
        fflush(stdout);

        // Read file and sed its contents to the client
        FILE *fp = fopen(req->url, "r");
        if(fp == NULL) {
            printf("ERROR(open): %s\n", req->url);
            close(clntSocket);
            return;
        }

        char* header = "HTTP/1.1 200 OK \r\n";
        size_t bytes_read;
        while((bytes_read = fread(buffuer, 1, BUFSIZE, fp)) > 0) {
            printf("Sending %2u bytes\n", bytes_read);
            write(clntSocket, buffer, bytes_read);
        } 

        close(clntSocket);
        fclose(fp);
        printf("Connection closed\n");

    }
    

    return 0;
}