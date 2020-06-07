#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <algorithm>

#include "proxy_funcs.h"
#include "proxy_trace.h"

#define MAX_SOCKET_AMOUNT 300
#define MAX_DATA_SIZE 256
#define MAX_LOGIN_SIZE 512
#define MAX_SHORT_INFO_SIZE 64
#define NODE_SERVER "localhost" 
#define SERVICE_SERVER "7000"
#define AMOUNT_ROOMS 2

using namespace std;

typedef enum {
    CLIENT_ENTER_LOGIN,
    CLIENT_ENTER_PASSWORD,
    CLIENT_ENTER_TEXT,
    SERVER
} SocketState_t;

typedef enum {
    COMMON_ROOM,
    ALPHA_ROOM
} ClientState_t;

typedef struct {
    char login[MAX_LOGIN_SIZE];
    unsigned int first_not_reading_messages[AMOUNT_ROOMS];
    unsigned int first_reading_messages[AMOUNT_ROOMS];
    ClientState_t state;
} ClientInfo_t;

typedef struct {
    int fd;
    SocketState_t state;
    int client_id;
    char* entering_login;
    char info[MAX_SHORT_INFO_SIZE];
} ConnectionInfo_t;

typedef struct {
    int author_id;
    char* text;
} MessageInfo_t;

vector<MessageInfo_t> messages[AMOUNT_ROOMS];
vector<ClientInfo_t> clients;
ConnectionInfo_t* socket_info;

int init(int argc, char* argv[]) {
    trace(TL_INFO, "NC-proxy");
    if (argc < 2) {
        trace(TL_ERROR, "Listening port (first parameter) not found");
        return -1;
    }
    int proxy_socket_fd = get_listening_socket_fd("localhost", argv[1]);
    if (proxy_socket_fd == -1) {
        trace(TL_ERROR, "Could not get a listening socket");
        return -1;
    }
    socket_info = (ConnectionInfo_t*) malloc(MAX_SOCKET_AMOUNT * sizeof(ConnectionInfo_t));
    if (socket_info == NULL) {
        trace(TL_ERROR, "Allocation %ld bytes in memory", MAX_SOCKET_AMOUNT * sizeof(ConnectionInfo_t));
        return -1;
    }
    return proxy_socket_fd;
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

    socket_info[server_connection_socket_fd].fd = client_connection_socket_fd;
    socket_info[server_connection_socket_fd].state = SERVER;
    socket_info[server_connection_socket_fd].client_id = -1;
    socket_info[server_connection_socket_fd].entering_login = NULL;
    sprintf(socket_info[server_connection_socket_fd].info, "connection to server, %s", get_ip_addr(server_connection_socket_fd));

    socket_info[client_connection_socket_fd].fd = server_connection_socket_fd;
    socket_info[client_connection_socket_fd].state = CLIENT_ENTER_TEXT;
    socket_info[client_connection_socket_fd].client_id = -1;
    socket_info[client_connection_socket_fd].entering_login = NULL;
    sprintf(socket_info[client_connection_socket_fd].info, "connection to client, %s", get_ip_addr(client_connection_socket_fd));

    trace(TL_INFO, "Socket %d: %s", socket_info[server_connection_socket_fd].fd, socket_info[server_connection_socket_fd].info);
    trace(TL_INFO, "Socket %d: %s", socket_info[client_connection_socket_fd].fd, socket_info[client_connection_socket_fd].info);

    return 0;
}

