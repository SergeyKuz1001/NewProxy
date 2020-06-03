#ifndef PROXY_PRIVATE_FUNCS_H
#define PROXY_PRIVATE_FUNCS_H

void set_fd_nonblocking(int fd);

int proxy_epoll_init();

int proxy_epoll_del_fd(int fd);

int proxy_epoll_add_fd(int fd);

int proxy_epoll_wait();

int proxy_epoll_fd(int index);

int proxy_epoll_event_in(int index);

int proxy_epoll_event_rdhup(int index);

void proxy_epoll_finish();

int get_listening_socket_fd(const char* node, const char* service);

#endif
