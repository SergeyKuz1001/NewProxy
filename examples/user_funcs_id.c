// Copyright 2020 Sergey Kuzivanov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "proxy_funcs.h"
#include "proxy_trace.h"

#define MAX_SOCKET_AMOUNT 300
#define MAX_DATA_SIZE 256
#define MAX_SHORT_INFO_SIZE 64
#define NODE_SERVER "localhost" 
#define SERVICE_SERVER "7000"

typedef struct {
    int fd;
    char info[MAX_SHORT_INFO_SIZE];
} ConnectionInfo_t;

ConnectionInfo_t *socket_info;

int init(int argc, char* argv[]) {
    trace(TL_INFO, "ID-proxy");
    if (argc < 2) {
        trace(TL_ERROR, "Listening port (first parameter) not found");
        return -1;
    }
    int proxy_socket_fd = get_listening_socket_fd("localhost", argv[1]);
    if (proxy_socket_fd == -1) {
        trace(TL_ERROR, "Could not get a listening socket");
        return -1;
    }
    socket_info = malloc(MAX_SOCKET_AMOUNT * sizeof(ConnectionInfo_t));
    if (socket_info == NULL) {
        trace(TL_ERROR, "Allocation %ld bytes in memory", MAX_SOCKET_AMOUNT * sizeof(ConnectionInfo_t));
        return -1;
    }
    return proxy_socket_fd;
}

int connecting(int proxy_socket_fd) {
    int client_connection_socket_fd = accept_connection(proxy_socket_fd);
    if (client_connection_socket_fd == -1)
        return -1;
    int server_connection_socket_fd = connect_to_server(NODE_SERVER, SERVICE_SERVER);
    if (server_connection_socket_fd == -1) {
        close(client_connection_socket_fd);
        return -1;
    }

    sprintf(socket_info[server_connection_socket_fd].info, "connection to server, %s", get_ip_addr(server_connection_socket_fd));
    socket_info[server_connection_socket_fd].fd = client_connection_socket_fd;

    sprintf(socket_info[client_connection_socket_fd].info, "connection to client, %s", get_ip_addr(client_connection_socket_fd));
    socket_info[client_connection_socket_fd].fd = server_connection_socket_fd;

    trace(TL_INFO, "Socket %d: %s", socket_info[server_connection_socket_fd].fd, socket_info[server_connection_socket_fd].info);
    trace(TL_INFO, "Socket %d: %s", socket_info[client_connection_socket_fd].fd, socket_info[client_connection_socket_fd].info);

    return 0;
}

int disconnecting(int socket_fd) {
    int conjugate_socket_fd = socket_info[socket_fd].fd;
    if (close_socket(socket_fd) == -1)
        return -1;
    trace(TL_INFO, "Socket %d (%s) disconnected", socket_fd, socket_info[socket_fd].info);
    if (close_socket(conjugate_socket_fd) == -1)
        return -1;
    trace(TL_INFO, "Socket %d (%s) disconnected", conjugate_socket_fd, socket_info[conjugate_socket_fd].info);
    return 0;
}

int messaging(int socket_fd) {
    char buf[MAX_DATA_SIZE];
    memset(buf, '\0', MAX_DATA_SIZE);
    int conjugate_socket_fd = socket_info[socket_fd].fd;
    if (read(socket_fd, buf, MAX_DATA_SIZE) == -1) {
        trace(TL_ERROR, "Reading from socket %d", socket_fd);
        return -1;
    }
    if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
        trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
        return -1;
    }
    trace(TL_INFO, "Message from socket %d (%s) to socket %d (%s) sent",
            socket_fd, socket_info[socket_fd].info,
            conjugate_socket_fd, socket_info[conjugate_socket_fd].info);
    return 0;
}

void finish() {
    free(socket_info);
}
