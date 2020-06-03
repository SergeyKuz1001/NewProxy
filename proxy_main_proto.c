#include <stdio.h>

#include "proxy_private_funcs.h"
#include "proxy_trace.h"
#include "user_funcs.h"

#define NODE_PROXY <host>
#define SERVICE_PROXY <port>

int main(int argc, char* argv[]) {
    trace(TL_OK, "Proxy-server started...");
    if (proxy_epoll_init() == -1) {
        proxy_epoll_finish();
    }
    int proxy_socket_fd = get_listening_socket_fd(NODE_PROXY, SERVICE_PROXY);
    if (proxy_socket_fd == -1) {
        proxy_epoll_finish();
        return -1;
    }
    if (init(argc, argv) == -1) {
        finish();
        proxy_epoll_finish();
        return -1;
    }
    while(1) {
        int amount_events = proxy_epoll_wait();
        if (amount_events == -1)
            break;
        trace(TL_INFO, "%d events got", amount_events);
        for (int i = 0; i < amount_events; i++) {
            int socket_fd = proxy_epoll_fd(i);
            if (socket_fd == proxy_socket_fd) {
                if (connecting(socket_fd) == -1) {
                    trace(TL_ERROR, "Connecting(socket_fd = %d)", socket_fd);
                    continue;
                }
                trace(TL_OK, "Connecting was successful");
            } else {
                if (proxy_epoll_event_in(i)) {
                    if (messaging(socket_fd) == -1) {
                        trace(TL_ERROR, "Messaging(socket_fd = %d)", socket_fd);
                        continue;
                    }
                    trace(TL_OK, "Messaging from socket %d was successful", socket_fd);
                }
                if (proxy_epoll_event_rdhup(i)) {
                    if (disconnecting(socket_fd) ==-1) {
                        trace(TL_ERROR, "Disconnecting(socket_fd = %d)", socket_fd);
                        continue;
                    }
                    trace(TL_OK, "Disconnecting for socket %d was successful", socket_fd);
                }
            }
        }
    }
    finish();
    proxy_epoll_finish();
    return 0;
}
