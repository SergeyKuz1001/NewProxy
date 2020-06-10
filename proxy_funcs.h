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

#ifndef PROXY_FUNCS_H
#define PROXY_FUNCS_H

int close_socket(int socket_fd);

int connect_to_server(const char* node, const char* service);

int accept_connection(int accepting_socket_fd);

char* get_ip_addr(int socket_fd);

int get_listening_socket_fd(const char* node, const char* service);

#endif
