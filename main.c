#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "ring.h"
#include "http_serializer.h"
#include "conn.h"
#include "utils.h"

#define BACKLOG 512
#define MAX_CONNECTIONS 4096
#define FILE_BUFFER 2048
#define HOSTNAME "localhost"

extern const char* g_dir_path;
extern size_t g_dir_path_sz;

static void server_new_connection_cb(ring_listener_t* listener, int fd);

typedef struct client_file_s {
    ring_file_t file;
    char file_buffer[2048];
    int file_offset;
    void(*cb)(void*, int);
} client_file_t;

typedef struct client_s {
    client_conn_t conn;
    client_file_t* file;
} client_t;

typedef void(*client_file_read_cb)(client_t*, int);

static void server_close_connection_cb(client_t* client, int error);
static void server_read_header_cb(client_t* client, int result);
static void server_sent_header_cb(client_t* client, int result);
static void server_chunk_file_read_cb(client_t* client, int result);
static void server_chunk_file_sent_cb(client_t* client, int result);

static void client_file_read_cb_impl(ring_file_t* handle, int result);
static void server_file_read(client_t* client, int size, client_file_read_cb cb);

static void server_close_connection(client_t* client) {
    server_close_connection_cb(client, 0);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("usage: %s [port] [host-dir]\n", argv[0]);
        return 0;
    }

    int server_port = atoi(argv[1]);

    if (server_port == 0 || server_port > 65535) {
        puts("incorrect port!");
        return 0;
    }

    if (parse_host_dir(argv) != 0) {
        return 0;
    }

    ring_loop_t loop;

    if (ring_loop_init(&loop, MAX_CONNECTIONS) != 0) {
        puts("failed to init io_uring! check capability with your kernel!");
        return 0;
    }

    int server_fd = make_server(BACKLOG, server_port);

    if (server_fd == 0) {
        puts("failed to open port! select another one port or run as root");
        return 0;
    }

    ring_listener_t listener;

    ring_listener_init(&listener, &loop, server_fd);

    ring_listener_start(&listener, server_new_connection_cb);

    ring_loop_submit(&loop);

    ring_loop_run(&loop);

    return 0;
}

static void server_new_connection_cb(ring_listener_t* listener, int fd) {
    if (fd > 0) {
        client_t* client = (client_t*)malloc(sizeof(client_t));
        memset(client, 0, sizeof(client_t));

        client_async_init(&client->conn, listener->handle.loop, fd, (client_error_cb)server_close_connection_cb);
        client_async_read(&client->conn, (client_conn_cb)server_read_header_cb);
    }
}

static void server_close_connection_cb(client_t* client, int error) {
    close(client->conn.handle.fd);

    if (client->file) {
        close(client->file->file.fd);
        free(client->file);
    }

    free(client);
}

static void server_read_header_cb(client_t* client, int result) {
    if (result < 32) {
        server_close_connection_cb(client, EBADMSG);
        return;
    }

    client->conn.buffer[result - 1] = 0;
    char* requested_file = parse_http_header(client->conn.buffer);

    if (requested_file == NULL) {
        server_close_connection(client);
        return;
    }

    int file_fd = open(requested_file, O_RDONLY);

    if (file_fd == -1) {
        server_close_connection(client);
        return;
    }

    struct stat file_info;

    if (stat(requested_file, &file_info) != 0) {
        server_close_connection(client);
        return;
    }

    char file_size[32];

    sprintf(file_size, "%zu", file_info.st_size);

    free(requested_file);

    client_file_t* file = (client_file_t*)malloc(sizeof(client_file_t));
    memset(file, 0, sizeof(client_file_t));
    ring_file_init(&file->file, client->conn.handle.handle.loop, file_fd);
    file->file.handle.user_data = client;

    client->file = file;

    http_header_t header;

    http_header_init(&header, NULL, NULL, 200);

    http_header_push_field(&header, "Host", HOSTNAME);
    http_header_push_field(&header, "Content-Length", file_size);
    http_header_push_field(&header, "Connection", "closed");

    char buffer[512];

    result = http_header_serialize(&header, buffer, sizeof(buffer));
    http_header_free(&header);

    client_async_write(&client->conn, buffer, result, (client_conn_cb)server_sent_header_cb);
}

static void client_file_read_cb_impl(ring_file_t* handle, int result) {
    client_t* client = (client_t*)handle->handle.user_data;

    if (result < 0) {
        server_close_connection_cb(client, result);
        return;
    }

    client->file->file_offset += result;

    ((client_file_read_cb)client->file->cb)(client, result);
}

static void server_file_read(client_t* client, int size, client_file_read_cb cb) {
    if (!client->file) {
        abort();
    }

    client->file->cb = (void(*)(void*, int))cb;

    ring_file_read(&client->file->file, client->file->file_buffer, size, client->file->file_offset, client_file_read_cb_impl);
}

static void server_sent_header_cb(client_t* client, int result) {
    server_file_read(client, FILE_BUFFER, server_chunk_file_read_cb);
}

static void server_chunk_file_read_cb(client_t* client, int result) {
    if (result == 0) {
        server_close_connection(client);
        return;
    }

    client_async_write(&client->conn, client->file->file_buffer, result, (client_conn_cb)server_chunk_file_sent_cb);
}

static void server_chunk_file_sent_cb(client_t* client, int result) {
    server_file_read(client, FILE_BUFFER, server_chunk_file_read_cb);
}
