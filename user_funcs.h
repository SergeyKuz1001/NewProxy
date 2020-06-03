#ifndef USER_FUNCS_H
#define USER_FUNCS_H

int init(int argc, char* argv[]);

int connecting(int proxy_socket_fd);

int disconnecting(int socket_fd);

int messaging(int socket_fd);

void finish();

#endif
