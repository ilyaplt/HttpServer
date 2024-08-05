//
// Created by user on 3/7/24.
//

#include "conn.h"
#include <stdio.h>

static void tcp_read_cb(ring_tcp_t* tcp, int result) {
    client_conn_t* conn = (client_conn_t*)tcp;

    if (result <= 0) {
        conn->error_cb(conn, result);
        return;
    }

    conn->cb(conn, result);
}

void client_async_init(client_conn_t* client, ring_loop_t* loop, int fd, client_error_cb error_cb) {
    memset(client, 0, sizeof(client_conn_t));

    ring_tcp_init(&client->handle, loop, fd);
    client->error_cb = error_cb;
}

void client_async_read(client_conn_t* client, client_conn_cb cb) {
    client->cb = cb;

    ring_tcp_receive(&client->handle, client->buffer, NET_BUFFER, tcp_read_cb);
}

static void tcp_write_cb(ring_tcp_t* handle, int result) {
    client_conn_t* conn = (client_conn_t*)handle;

    if (result <= 0) {
        conn->error_cb(conn, result);
        return;
    }

    conn->sent_bytes += result;

    if (conn->sent_bytes >= conn->total_bytes) {
        int sent = conn->sent_bytes;
        conn->sent_bytes = 0;
        conn->total_bytes = 0;

        conn->cb(conn, sent);
        return;
    }

    ring_tcp_send(&conn->handle, conn->buffer + conn->sent_bytes, conn->total_bytes - conn->sent_bytes, tcp_write_cb);
}

void client_async_write(client_conn_t* client, char* buffer, int size, client_conn_cb cb) {
    client->cb = cb;

    if (size > NET_BUFFER) abort();

    memcpy(client->buffer, buffer, size);

    client->total_bytes = size;
    client->sent_bytes = 0;

    ring_tcp_send(&client->handle, client->buffer, size, tcp_write_cb);
}