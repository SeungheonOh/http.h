#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct {
  char* buf;
  size_t len;
} bytebuf;

typedef struct {
  struct _header_list {
    char* key;
    char* value;
  } *headers;
  size_t size;
} http_headers;

typedef struct {
  char version[20];
  uint16_t status_code;
  char status_text[20];
  http_headers* headers;
  bytebuf* body;
} http_response;

typedef struct {
  char version[20];
  char method[20];
  bytebuf* target;
  http_headers *headers;
  bytebuf* body;
} http_request;

bytebuf* bytebuf_new();
bytebuf* to_bytebuf(char*);
void bytebuf_free(bytebuf*);
void bytebuf_append(bytebuf*, char*, size_t);
void bytebuf_set(bytebuf*, char*, size_t);
void bytebuf_append_string(bytebuf*, char*);
void bytebuf_set_string(bytebuf*, char*);
void bytebuf_clear(bytebuf*);
void bytebuf_cstring(bytebuf*, char*);
bool bytebuf_compare(bytebuf* a, bytebuf* b);
bool bytebuf_compare_string(bytebuf* a, char* b);
bool bytebuf_substring_compare(bytebuf*, int, int, char*);
int bytebuf_get_nth_index(bytebuf*, char*, size_t, int);
int bytebuf_get_nth_index_string(bytebuf*, char*, int);
int bytebuf_get_index(bytebuf*, char*, size_t);
int bytebuf_get_index_string(bytebuf*, char*);

http_headers* headers_new();
int headers_free(http_headers* h);
http_headers* headers_parse(char* s);
int _headers_add(http_headers* h, char* key, char* value);
int headers_add_single(http_headers* h, char* key, char* value);
int headers_add(http_headers* h, char** headers);
char* headers_get(http_headers* h, char* key);
char* headers_string(http_headers* h);

http_request* request_new();
void request_free(http_request*);
http_request* request_parse(char*);
int request_set_version(http_request*, char*);
int request_set_method(http_request*, char*);
char* request_string(http_request* r);

http_response* response_new();
void response_free(http_response*);
http_response* response_parse(char*);
void response_set_version(http_response*, char*);
void response_set_status_code(http_response*, uint16_t);
void response_set_status_text(http_response*, char*);
char* response_string(http_response*);


bytebuf* bytebuf_new() {
  bytebuf* s = malloc(sizeof(bytebuf));
  s->buf = malloc(0);
  s->len = 0;
  return s;
}

bytebuf* to_bytebuf(char* str) {
  bytebuf* r = bytebuf_new();
  bytebuf_set_string(r, str);
  return r;
}

void bytebuf_free(bytebuf* s) {
  free(s->buf);
  free(s);
}

void bytebuf_append(bytebuf* s, char* a, size_t l) {
  s->buf = realloc(s->buf, l + s->len);
  memcpy(s->buf + s->len, a, l);
  s->len += l;
}

void bytebuf_set(bytebuf* s, char* a, size_t l) {
  s->buf = realloc(s->buf, l);
  memcpy(s->buf, a, l);
  s->len = l;
}

void bytebuf_append_string(bytebuf* s, char* str) {
  size_t slen = strlen(str);
  s->buf = realloc(s->buf, slen + s->len);
  memcpy(s->buf + s->len, str, slen);
  s->len += slen;
}

void bytebuf_set_string(bytebuf* s, char* str) {
  size_t slen = strlen(str);
  s->buf = realloc(s->buf, slen);
  memcpy(s->buf, str, slen);
  s->len = slen;
}

void bytebuf_clear(bytebuf* s) {
  s->buf = realloc(s->buf, 0);
  s->len = 0;
}

// precondition: output >= s->len
void bytebuf_cstring(bytebuf* s, char* output) {
  memcpy(output, s->buf, s->len);
}

void bytebuf_substring(bytebuf* s, int index, int len, char* output) {
  if(index + len > s->len) return;
  memcpy(output, s->buf+index, len);
  output[len] = '\0';
}

bool bytebuf_compare(bytebuf* a, bytebuf* b) {
  if(a->len != b->len) return false;
  for(int i = 0; i < a->len; i++) 
    if(a->buf[i] != b->buf[i]) return false;
  return true;
}

bool bytebuf_compare_string(bytebuf* a, char* b) {
  if(a->len != strlen(b)) return false;
  for(int i = 0; i < a->len; i++) 
    if(a->buf[i] != b[i]) return false;
  return true;
}

bool bytebuf_substring_compare(bytebuf* a, int index, int len, char* b) {
  if(index + len > a->len) return false;
  for(int i = 0; i < len; i++) 
    if(a->buf[i + index] != b[i]) return false;
  return true;
}

