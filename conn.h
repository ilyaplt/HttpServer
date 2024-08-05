//
// Created by user on 3/7/24.
//

#ifndef HTTP_SERVER3_CONN_H
#define HTTP_SERVER3_CONN_H

#define NET_BUFFER 2048

#include "ring.h"

typedef struct client_conn_s {
    ring_tcp_t handle;
    char buffer[NET_BUFFER];
    void(*cb)(struct client_conn_s*, int);
    void(*error_cb)(struct client_conn_s*, int);
    int sent_bytes;
    int total_bytes;
} client_conn_t;

typedef void(*client_error_cb)(client_conn_t*, int);
typedef void(*client_conn_cb)(client_conn_t*, int);

void client_async_init(client_conn_t* client, ring_loop_t* loop, int fd, client_error_cb error_cb);
void client_async_read(client_conn_t* client, client_conn_cb cb);
void client_async_write(client_conn_t* client, char* buffer, int size, client_conn_cb cb);

#endif //HTTP_SERVER3_CONN_H
