//
// Created by user on 3/6/24.
//

#ifndef GETOPT_EXAMPLE1_HTTP_SERIALIZER_H
#define GETOPT_EXAMPLE1_HTTP_SERIALIZER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_HEADERFIELDS 32

#define SAFE_FREE(x) { if (x) { free(x); x = NULL; }}

typedef struct http_header_field_s {
    char* key;
    char* value;
} http_header_field_t;

typedef struct http_header_s {
    char* request;
    char* method;
    http_header_field_t fields[MAX_HEADERFIELDS];
    int status_code;
    int headers_count;
} http_header_t;

void http_header_init(http_header_t* header, const char* method, const char* request, int status);

int http_header_push_field(http_header_t* header, const char* key, const char* value);

void http_header_free(http_header_t* header);

int http_header_serialize(http_header_t* header, char* buffer, size_t buffer_sz);

#endif //GETOPT_EXAMPLE1_HTTP_SERIALIZER_H
