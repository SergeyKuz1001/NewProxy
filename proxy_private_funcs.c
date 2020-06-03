#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netdb.h>

#include "proxy_trace.h"

#define MAX_EVENT_AMOUNT 1024
#define MAX_LISTEN_QUEUE_SIZE 100
#define COLORIZATION_OUTPUT

#ifdef COLORIZATION_OUTPUT
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#else
#define COLOR_RESET   ""
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_BLUE    ""
#endif

#define TRACE_END          COLOR_RESET "\n"
#define TRACE_START_OK     COLOR_GREEN "OK: "
#define TRACE_START_INFO   COLOR_BLUE "Info: "
#define TRACE_START_WARN   COLOR_YELLOW "Warning: "
#define TRACE_START_ERROR  COLOR_RED "Error: "

int epoll_fd;
struct epoll_event event;
struct epoll_event events[MAX_EVENT_AMOUNT];

void set_fd_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int proxy_epoll_init() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        trace(TL_ERROR, "Creating epoll");
        return -1;
    }
    event.events = EPOLLIN | EPOLLRDHUP;
    trace(TL_OK, "Epoll initialized");
    return 0;
}

int proxy_epoll_del_fd(int fd) {
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    if (res == -1)
        trace(TL_ERROR, "Deleting fd %d from epoll", fd);
    else
        trace(TL_OK, "Fd %d deleted from epoll", fd);
    return res;
}

int proxy_epoll_add_fd(int fd) {
    event.data.fd = fd;
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if (res == -1)
        trace(TL_ERROR, "Adding fd %d to epoll", fd);
    else
        trace(TL_OK, "Fd %d added to epoll", fd);
    return res;
}

int proxy_epoll_wait() {
    int res = epoll_wait(epoll_fd, events, MAX_EVENT_AMOUNT, -1);
    if (res == -1)
        trace(TL_ERROR, "Waiting events on epoll");
    else
        trace(TL_OK, "Events received on epoll");
    return res;
}

int proxy_epoll_fd(int index) {
    return events[index].data.fd;
}

int proxy_epoll_event_in(int index) {
    return (events[index].events & EPOLLIN);
}

int proxy_epoll_event_rdhup(int index) {
    return (events[index].events & EPOLLRDHUP);
}

void proxy_epoll_finish() {
    close(epoll_fd);
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
