#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "proxy_private_funcs.h"
#include "proxy_trace.h"

#define MAX_LISTEN_QUEUE_SIZE 100

int close_socket(int socket_fd) {
    if (proxy_epoll_del_fd(socket_fd) == -1) {
        trace(TL_ERROR, "Deleting socket %d from epoll", socket_fd);
        return -1;
    }
    if (close(socket_fd) == -1) {
        trace(TL_ERROR, "Closing socket %d", socket_fd);
        return -1;
    }
    trace(TL_OK, "Socket %d closed", socket_fd);
    return 0;
}

int connect_to_server(const char* node, const char* service) {
    int socket_fd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *ptr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(node, service, &hints, &servinfo) != 0) {
        trace(TL_ERROR, "Failing getaddrinfo(%s, %s)", node, service);
        return -1;
    }

    for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
        socket_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket_fd == -1)
            continue;
        if (connect(socket_fd, ptr->ai_addr, ptr->ai_addrlen) != -1)
            break;
        close(socket_fd);
    }

    freeaddrinfo(servinfo);
    if (ptr == NULL) {
        trace(TL_ERROR, "Finding correct socket");
        return -1;
    }
    set_fd_nonblocking(socket_fd);
    if (proxy_epoll_add_fd(socket_fd) == -1)
        return -1;
    trace(TL_OK, "Connected to server on socket %d", socket_fd);
    return socket_fd;
}

int accept_connection(int accepting_socket_fd) {
    int socket_fd = accept(accepting_socket_fd, NULL, NULL);
    if (socket_fd == -1) {
        trace(TL_ERROR, "Accepting connection on socket %d", accepting_socket_fd);
        return -1;
    }
    set_fd_nonblocking(socket_fd);
    if (proxy_epoll_add_fd(socket_fd) == -1)
        return -1;
    trace(TL_OK, "Connection on socket %d accepted", socket_fd);
    return socket_fd;
}

void* get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* get_ip_addr(int socket_fd) {
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    if (getpeername(socket_fd, (struct sockaddr *)&addr, &addrlen) == -1) {
        trace(TL_ERROR, "Getting peer name from socket %d", socket_fd);
        return NULL;
    }
    char* ip_addr = (char*) malloc(INET6_ADDRSTRLEN * sizeof(char));
    if (inet_ntop(addr.ss_family, get_in_addr((struct sockaddr *)&addr), ip_addr, INET6_ADDRSTRLEN * sizeof(char)) == NULL) {
        trace(TL_ERROR, "Converting address to text form");
        return NULL;
    }
    trace(TL_OK, "IP address for socket %d got", socket_fd);
    return ip_addr;
}

int get_listening_socket_fd(const char* node, const char* service) {
    int socket_fd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *ptr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(node, service, &hints, &servinfo) != 0) {
        trace(TL_ERROR, "Failing getaddrinfo(%s, %s, ...)", node, service);
        return -1;
    }

    for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
        socket_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket_fd == -1)
            continue;
        if (bind(socket_fd, ptr->ai_addr, ptr->ai_addrlen) != 0)
            continue;
        if (listen(socket_fd, MAX_LISTEN_QUEUE_SIZE) != -1)
            break;
        close(socket_fd);
    }

    freeaddrinfo(servinfo);
    if (ptr == NULL) {
        trace(TL_ERROR, "Finding correct socket");
        return -1;
    }
    set_fd_nonblocking(socket_fd);
    if (proxy_epoll_add_fd(socket_fd) == -1) {
        trace(TL_ERROR, "Adding socket %d in epoll", socket_fd);
        return -1;
    }
    trace(TL_OK, "Listening socket %d got", socket_fd);
    return socket_fd;
}
