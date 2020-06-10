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

#include "proxy_private_funcs.h"
#include "proxy_trace.h"
#include "user_funcs.h"

int main(int argc, char* argv[]) {
    trace(TL_OK, "Proxy-server started...");
    if (proxy_epoll_init() == -1) {
        proxy_epoll_finish();
        trace(TL_ERROR, "Epoll was not initialize");
        return -1;
    }
    int proxy_socket_fd = init(argc, argv);
    if (proxy_socket_fd == -1) {
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
