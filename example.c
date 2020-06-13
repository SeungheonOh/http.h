#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "./http.h"

#define ROUTE_START() if(0) {
#define ROUTE(REQ,PATH) } else if(strcmp(REQ->method, "GET") == 0 && bytebuf_compare_string(REQ->target, PATH)) {
#define ROUTE_TYPE(REQ,TYPE,PATH) } else if(strcmp(REQ->method, TYPE) == 0 && bytebuf_compare_string(REQ->target, PATH)) {
#define ROUTE_DEFAULT() } else {
#define ROUTE_END() }

/*
 * Create new server
 */
int new_server(char* address, int port) {
  if(!address) return -1;

  struct sockaddr_in addr;
  int fd;
  
  // Create socket
  if((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) return 0;

  // Configure address
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(address);
  addr.sin_port = htons(port);

  // Bind
  if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    return -1;

  return fd;
}

int main(int argv, char** argc) {
  if(argv < 2) return 1;
  int serverfd = new_server("10.0.0.8", atoi(argc[1]));
  if(serverfd == -1) return 1;

  // Start listening
  listen(serverfd, 5);

  while(1){
    int confd = accept(serverfd, (struct sockaddr*)NULL, NULL);
        
    // Read HTTP request
    char buf[100000] = "\0";
    int rn = read(confd, buf, sizeof(buf));

    // Parse request
    http_request* req = request_parse(buf);
    if(req == NULL) continue;
    printf("Request: %s Target: %.*s\n", req->method, (int)req->target->len, req->target->buf);

    // Create response
    http_response* resp = response_new();
    response_set_version(resp, "HTTP/1.1");
    response_set_status_code(resp, 200);
    response_set_status_text(resp, "OK");

    // Route
    ROUTE_START()
    ROUTE(req,"/home")
      bytebuf_append_string(resp->body, "Hello world");
      bytebuf_append_string(resp->body, "this is /home");
    ROUTE_TYPE(req,"POST", "/post")
      bytebuf_append_string(resp->body, "this is response to the POST request");
    ROUTE_DEFAULT()
      bytebuf_append(resp->body, req->target->buf, req->target->len);
      bytebuf_append_string(resp->body, " is unhandled");
    ROUTE_END() 
    
    // Send Response to client
    char* resp_str = response_string(resp);
    write(confd, resp_str, strlen(resp_str));
    write(confd, "\r\n", strlen("\r\n"));
    write(confd, resp->body->buf, resp->body->len);
    close(confd);

    // Free
    free(resp_str);
    response_free(resp);
    request_free(req);
    close(confd);
  }
  // Clean up
  shutdown(serverfd, SHUT_WR);
  close(serverfd);
}
