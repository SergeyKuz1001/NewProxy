#ifndef PROXY_FUNCS_H
#define PROXY_FUNCS_H

int close_socket(int socket_fd);

int connect_to_server(const char* node, const char* service);

int accept_connection(int accepting_socket_fd);

char* get_ip_addr(int socket_fd);

#endif
