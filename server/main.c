#include <pthread.h>
#include <time.h>

#include "request.c"

#define PORT 8080 
#define BACKLOG 10 
#define BUFSIZE 1024 
#define MAX_BUFSIZE 100*1024
#define MAX_TIMEOUT 20 // 3 minutes

pthread_mutex_t lock; 
int num_of_threads;

const char* get_method_name(enum Method method) {
    switch (method){
        case GET: return "GET";
        case POST: return "POST";
        default: return "UNSUPPORTED";
    }
}

void* handle_connection(void* p_clntSocket);
void handle_get(int clntSocket, char* url);
void handle_post(int clntSocket, char* url, char* body);
float timeout();

int main(int argc, char const* argv[]){ 
    if(argc != 2){
        perror("Parameter(s): <Server Port>");
        exit(EXIT_FAILURE);
    }

    in_port_t port = atoi(argv[1]);

    int server_fd, client_fd; 
    struct sockaddr_in addr; 
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
    addr.sin_port = htons(port); 
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // use my IP address 
    memset(&(addr.sin_zero), '\0', 8); 
 
    // Bind to the port
    if(bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    // Listen to incoming connections
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
 
        printf("Connected with client %d\n", client_fd); 

        // Delegate the connection handling to a working thread
        pthread_t t;
        // A pointer to the client socket number
        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;
        pthread_create(&t, NULL, handle_connection, pclient);
        pthread_detach(t);
    } 

    puts("Closed\n");
    return 0; 
}

void* handle_connection(void* p_clntSocket) {
    // Increment number of working threads
    pthread_mutex_lock(&lock); 
    num_of_threads++;
    pthread_mutex_unlock(&lock); 

    int clntSocket = *(int *)p_clntSocket;
    free(p_clntSocket);

    // Start setting time for persistent connection
    time_t start_time = time(NULL);

    do {
        char buffer[MAX_BUFSIZE]; 

        // Receive from client 
        ssize_t bytes_rcvd = recv(clntSocket, buffer, MAX_BUFSIZE - 1, 0); 
        if(bytes_rcvd < 0) { 
            perror("recieve failed"); 
            exit(EXIT_FAILURE); 
        } else if(bytes_rcvd == 0) { // Stream ended
            continue;
        }

        // Print the request 
        buffer[bytes_rcvd] = '\0';
        printf("REQUEST:\n%s\n", buffer);
        fflush(stdout);

        // Parse the HTTP request 
        struct Request *req = parse_request(buffer); 

        if(req->method == GET) {
            handle_get(clntSocket, req->url);
        } else if (req->method == POST) {
            handle_post(clntSocket, req->url, req->body);
        }

        free(req);

    } while (difftime(time(NULL), start_time) < timeout()); 

    // Close connection
    close(clntSocket); 
     
    printf("Connection closed with client %d\n", clntSocket); 

    // Decrement number of working threads
    pthread_mutex_lock(&lock); 
    num_of_threads--;
    pthread_mutex_unlock(&lock); 

    return NULL;
} 

float timeout() {
    return (float)MAX_TIMEOUT / num_of_threads;
}

void handle_get(int clntSocket, char* url) {
    // Read file and sed its contents to the client 
    FILE *fp = fopen(url, "rb"); 
    if(fp == NULL) { 
        printf("ERROR(open): %s\n", url); 
        char* header = "HTTP/1.1 404 Not Found\r\n\r\n";
        write(clntSocket, header, strlen(header));
        return;
    } 

    // Get size of file
    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    rewind(fp);

    char header[BUFSIZE];
    size_t tot = snprintf(header, BUFSIZE, "HTTP/1.1 200 OK\r\nContent-length: %lu\r\n\r\n", file_size);
    
    // Send the header
    send(clntSocket, header, tot, 0);

    // Read contents of file
    ssize_t bytes_read;
    char file[file_size];
    fread(file, 1, file_size, fp);
    printf("Sending %2lu bytes\n", file_size); 
    send(clntSocket, file, file_size, 0); 
    fclose(fp);

}

void handle_post(int clntSocket, char* url, char* body) {
    char* header = "HTTP/1.1 200 OK\r\n\r\n";
    send(clntSocket, header, strlen(header), 0); 

    FILE *fp = fopen(url, "wb"); 
    if(fp == NULL) { 
        printf("ERROR(open): %s\n", url); 
        return;
    } 

    fwrite(body, 1, strlen(body), fp);
    fclose(fp);
}