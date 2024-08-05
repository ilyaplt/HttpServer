//
// Created by user on 3/6/24.
//

#include "http_serializer.h"

void http_header_free(http_header_t* header) {
    SAFE_FREE(header->request);
    SAFE_FREE(header->method);

    for (int i = 0; i < MAX_HEADERFIELDS; ++i) {
        http_header_field_t* field = &header->fields[i];

        if (!field->key) {
            break;
        }

        SAFE_FREE(field->key);
        SAFE_FREE(field->value);
    }
}

int http_header_push_field(http_header_t* header, const char* key, const char* value) {
    if (header->headers_count == MAX_HEADERFIELDS) {
        return -1;
    }

    http_header_field_t* field = &header->fields[header->headers_count++];

    field->key = strdup(key);
    field->value = strdup(value);

    return 0;
}

void http_header_init(http_header_t* header, const char* method, const char* request, int status) {
    memset(header, 0, sizeof(http_header_t));

    if (request) {
        header->request = strdup(request);
    }

    if (method) {
        header->method = strdup(method);
    }

    header->status_code = status;
}

int http_header_serialize(http_header_t* header, char* buffer, size_t buffer_sz) {
    ssize_t remaining_bytes = (ssize_t)buffer_sz;

    if (!header->request) {
        int result = snprintf(buffer, remaining_bytes, "HTTP/1.1 %d OK\r\n", header->status_code);

        if (remaining_bytes < result) {
            return buffer_sz + result - remaining_bytes;
        }

        remaining_bytes -= result;
        buffer += result;
    }

    for (int i = 0; i < header->headers_count; ++i) {
        http_header_field_t* field = &header->fields[i];

        if (!field->key) {
            break;
        }

        int result = snprintf(buffer, remaining_bytes, "%s: %s\r\n", field->key, field->value);

        if (remaining_bytes < result) {
            return buffer_sz + result - remaining_bytes;
        }

        remaining_bytes -= result;
        buffer += result;
    }

    if (remaining_bytes < 3) {
        return buffer_sz + 3;
    }

    memcpy(buffer, "\r\n", 3);
    remaining_bytes -= 2;

    return buffer_sz - remaining_bytes;
}