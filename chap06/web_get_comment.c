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

#include "chap06.h"

#define TIMEOUT 5.0

/**
 * 解析 URL
 *
 * 会修改 url 字符串, 将其分割成多个字符串
 *
 * http://www.example.com:80/res/page1.php?user=bob#account
 * 经过处理后:
 * 'http0//www.example.com080/res/page1.php0user=bob0account'
 * ':' --> 0
 * hostname, port, path 分别指向了 url 字符串中的不同位置, 不需要额外申请内存
 *
 * */
void parse_url(char *url, char **hostname, char **port, char** path) {
    printf("URL: %s\n", url);

    char *p;
/**
 * strstr() is called to search for :// in the URL.
 * If it is not found, then protocol is left at 0,
 * and p is set to point back to the beginning of the URL
 * */
    p = strstr(url, "://"); // 查找 '://', 查找是否包含协议

    char *protocol = 0;
    if (p) { /** 含有 '://' 的情况 */
        protocol = url; /** protocol 指向 URL 开头处 */
        *p = 0; /** 'http://' > 'http0//'   *p = 0 用于分割字符串(字符串结束标识符), 避免内存申请 */
        p += 3; /** p 指向 '://' 之后, 也就是 hostname 开始处 */
    } else {
        p = url; /** p 指向 URL 开头处 */
    }

    if (protocol) { /** 含有 '://' 的情况, 提取协议名称 */
        if (strcmp(protocol, "http")) { /** 目前仅仅支持 'http' 协议, 不是该协议的就直接退出 */
            fprintf(stderr,
                    "Unknown protocol '%s'. Only 'http' is supported.\n",
                    protocol);
            exit(1);
        }
    }

    *hostname = p; /** 经过上面的处理, p 始终指向 hostname 处了 */
    /**
     * 查找 hostname 的结尾处, 有一下情况:
     *     'example.com:80'         端口号
     *     'example.com/path'       路径
     *     'example.com#index'      hash
     * */
    while (*p && *p != ':' && *p != '/' && *p != '#') ++p; /** 处理之后, p 指向 hostname 末尾处 */

    *port = "80"; /** 端口默认 80 */
    if (*p == ':') {
        *p++ = 0; /** *p = 0 用于分割字符串(字符串结束标识符), 避免内存申请 */
        *port = p; /** 默认修改为 URL 中指定的端口 */
    }
    while (*p && *p != '/' && *p != '#') ++p; /** 处理之后, p 指向 path 字段的开始处 */

    *path = p; /** path */
    if (*p == '/') {
        *path = p + 1; /** 如果第一个是 '/' 则忽略掉第一个 '/' */
    }
    *p = 0; /** *p = 0 用于分割字符串(字符串结束标识符), 避免内存申请 */

    //while (*p && *p != '#') ++p;
    //上面注释的语句, 不能在 '#' 处结束 while 循环, 不能正确分割出 path 和 hash
    while (*p != '0' && *p != '#') ++p; /** 处理 '#' 分割, 处理完后 p 指向 path 的结尾 */
    /**
     * 如果有 '#' 分割, 需要修改'#' 为 '0' 当作 path 字符串的结尾.
     * *p = 0 用于分割字符串(字符串结束标识符), 避免内存申请
     * */
    char *hash = 0;
    if (*p == '#') {
        *p = 0;
        hash = p + 1;

        // 后面这里不需要再处理, URL 字符串的结尾本身就是 '0'
        //while(*p) ++p;
        //*p = 0;
    }

    /** 最后不处理 hash 部分, 这部分不需要提交服务器, 只是用于浏览器将显示定位到该标签处而已 */


    printf("protocol:%s\n", protocol);
    printf("hostname: %s\n", *hostname);
    printf("port: %s\n", *port);
    printf("path: %s\n", *path);
    printf("hash: %s\n\n", hash);
}

/** 
 * 填充 http 协议头
 * 发送 http request GET 请求
 * */
void send_request(SOCKET s, char *hostname, char *port, char *path) {
    char buffer[2048];

    sprintf(buffer, "GET /%s HTTP/1.1\r\n", path);
    sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "User-Agent: honpwc web_get 1.0\r\n");
    sprintf(buffer + strlen(buffer), "\r\n"); // 根据协议 最后要用 '\r\n' 空一行

    send(s, buffer, strlen(buffer), 0); /** 发送 http GET (over TCP) 请求 */
    printf("Sent Headers:\n%s", buffer);
}


