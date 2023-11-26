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

#define PORT 8080
#define BACKLOG 10

void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}

int main(int argc, char const* argv[]){
    int server_fd, new_fd;
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
    while(1){
        if(listen(server_fd, BACKLOG) < 0) {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }

        if((new_fd = accept(server_fd, (struct sockaddr *)&addr, &sin_size)) < 0) {
            perror("accept failed");
            continue;
        }

        if(!fork()) { // This is the child process
            close(server_fd); // Child doesn't need the listener
            if(send(new_fd, "Hello World!\n", 14, 0) < 0)
                perror("send");
            close(new_fd);
            exit(EXIT_SUCCESS);
        }
        close(new_fd); // Parent does not need this

        
    }
    
    

    return 0;
}