int disconnecting(int socket_fd) {
    int conjugate_socket_fd = socket_info[socket_fd].fd;
    if (socket_info[socket_fd].client_id != -1) {
        int room_id = clients[socket_info[socket_fd].client_id].state;
        clients[socket_info[socket_fd].client_id].first_not_reading_messages[room_id] = messages[room_id].size();
        clients[socket_info[socket_fd].client_id].first_reading_messages[room_id] = -1;
    }
    if (close_socket(socket_fd) == -1)
        return -1;
    trace(TL_INFO, "Socket %d (%s) disconnected", socket_fd, socket_info[socket_fd].info);
    if (socket_info[conjugate_socket_fd].client_id != -1) {
        int room_id = clients[socket_info[conjugate_socket_fd].client_id].state;
        clients[socket_info[conjugate_socket_fd].client_id].first_not_reading_messages[room_id] = messages[room_id].size();
        clients[socket_info[conjugate_socket_fd].client_id].first_reading_messages[room_id] = -1;
    }
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
    if (socket_info[socket_fd].state == SERVER && socket_info[conjugate_socket_fd].state == CLIENT_ENTER_LOGIN) {
        if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
            trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
            return -1;
        }
    }
    else if (socket_info[socket_fd].state == SERVER && socket_info[conjugate_socket_fd].state == CLIENT_ENTER_PASSWORD) {
        if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
            trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
            return -1;
        }
    }
    else if (socket_info[socket_fd].state == SERVER && socket_info[conjugate_socket_fd].state == CLIENT_ENTER_TEXT) {
        if (socket_info[conjugate_socket_fd].entering_login != NULL) {
            if (buf[0] == '\n' && (buf[1] == 'C' || buf[1] == 'S')) {
                int client_index;
                for (client_index = 0; client_index < clients.size(); client_index++) {
                    if (strcmp(clients[client_index].login, socket_info[conjugate_socket_fd].entering_login) == 0)
                        break;
                }
                if (client_index == clients.size()) {
                    ClientInfo_t new_client;
                    strcpy(new_client.login, socket_info[conjugate_socket_fd].entering_login);
                    for (int i = 0; i < AMOUNT_ROOMS; i++) {
                        new_client.first_not_reading_messages[i] = 0;
                        new_client.first_reading_messages[i] = -1;
                    }
                    new_client.state = COMMON_ROOM;
                    clients.push_back(new_client);
                }
                else {
                    clients[client_index].state = COMMON_ROOM;
                }
                socket_info[conjugate_socket_fd].client_id = client_index;
            }
            free(socket_info[conjugate_socket_fd].entering_login);
            socket_info[conjugate_socket_fd].entering_login = NULL;
        }
        if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
            trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
            return -1;
        }
    }
    else if (socket_info[socket_fd].state == CLIENT_ENTER_LOGIN && socket_info[conjugate_socket_fd].state == SERVER) {
        socket_info[socket_fd].entering_login = strndup(buf, strlen(buf) - 1);
        socket_info[socket_fd].state = CLIENT_ENTER_PASSWORD;
        if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
            trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
            return -1;
        }
    }
    else if (socket_info[socket_fd].state == CLIENT_ENTER_PASSWORD && socket_info[conjugate_socket_fd].state == SERVER) {
        socket_info[socket_fd].state = CLIENT_ENTER_TEXT;
        if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
            trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
            return -1;
        }
    }
    else if (socket_info[socket_fd].state == CLIENT_ENTER_TEXT && socket_info[conjugate_socket_fd].state == SERVER) {
        if (buf[0] == '/') {
            if (strcmp(buf, "/login\n") == 0 || strcmp(buf, "/signup\n") == 0) {
                if (socket_info[socket_fd].client_id == -1) {
                    socket_info[socket_fd].state = CLIENT_ENTER_LOGIN;
                }
                if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
                    trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
                    return -1;
                }
            }
            else if (strcmp(buf, "/logout\n") == 0) {
                socket_info[socket_fd].client_id = -1;
                if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
                    trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
                    return -1;
                }
            }
            else if (strcmp(buf, "/connect common\n") == 0) {
                if (socket_info[socket_fd].client_id != -1 && clients[socket_info[socket_fd].client_id].state == ALPHA_ROOM) {
                    clients[socket_info[socket_fd].client_id].first_not_reading_messages[ALPHA_ROOM] = messages[ALPHA_ROOM].size();
                    clients[socket_info[socket_fd].client_id].first_reading_messages[ALPHA_ROOM] = -1;
                    clients[socket_info[socket_fd].client_id].first_reading_messages[COMMON_ROOM] = messages[COMMON_ROOM].size();
                    clients[socket_info[socket_fd].client_id].state = COMMON_ROOM;
                }
                if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
                    trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
                    return -1;
                }
            }
            else if (strcmp(buf, "/connect alpha\n") == 0) {
                if (socket_info[socket_fd].client_id != -1 && clients[socket_info[socket_fd].client_id].state == COMMON_ROOM) {
                    clients[socket_info[socket_fd].client_id].first_not_reading_messages[COMMON_ROOM] = messages[COMMON_ROOM].size();
                    clients[socket_info[socket_fd].client_id].first_reading_messages[COMMON_ROOM] = -1;
                    clients[socket_info[socket_fd].client_id].first_reading_messages[ALPHA_ROOM] = messages[ALPHA_ROOM].size();
                    clients[socket_info[socket_fd].client_id].state = ALPHA_ROOM;
                }
                if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
                    trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
                    return -1;
                }
            }
            else if (strcmp(buf, "/help\n") == 0) {
                if (dprintf(socket_fd, "\nList of commands:\n/help - show this list\n/roomlist - list of all available chatrooms\n/connect <to> - connect to chatroom\n/new_messages - not reading messages from all chatrooms\n/logout - change the profile\n\n") < 0) {
                    trace(TL_ERROR, "Writting to socket %d", socket_fd);
                    return -1;
                }
            }
            else if (strcmp(buf, "/new_messages\n") == 0) {
                char* room_names[AMOUNT_ROOMS];
                room_names[0] = strdup("common");
                room_names[1] = strdup("alpha");
                for (int i = 0; i < AMOUNT_ROOMS; i++) {
                    dprintf(socket_fd, "New messages in %s room:\n", room_names[i]);
                    for (int j = clients[socket_info[socket_fd].client_id].first_not_reading_messages[i];
                            j < clients[socket_info[socket_fd].client_id].first_reading_messages[i] && j < messages[i].size(); j++) {
                        dprintf(socket_fd, "%s:%s", clients[messages[i][j].author_id].login, messages[i][j].text);
                    }
                    dprintf(socket_fd, "\n");
                    clients[socket_info[socket_fd].client_id].first_not_reading_messages[i] = messages[i].size();
                    if (clients[socket_info[socket_fd].client_id].state == i)
                        clients[socket_info[socket_fd].client_id].first_reading_messages[i] = messages[i].size();
                    else
                        clients[socket_info[socket_fd].client_id].first_reading_messages[i] = -1;
                }
            }
            else {
                if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
                    trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
                    return -1;
                }
            }
        }
        else {
            if (socket_info[socket_fd].client_id != -1) {
                int room_id = clients[socket_info[socket_fd].client_id].state;
                MessageInfo_t new_message;
                new_message.author_id = socket_info[socket_fd].client_id;
                new_message.text = strdup(buf);
                messages[room_id].push_back(new_message);
            }
            if (dprintf(conjugate_socket_fd, "%s", buf) < 0) {
                trace(TL_ERROR, "Writting to socket %d", conjugate_socket_fd);
                return -1;
            }
        }
    }
    return 0;
}

void finish() {
    free(socket_info);
}
