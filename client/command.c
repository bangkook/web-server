#include "command.h"

char* get_file_name(char* full_path) {
    size_t len, tot_len = 0;
    while((len = strcspn(full_path, "/")) != strlen(full_path)) {
        full_path += len + 1;
        tot_len += len + 1;
    }
    char* name = malloc(len + 1);
    memcpy(name, full_path, len);
    name[len] = '\0';
    full_path -= tot_len; // Return full_path pointer to initial value;
    return name;
}

struct Command *parse_command(const char *raw){
    struct Command *cmd = NULL; 
    cmd = malloc(sizeof(struct Command)); 
    memset(cmd, 0, sizeof(struct Command)); 
 
    // Extract method 
    size_t method_sz = strcspn(raw, " "); 
    cmd->method = malloc(method_sz + 1);
    memcpy(cmd->method, raw, method_sz);
    cmd->method[method_sz] = '\0';

    raw += method_sz + 1;

    // Extract file path on server
    size_t path_sz = strcspn(raw, " "); 
    cmd->file_path = malloc(path_sz + 1);
    memcpy(cmd->file_path, raw, path_sz);
    cmd->file_path[path_sz] = '\0';

    raw += path_sz + 1;

    // Extract file name from file path
    cmd->file_name = get_file_name(cmd->file_path);

    // Extract host name
    size_t host_sz = strcspn(raw, " "); 
    cmd->hostname = malloc(host_sz + 1);
    memcpy(cmd->hostname, raw, host_sz);
    cmd->hostname[host_sz] = '\0';

    raw += host_sz + 1;

    // Extract server port
    size_t port_sz = strlen(raw); 
    cmd->port = malloc(port_sz + 1);
    memcpy(cmd->port, raw, port_sz);
    cmd->port[port_sz] = '\0';

    raw += port_sz + 1;

    return cmd;
}

// Free allocated memory to avoid leakage
void free_command(struct Command *cmd){
    free(cmd);
}