int bytebuf_get_nth_index(bytebuf* a, char* b, size_t len, int n) {
  if(a->len < len) return -1;
  for(int i = 0; i <= a->len - len; i++)
    if(bytebuf_substring_compare(a, i, len, b)) 
      if(n == 0) return i; 
      else n--;
  return -1;
}

int bytebuf_get_nth_index_string(bytebuf* a, char* b, int n) {
  size_t blen = strlen(b);
  if(a->len < blen) return -1;
  for(int i = 0; i <= a->len - blen; i++)
    if(bytebuf_substring_compare(a, i, blen, b)) 
      if(n == 0) return i; 
      else n--;
  return -1;
}

int bytebuf_get_index(bytebuf* a, char* b, size_t len) {
  if(a->len < len) return -1;
  for(int i = 0; i <= a->len - len; i++)
    if(bytebuf_substring_compare(a, i, len, b)) return i;
  return -1;
}

int bytebuf_get_index_string(bytebuf* a, char* b) {
  size_t blen = strlen(b);
  if(a->len < blen) return -1;
  for(int i = 0; i <= a->len - blen; i++)
    if(bytebuf_substring_compare(a, i,blen, b)) return i;
  return -1;
}

/*
 * HTTP Header
 */


http_headers* headers_new(){
  http_headers* h = malloc(sizeof(http_headers));
  h->headers = malloc(0);
  h->size = 0;
  return h;
}

int headers_free(http_headers* h) {
  for(int i = 0; i < h->size; i++) {
    free(h->headers[i].key);
    free(h->headers[i].value);
  }
  free(h->headers);
  free(h);
}

http_headers* headers_parse(char* s) {
  http_headers* ret = headers_new();
  char* header;
  char* rest = s;

  while((header = __strtok_r(rest, "\r\n", &rest))) {
    char* key = strtok(header, ":");
    char* value = strtok(NULL, "");
    if(!key || !value) continue;
    if(headers_add_single(ret, key, value)) return NULL;
  }
  return ret;
}

int _headers_add(http_headers* h, char* key, char* value) {
  h->headers = realloc(h->headers, (h->size + 1) * sizeof(*h->headers));
  h->headers[h->size].key = (char*)malloc(strlen(key)+1);
  h->headers[h->size].value = (char*)malloc(strlen(value)+1);
  if(!h->headers[h->size].key || !h->headers[h->size].value) return 1;
  strcpy(h->headers[h->size].key, key);
  strcpy(h->headers[h->size].value, value);
  h->size++;
  return 0;
}

/*
 * Add single header to http_headers
 */
int headers_add_single(http_headers* h, char* key, char* value) {
  return _headers_add(h, key, value);
}

/*
 * Addes Headers via NULL-terminated array with key and value in pair.
 * for example, { "key 1", "value 1", ..., NULL }
 */
int headers_add(http_headers* h, char** headers) {
  do {
    if(_headers_add(h, *headers, *(headers + 1))) return 1;
  } while ((headers += 2) && *headers && *(headers + 1));
}

char* headers_get(http_headers* h, char* key) {
  for(int i = 0; i < h->size; i++) 
    if(strcmp(h->headers[i].key, key) == 0)
      return h->headers[i].value;
  return "";
}

/*
 * Parse headers to string
 * returned char* needs to be freed
 */
char* headers_string(http_headers* h) {
  char* buf = malloc(2049);
  char* b = buf;
  int n;
  *buf = '\0';
  for(int i = 0; i < h->size; i++) {
    n = snprintf(b, 2049 - (b - buf), "%s:%s\r\n", h->headers[i].key, h->headers[i].value);
    if(n > 0) b += n;
  }
  return buf;
};

http_request* request_new() {
  http_request* r = malloc(sizeof(http_request));
  r->headers = headers_new();
  r->target = bytebuf_new();
  r->body = bytebuf_new();
  return r;
}

void request_free(http_request* r) {
  headers_free(r->headers);
  bytebuf_free(r->body);
  free(r->target);
  free(r);
}

