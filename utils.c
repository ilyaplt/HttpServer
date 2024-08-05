//
// Created by user on 3/7/24.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "utils.h"

const char* g_dir_path = NULL;
size_t g_dir_path_sz = 0;

int parse_host_dir(char** argv) {
    const char* host_directory = argv[2];

    struct stat accessible;

    if (stat(host_directory, &accessible) != 0 || !S_ISDIR(accessible.st_mode)) {
        printf("%s is not directory!\n", host_directory);
        return -1;
    }

    size_t host_directory_sz = strlen(host_directory);

    if (host_directory[host_directory_sz - 1] != '/') {
        char* new_directory = (char*)malloc(host_directory_sz + 2);

        memcpy(new_directory, host_directory, host_directory_sz);

        new_directory[host_directory_sz] = '/';
        new_directory[host_directory_sz + 1] = 0;

        host_directory = new_directory;
    }

    g_dir_path = host_directory;
    g_dir_path_sz = strlen(g_dir_path);

    return 0;
}

char* parse_http_header(char* data) {
    char* request = strstr(data, "/");

    if (!request) {
        return NULL;
    }

    char* request_end = strstr(request, " ");

    if (!request_end) {
        return NULL;
    }

    size_t request_line_sz = request_end - request;

    char* request_line = (char*)malloc(request_line_sz + 2);

    memcpy(request_line, request, request_line_sz);

    request_line[request_line_sz] = 0;

    static const char* default_file = "index.html";
    static size_t default_file_sz = 10;

    char* requested_path = NULL;
    size_t requested_path_sz;

    if (strcmp(request_line, "/") != 0) {
        requested_path_sz = g_dir_path_sz + request_line_sz;
        requested_path = (char*)malloc(requested_path_sz);

        memcpy(requested_path, g_dir_path, g_dir_path_sz);
        memcpy(requested_path + g_dir_path_sz, request_line + 1, request_line_sz - 1);

        requested_path[requested_path_sz] = 0;
    } else {
        requested_path_sz = g_dir_path_sz + default_file_sz + 1;
        requested_path = (char*)malloc(requested_path_sz);

        memcpy(requested_path, g_dir_path, g_dir_path_sz);
        memcpy(requested_path + g_dir_path_sz, default_file, default_file_sz);

        requested_path[requested_path_sz] = 0;
    }

    return requested_path;
}

int make_server(int backlog, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in address;

    memset(&address, 0, sizeof(struct sockaddr_in));

    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    if (bind(fd, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) != 0) {
        close(fd);
        return 0;
    }

    if (listen(fd, backlog) != 0) {
        close(fd);
        return 0;
    }

    return fd;
}