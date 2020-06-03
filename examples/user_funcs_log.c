#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "proxy_funcs.h"
#include "proxy_trace.h"

#define MAX_SOCKET_AMOUNT 300
#define MAX_DATA_SIZE 256
#define MAX_LOG_MESSAGE_SIZE 512
#define MAX_SHORT_INFO_SIZE 64
#define NODE_SERVER "localhost" 
#define SERVICE_SERVER "7000"

typedef struct {
    int fd;
    char info[MAX_SHORT_INFO_SIZE];
} ConnectionInfo_t;

ConnectionInfo_t *socket_info;
int log_file_fd;

int init(int argc, char* argv[]) {
    trace(TL_INFO, "LOG-proxy");
    socket_info = malloc(MAX_SOCKET_AMOUNT * sizeof(ConnectionInfo_t));
    if (socket_info == NULL) {
        trace(TL_ERROR, "Allocation %ld bytes in memory", MAX_SOCKET_AMOUNT * sizeof(ConnectionInfo_t));
        return -1;
    }
    log_file_fd = open("messages.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR);
    return 0;
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
    char log_message[MAX_LOG_MESSAGE_SIZE];
    sprintf(log_message, "From: socket %d (%s)\nTo:   socket %d (%s)\nText: [%s]\n\n",
            socket_fd, socket_info[socket_fd].info,
            conjugate_socket_fd, socket_info[conjugate_socket_fd].info,
            buf);
    if (write(log_file_fd, log_message, strlen(log_message)) == -1) {
        trace(TL_ERROR, "Writting on log file on fd %d", log_file_fd);
        return -1;
    }
    trace(TL_INFO, "Message from socket %d (%s) to socket %d (%s) sent",
            socket_fd, socket_info[socket_fd].info,
            conjugate_socket_fd, socket_info[conjugate_socket_fd].info);
    return 0;
}

void finish() {
    free(socket_info);
    close(log_file_fd);
}
