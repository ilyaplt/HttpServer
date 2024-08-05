//
// Created by user on 3/7/24.
//

#ifndef HTTP_SERVER3_UTILS_H
#define HTTP_SERVER3_UTILS_H

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

char* parse_http_header(char* data);
int parse_host_dir(char** argv);
int make_server(int backlog, uint16_t port);

#endif //HTTP_SERVER3_UTILS_H