SOCKET connect_to_host(char *hostname, char *port) {
    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM; /** http over TCP */
    struct addrinfo *peer_address;
    if (getaddrinfo(hostname, port, &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET server;
    server = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(server)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    printf("Connecting...\n");
    if (connect(server,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    freeaddrinfo(peer_address);

    printf("Connected.\n\n");

    return server;
}



int main(int argc, char *argv[]) {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif


    if (argc < 2) {
        fprintf(stderr, "usage: ./web_get url\n");
        printf("\teg:\n\t\t./web_get http://www.example.com:80/res/page1.php?user=bob#account\n");
        return 1;
    }
    char *url = argv[1];

    char *hostname, *port, *path;
    parse_url(url, &hostname, &port, &path);

    SOCKET server = connect_to_host(hostname, port);
    send_request(server, hostname, port, path);

    const clock_t start_time = clock();

/**
 * RESPONSE_SIZE is the maximum size of the HTTP response we reserve memory for
 *
 * response is a character array that holds the entire HTTP response
 * p is a char pointer that keeps track of how far we have written into response so far
 * q is an additional char pointer that is used later
 * end as a char pointer, which points to the end of the response buffer
 * body pointer is used to remember the beginning of the HTTP response body once received
 *
 * */
#define RESPONSE_SIZE 32768
    char response[RESPONSE_SIZE+1];
    char *p = response, *q;
    char *end = response + RESPONSE_SIZE;
    char *body = 0;

    enum {length, chunked, connection};
    int encoding = 0; /** store the actual method used */
    int remaining = 0; /** record how many bytes are still needed to finish the HTTP body or body chunk */

    while(1) {

        if ((clock() - start_time) / CLOCKS_PER_SEC > TIMEOUT) {
            fprintf(stderr, "timeout after %.2f seconds\n", TIMEOUT);
            return 1;
        }

        if (p == end) {
            fprintf(stderr, "out of buffer space\n");
            return 1;
        }

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(server, &reads);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        /** select 使用 timeout 配置 */
        if (select(server+1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        if (FD_ISSET(server, &reads)) {
            int bytes_received = recv(server, p, end - p, 0); /** 接收响应 */
            if (bytes_received < 1) { /** 0 是 timeout, <0 是 error */
                if (encoding == connection && body) {
                    printf("%.*s", (int)(end - body), body);
                }

                printf("\nConnection closed by peer.\n");
                break;
            }

            /*printf("Received (%d bytes): '%.*s'",
                    bytes_received, bytes_received, p);*/

            p += bytes_received; /** p 指向了接收的结尾处, 每次接收的 bytes 不确定 */
            *p = 0;

            /** 查找 http 头的结尾 ‘\r\n\r\n' */
            if (!body && (body = strstr(response, "\r\n\r\n"))) {
                *body = 0; /** 置为字符串结尾处, 用来根个 http header 和 body */
                body += 4; /** body 指向实际的 http body 开头处 */

                /** 上面用 '0' 分割了 http 响应, 所以可以直接打印 http header */
                printf("Received Headers:\n%s\n", response);

                /** 查找长度，判断 encoding 类型  */
                q = strstr(response, "\nContent-Length: ");
                if (q) {
                    encoding = length; /** 确定 encoding 方式为 length */
                    q = strchr(q, ' '); /** q 跳转到 "\nContent-Length: " 的空格位置 */
                    q += 1; /** q 指向 length 的数字大小处 */
                     /** 剩余数据大小. 将数字字符串转换成十进制数字 */
                    remaining = strtol(q, 0, 10);

                } else {
                    q = strstr(response, "\nTransfer-Encoding: chunked");
                    if (q) {
                        encoding = chunked; /** 确定 encoding 方式为 chunked */
                        remaining = 0; /** 剩余数据大小为 0 */
                    } else {
                        encoding = connection; /** 确定 encoding 方式为 connection */
                    }
                }
                printf("\nReceived Body:\n");
            }

            // 处理 http body
            if (body) {
                if (encoding == length) {
                    if (p - body >= remaining) { /** 接收结束, 打印后退出 while 接收循环 */
                        printf("%.*s", remaining, body);
                        break;
                    }
                } else if (encoding == chunked) {
                    do {
                        if (remaining == 0) {
                            if ((q = strstr(body, "\r\n"))) {
                                remaining = strtol(body, 0, 16);
                                if (!remaining) goto finish;
                                body = q + 2;
                            } else {
                                break;
                            }
                        }
                        if (remaining && p - body >= remaining) {
                            printf("%.*s", remaining, body);
                            body += remaining + 2;
                            remaining = 0;
                        }
                    } while (!remaining);
                }
            } //if (body)
        } //if FDSET
    } //end while(1)
finish:

    printf("\nClosing socket...\n");
    CLOSESOCKET(server);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}

