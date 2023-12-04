#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 

struct Command{
    char* method;
    char* file_path;
    char* file_name;
    char* hostname;
    char* port;
};

struct Command *parse_command(const char *raw); 
void free_command(struct Command *cmd);
