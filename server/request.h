typedef enum Method {GET, POST, UNSUPPORTED} Method;

typedef struct request
{
    /* data */ 
    enum Method method;
    char* url;
    char* version;
    char* options;
    char* body;
};

struct Request *parse_request(const char *raw);
void free_request(struct Request *req);

