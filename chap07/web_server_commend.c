/*
 * MIT License
 *
 * Copyright (c) 2018 Lewis Van Winkle
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "chap07.h"


const char *get_content_type(const char* path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }

    return "application/octet-stream";
}


SOCKET create_socket(const char* host, const char *port) {
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /** ipv4 */
    hints.ai_socktype = SOCK_STREAM; /** TCP */
    hints.ai_flags = AI_PASSIVE; /** 接收任意 ip 连接??? */

    struct addrinfo *bind_address;
    getaddrinfo(host, port, &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) { /** 默认限制 10 个 */
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return socket_listen;
}



#define MAX_REQUEST_SIZE 2047

/** store information on each connected client */
struct client_info {
    socklen_t address_length;
    struct sockaddr_storage address;
    SOCKET socket;
    /** All of the data received from the client so far is stored in the request array */
    char request[MAX_REQUEST_SIZE + 1];
    int received;
    struct client_info *next; /** linked list */
};
 /**
  * 唯一的全局变量
  * 在 web_server2.c 中避免了使用全局变量
  * */
static struct client_info *clients = 0;

/** 
 * takes a SOCKET variable and searches our linked list
 * for the corresponding client_info data structure
 *
 * it can find an existing client_info,
 * or it can create a new client_info
 * */
struct client_info *get_client(SOCKET s) {
    struct client_info *ci = clients;

    while(ci) {
        if (ci->socket == s)
            break;
        ci = ci->next;
    }
    /** 查到就返回 */
    if (ci) return ci;

    /** 查到不到, 就新添加一个结构体节点, 加入链表, 最后返回新增加的节点 */
    /** calloc 申请并初始化内存为0 */
    struct client_info *n =
        (struct client_info*) calloc(1, sizeof(struct client_info));

    if (!n) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    n->address_length = sizeof(n->address);
    /** 这里是往表头插入节点 */
    n->next = clients;
    clients = n; /** 这里要更新表头 */

    /** 返回新节点, 用于填充对应数据 */
    return n;
}

/**
 * closes the connection to a client and removes it from the clients linked list
 * */
void drop_client(struct client_info *client) {
    CLOSESOCKET(client->socket);

    /** 这里用了双指针, 便于操作 */
    struct client_info **p = &clients;

    while(*p) {
        if (*p == client) {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
    }

    fprintf(stderr, "drop_client not found.\n");
    exit(1);
}

/** returns a client's IP address as a string (character array) */
const char *get_client_address(struct client_info *ci) {
    /**
     * This char array is declared static, which ensures that its memory is available after the function
     * returns. This means that we don't need to worry about having the caller free() the memory. The
     * downside to this method is that get_client_address() has a global state and is not
     * re-entrant-safe. See web_server2.c for an alternative version that is re-entrant-safe
     * */
    static char address_buffer[100]; /** Note: static */

    getnameinfo((struct sockaddr*)&ci->address,
            ci->address_length,
            address_buffer, sizeof(address_buffer), 0, 0,
            NI_NUMERICHOST);
    return address_buffer; /** Note: return static 变量 */
}

/**
 * blocks until an existing client sends data, or a new client attempts to connect
 *
 * uses the select() function to wait until either a client has data available
 * or a new client is attempting to connect
 * */
fd_set wait_on_clients(SOCKET server) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    SOCKET max_socket = server;

    struct client_info *ci = clients;

    /**
     * 每次新连接一个 client, 都要遍历所有的已连接 sock, 查找 max sock,
     * 以便使用 select 监听
     *
     * 这个能否简化(优化)??
     * 1. 专门记录当前最大的 sock 值, 进连接的 client sock 仅仅需要和这个记录值比较就行. 
     *    节省中间变量的时间和内存开销, 避免每次遍历
     * 2. 不考虑和 windows 系统的兼容, 使用 poll/epoll 接口实现
     * */
    while(ci) {
        FD_SET(ci->socket, &reads);
        if (ci->socket > max_socket)
            max_socket = ci->socket;
        ci = ci->next;
    }

    /** block in select */
    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return reads;
}

/** handle HTTP error conditions */
void send_400(struct client_info *client) {
    const char *c400 = "HTTP/1.1 400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Length: 11\r\n\r\nBad Request";
    send(client->socket, c400, strlen(c400), 0);
    drop_client(client); /** 断开连接 */
}

/** handle HTTP error conditions */
void send_404(struct client_info *client) {
    const char *c404 = "HTTP/1.1 404 Not Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\nNot Found";
    send(client->socket, c404, strlen(c404), 0);
    drop_client(client); /** 断开连接 */
}

/**
 * attempts to transfer a file to a connected client
 *
 * Our server expects all hosted files to be in a subdirectory called public.
 * Ideally, our server should not allow access to any files outside of this public directory
 *
 * Keep in mind that while serve_resource() attempts to limit access to only the public directory,
 * it is not adequate in doing so,
 * and serve_resource() should not be used in production code
 * without carefully considering additional access loopholes
 *
 * */
