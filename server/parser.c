#include "request.h"

struct Request *parse_request(const char *raw) {
    struct Request *req = NULL;
    req = malloc(sizeof(struct Request));
    memset(req, 0, sizeof(struct Request));

    // Extract method
    size_t method_sz = strcspn(raw, " ");
    // If method type is GET
    if(memcmp(raw, "GET", strlen("GET")) == 0) {
        req->method = GET;
    } else if (memcmp(raw, "POST", strlen("POST")) == 0) { // If method is POST
        req->method = POST;
    } else { // Unsupported, only GET and POST are supported
        req->method = UNSUPPORTED;
    }

    // Procced pointer after method
    raw += method_sz + 1;

    // Extract URL
    size_t url_sz = strcspn(raw, " ");
    req->url = malloc(url_sz + 1);
    memcpy(req->url, raw, url_sz);
    // Add null pointer to end of string
    req->url[url_sz] = '\0';

    // Procced pointer after url
    raw += url_sz + 1;

    // Extract http version
    size_t ver_sz = strcspn(raw, "\r\n");
    req->version = malloc(ver_sz + 1);
    memcpy(req->version, raw, ver_sz);
    // Add null pointer to end of string
    req->version[ver_sz]  = '\0';

    // Procced pointer to the next line
    raw += ver_sz + 2;

    // Extract options
    // End of optional lines is marked by an empty line with "\r\n"
    if(raw[0] != '\r' || raw[1] != '\n'){ // If there are optional lines
        size_t op_sz = strcspn(raw, "\r\n\r\n");
        req->options = malloc(op_sz + 1);
        memcpy(req->options, raw, op_sz);
        // Add null pointer to end of string
        req->options[op_sz]  = '\0';

        // Procced pointer to the empty line
        raw += op_sz + 2;
    }

    // Procced pointer after the empty line
    raw += 2;

    // Extract body in case of POST request
    size_t body_sz = strlen(raw);
    req->body = malloc(body_sz + 1);
    memcpy(req->body, raw, body_sz);
    req->body[body_sz] = '\0';

    return req;

}

void free_request(struct Request *req){
    free(req->url);
    free(req->version);
    free(req->options);
    free(req->body);
    free(req);
}
