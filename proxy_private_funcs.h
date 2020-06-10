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

#endif