void serve_resource(struct client_info *client, const char *path) {
    /** debugging 信息.
     * 可能还会有其他信息. 例如:
     *      the date, time, request method,
     *      the client's user-agent string,
     *      and the response code as a minimum
     * */
    printf("serve_resource %s %s\n", get_client_address(client), path);

    /** '/' 路径, 默认 ‘/index.html' 文件 */
    if (strcmp(path, "/") == 0) path = "/index.html";

    /** 限制文件路径长度. 这里过滤后, 后面就不需要考虑路径长度溢出问题 */
    if (strlen(path) > 100) {
        send_400(client);
        return;
    }

    if (strstr(path, "..")) { /** 禁止访问上级目录, 资源保护 */
        send_404(client);
        return;
    }

    char full_path[128]; /** 这里不用考虑 path 长度太长问题. 前面已经过滤了 */
    sprintf(full_path, "public%s", path); /** 只能访问 public 目录文件 */

/** 处理 windows 上路径是反斜杠 */
#if defined(_WIN32)
    char *p = full_path;
    while (*p) {
        if (*p == '/') *p = '\\';
        ++p;
    }
#endif
    /**
     * Why not use open ??
     * */
    FILE *fp = fopen(full_path, "rb");

    if (!fp) {
        send_404(client);
        return;
    }

    fseek(fp, 0L, SEEK_END); /** 将文件指针指向文件结尾 */
    size_t cl = ftell(fp); /** 获取文件指针到文件开头的距离, 得出文件大小 */
    rewind(fp); /** 将文件指针指向文件开头 */
	/** rewind 的作用和 (void) fseek(stream, 0L, SEEK_SET) 一样 */

    const char *ct = get_content_type(full_path);

#define BSIZE 1024
    char buffer[BSIZE];

    /** 根据 http 协议发送 header 和 body 部分 */
    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Connection: close\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Length: %lu\r\n", cl);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Type: %s\r\n", ct);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    /** 一直从文件读取, 然后马上发送, 知道无数据可读 */
    int r = fread(buffer, 1, BSIZE, fp);
    while (r) {
        /**
         * Note that send() may block on large files.
         * In a truly robust, production-ready server, you would need to handle this case.
         * It could be done by using select() to determine when each socket is ready to read.
         * Another common method is to use fork()
         * or similar APIs to create separate threads/processes for each connected client.
         *
         * our server accepts the limitation that send() blocks on large files.
         * Please refer to Chapter 13, Socket Programming Tips
         * and Pitfalls, for more information about the blocking behavior of send().
         * */
        send(client->socket, buffer, r, 0);
        r = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
    drop_client(client);
}


int main() {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

//#define ONLY_LOCAL_CAN_CONNECT

#ifndef ONLY_LOCAL_CAN_CONNECT
    // 任意 ip 都可以连接
    SOCKET server = create_socket(0, "8080");
#else
    // accept connections from only the local system, and not outside systems
    SOCKET server = create_socket("127.0.0.1", "8080");
#endif

    while(1) {

        fd_set reads;
        reads = wait_on_clients(server);

        if (FD_ISSET(server, &reads)) {
            /** 有新连接, 申请新节点,保存 client 信息 */
            struct client_info *client = get_client(-1);

            client->socket = accept(server,
                    (struct sockaddr*) &(client->address),
                    &(client->address_length));

            if (!ISVALIDSOCKET(client->socket)) {
                fprintf(stderr, "accept() failed. (%d)\n",
                        GETSOCKETERRNO());
                return 1;
            }


            printf("New connection from %s.\n",
                    get_client_address(client));
        }


        struct client_info *client = clients;
        while(client) { /** 循环遍历 client */
            /**
             * 这里每次只接收一次 recv 数据, 应该还可以改进:
             *   1. 如果某个 client recv 一次, 并不能接收完 sock 数据, 就需要再遍历一轮所有 client 后再回来接收, 浪费 cpu 资源和时间
             *   2. 如果每个 client 都有数据可读, 能够满足性能需求
             * */
            struct client_info *next = client->next;

            if (FD_ISSET(client->socket, &reads)) { /** 检查是否是当前可读的 client */

                /** 检查缓存是否满, 如果缓存满了就报错 */
                if (MAX_REQUEST_SIZE == client->received) {
                    send_400(client);
                    client = next;
                    continue;
                }

                int r = recv(client->socket,
                        client->request + client->received,
                        MAX_REQUEST_SIZE - client->received, 0); /** 接收缓存剩余空间大小的数据,防止缓存溢出 */

                if (r < 1) {
                    printf("Unexpected disconnect from %s.\n",
                            get_client_address(client));
                    drop_client(client);

                } else {
                    client->received += r; /** 更新缓存结尾位置 */
                    /** allows us to use strstr() to search the buffer, as the null terminator tells strstr() when to stop */
                    client->request[client->received] = 0; /** 缓存结尾位置置'0', 字符串结束符标志 */

                    /**
                     * 查找 header 结束位置.
                     * 用于表征是否含有有效 header 部分
                     *
                     * 如果没有的话, 有可能:
                     *   1. header 还没有接收完, 继续接收完再解析
                     *   2. http 数据包不完整, header 有问题
                     * */
                    char *q = strstr(client->request, "\r\n\r\n");
                    if (q) {
                        *q = 0;
                        /** 
                         * header GET 部分实例:
                         *    'GET /page2.html HTTP/1.1\r\n'
                         * 其中不包含两头的单引号
                         * */
                        if (strncmp("GET /", client->request, 5)) { /** 检查是否是 GET 类型. 目前仅仅接受 GET 类型 */
                            send_400(client);
                        } else {
                            char *path = client->request + 4;
                            /**
                             * The end of the requested path is indicated by finding the next space character
                             * 为什么是下一个空格是路径结尾? 根据 GET 部分的协议, 用空格分隔的
                             * */
                            char *end_path = strstr(path, " ");
                            if (!end_path) {
                                send_400(client);
                            } else {
                                *end_path = 0; /** 将 path 结尾置'0', 使得可以使用字符串函数处理 path */
                                serve_resource(client, path);
                            }
                        }
                    } //if (q)
                }
            }

            client = next; /** 更新 client, 处理下一个 client */
        }

    } //while(1)


    printf("\nClosing socket...\n");
    CLOSESOCKET(server);


#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}

