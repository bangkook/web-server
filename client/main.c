#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <sys/wait.h> 
#include <signal.h> 

#include "command.c"

#define PORT 80
#define BUFSIZE 1024
#define MAX_BUFSIZE 10*1024

void request_service(int sock, struct Command* cmd);
void write_to_file(char* file_name, char buffer[], int len);
void handle_get(char buffer[], int sock, char* file_name, size_t bytes_rcvd);

int get_num_chars(char* file_name) {
    FILE* fp = fopen(file_name, "rb");
    int count = 0;
    for(char c = getc(fp); c != EOF; c = getc(fp)) {
        count = count + 1;
    }
    fclose(fp);
    return count;
}

int main(int argc, char const* argv[]){

    if(argc < 2 || argc > 3) {
        perror("Parameter(s) <Server Address> [<Server Port>]");
        exit(EXIT_FAILURE);
    }

    char* server_ip = argv[1];

    in_port_t server_port = (argc == 3) ? atoi(argv[2]) : PORT;
    
    int sock;
    int rtn_val;
    struct sockaddr_in server_addr;


    // Create a reliable stream socket using TCP
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Construct the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    // Convert address
    if((rtn_val = inet_pton(AF_INET, server_ip, &server_addr.sin_addr.s_addr)) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_port = htons(server_port); // Server port

    // Establish connection to the server
    if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Read and parse commands from file
    char* file_name = "input.txt";

    FILE *cfp = fopen(file_name, "r");
    if(cfp == NULL) {
        printf("ERROR(open): %s\n", file_name);
        fclose(cfp);
        close(sock);
        return -1;
    }

    char command[BUFSIZE];
    // Read commands till end of file
    while(fgets(command, BUFSIZE, cfp) != NULL){
        puts(command);
        struct Command *cmd = parse_command(command);
        request_service(sock, cmd);
    }

    fclose(cfp);
    close(sock);
    return 0;
}

void request_service(int sock, struct Command* cmd) {
    char req[BUFSIZE];
    size_t req_len = snprintf(req, BUFSIZE - 1, "%s %s HTTP/1.0\r\n"
                "Host: localhost\r\n"
                "Accept: text/html/png\r\n"
                "\r\n"
                "Empty body for debugging", cmd->method, cmd->file_path);
    
    //size_t req_len = len;

    ssize_t num_bytes = send(sock, req, req_len, 0);
    if(num_bytes < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    } else if (num_bytes != req_len) {
        perror("sent unexpected number of bytes");
    }

    size_t bytes_rcvd;
    char buffer[BUFSIZE];

    //fputs("Received:\n", stdout);
    //fflush(stdout);

    if((bytes_rcvd = recv(sock, buffer, BUFSIZE, 0)) > 0) {
        printf("Received %ld bytes\n", bytes_rcvd);

        if(memcmp(cmd->method, "GET", strlen("GET")) == 0)
            handle_get(buffer, sock, cmd->file_name, bytes_rcvd);
        else {
            buffer[bytes_rcvd] = '\0';  
            fputs(buffer, stdout);
            fflush(stdout);
        }

    }

    fputc('\n', stdout);
}

void handle_get(char buffer[], int sock, char* file_name, size_t bytes_rcvd) {

    // Extract files's size
    char* content_length = strstr(buffer, "Content-length");
    size_t file_size = 0;

    if(content_length) {
        content_length = strtok(content_length, "\r\n") + strcspn(content_length, " ") + 1;
        file_size = atoi(content_length);
    } else { // File not found
        buffer[bytes_rcvd] = '\0';  
        printf("%s\n", buffer);
        fflush(stdout);
        return;
    }

    int offset = 0;
    // Proceed pointer to the body
    while(buffer[offset] != '\r' || buffer[offset + 1] != '\n') {
        size_t sz = strcspn(buffer + offset, "\r\n");
        offset += sz + 2;
    }

    offset += 2;

    FILE *fp = fopen(file_name, "ab");

    if(fp == NULL) {
        printf("ERROR(open): %s\n", file_name);
        fclose(fp);
        return;
    }

    fwrite(buffer + offset, 1, bytes_rcvd - offset, fp);

    int tot_bytes = bytes_rcvd - offset;

    buffer[bytes_rcvd] = '\0';  
    printf("%s\n", buffer);
    fflush(stdout);

    // Read file content and store it in the local directory
    while(tot_bytes < file_size && (bytes_rcvd = recv(sock, buffer, BUFSIZE, 0)) > 0) {
        printf("Received %ld bytes\n", bytes_rcvd);  
        fwrite(buffer, 1, bytes_rcvd, fp);

        buffer[bytes_rcvd] = '\0';   
        printf("%s\n", buffer);
        fflush(stdout);

        tot_bytes += bytes_rcvd;
    }
    
    fclose(fp);
}