http_request* request_parse(char* s) {
  http_request* ret = request_new();
  bytebuf* raw = to_bytebuf(s);
  bytebuf* buf = bytebuf_new();
  char segment[4096] = "\0";
  int index1, index2;

  for(int i = 0; i < raw->len; i++)
    if(bytebuf_substring_compare(raw, i, 2, "\r\n")) break;
    else bytebuf_append(buf, raw->buf+i, 1);

  index1 = 0;
  index2 = bytebuf_get_nth_index_string(buf, " ", 0);
  if(index2 == -1) return NULL;
  bytebuf_substring(buf, index1, index2 - index1, segment);
  request_set_method(ret, segment);

  segment[0] = '\0';
  index1 = index2 + 1;
  index2 = bytebuf_get_nth_index_string(buf, " ", 1);
  if(index2 == -1) return NULL;
  bytebuf_substring(buf, index1, index2 - index1, segment);
  bytebuf_set_string(ret->target, segment);

  segment[0] = '\0';
  index1 = index2 + 1;
  if(index2 == -1) return NULL;
  bytebuf_substring(buf, index1, buf->len - index1, segment);
  request_set_version(ret, segment);

  index1 = buf->len + 2; // +2 for \r and \n
  bytebuf_clear(buf);

  for(int i = index1; i < raw->len; i++)
    if(bytebuf_substring_compare(raw, i, 4, "\r\n\r\n")) break;
    else bytebuf_append(buf, raw->buf+i, 1);

  segment[0] = '\0';
  bytebuf_substring(buf, 0, buf->len, segment);

  headers_free(ret->headers);
  ret->headers = headers_parse(segment);

  index1 += buf->len + 4; // +2 for \r\n\r\n
  if(index1 < raw->len) bytebuf_set(ret->body, raw->buf + index1, raw->len - index1);
  bytebuf_free(buf);
  bytebuf_free(raw);

  return ret;
}

int request_set_version(http_request* r, char* v) {
  if(!r || !v) return 1;
  strncpy(r->version, v, 20);
}

int request_set_method(http_request* r, char* m) {
  if(!r || !m) return 1;
  strncpy(r->method, m, 20);
}

/*
 * This function return string that contains HTTP version information
 * and Header. This function DOES NOT return body since body isn't Cstring always
 */
char* request_string(http_request* r) {
  char* buf = malloc(4097);
  char* h = headers_string(r->headers);

  snprintf(buf, 4096, "%s %.*s %s\r\n%s",
      r->method,
      (int)r->target->len, r->target->buf,
      r->version,
      h);

  free(h);
  return buf;
}

http_response* response_new() {
  http_response* r = malloc(sizeof(http_response));
  r->headers = headers_new();
  r->body = bytebuf_new();
  return r;
}

void response_free(http_response* r) {
  headers_free(r->headers);
  bytebuf_free(r->body);
  free(r);
}

http_response* response_parse(char* s) {
  http_response* ret = response_new();
  bytebuf* raw = to_bytebuf(s);
  bytebuf* buf = bytebuf_new();
  char segment[4096] = "\0";
  int index1, index2;

  for(int i = 0; i < raw->len; i++)
    if(bytebuf_substring_compare(raw, i, 2, "\r\n")) break;
    else bytebuf_append(buf, raw->buf+i, 1);

  index1 = 0;
  index2 = bytebuf_get_nth_index_string(buf, " ", 0);
  if(index2 == -1) return NULL;
  bytebuf_substring(buf, index1, index2 - index1, segment);
  response_set_version(ret, segment);

  segment[0] = '\0';
  index1 = index2 + 1;
  index2 = bytebuf_get_nth_index_string(buf, " ", 1);
  if(index2 == -1) return NULL;
  bytebuf_substring(buf, index1, index2 - index1, segment);
  response_set_status_code(ret, atoi(segment));

  segment[0] = '\0';
  index1 = index2 + 1;
  if(index2 == -1) return NULL;
  bytebuf_substring(buf, index1, buf->len - index1, segment);
  response_set_status_text(ret, segment);

  index1 = buf->len + 2; // +2 for \r and \n
  bytebuf_clear(buf);

  for(int i = index1; i < raw->len; i++)
    if(bytebuf_substring_compare(raw, i, 4, "\r\n\r\n")) break;
    else bytebuf_append(buf, raw->buf+i, 1);

  segment[0] = '\0';
  bytebuf_substring(buf, 0, buf->len, segment);

  headers_free(ret->headers);
  ret->headers = headers_parse(segment);

  index1 += buf->len + 4; // +2 for \r\n\r\n
  if(index1 < raw->len) bytebuf_set(ret->body, raw->buf + index1, raw->len - index1);
  bytebuf_free(buf);
  bytebuf_free(raw);

  return ret;
}

void response_set_version(http_response* r, char* v) {
  strncpy(r->version, v, 20);
}

void response_set_status_code(http_response* r, uint16_t code) {
  r->status_code = code;
}

void response_set_status_text(http_response* r, char* t) {
  strncpy(r->status_text, t, 20);
}

char* response_string(http_response* r) {
  char* buf = malloc(4097);
  char* h = headers_string(r->headers);

  snprintf(buf, 4096, "%s %d %s\r\n%s", 
      r->version, 
      r->status_code, 
      r->status_text,
      h);

  free(h);
  return buf;
}
