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

/*
 邮件 client/server 交互模拟:
    S: 220 mail.example.com SMTP server ready
    C: HELO mail.example.net
    S: 250 Hello mail.example.net [192.0.2.67]
    C: MAIL FROM:<alice@example.net>
    S: 250 OK
    C: RCPT TO:<bob@example.com>
    S: 250 Accepted
    C: DATA
    S: 354 Enter message, ending with "." on a line by itself
    C: Subject: Re: The Cake
    C: Date: Fri, 03 May 2019 02:31:20 +0000
    C:
    C: Do NOT forget to bring the cake!
    C: .
    S: 250 OK
    C: QUIT
    S: 221 closing connection

客户端命令:
The common client commands we use are as follows:
HELO: the client to identify itself to the server.
MAIL: specify who is sending the mail.
RCPT: specify a recipient.
DATA: initiate the transfer of the actual email. This email should include both headers and a body.
QUIT: end the session.

服务端命令:
The server response codes used in a successful email transfer are the following:
220: The service is ready
250: The requested command was accepted and completed successfully
354: Start sending the message
221: The connection is closing

示例:
C: MAIL FROM:<alice@example.net>
S: 250 OK
C: RCPT TO:<not-a-real-user@example.com>
S: 550-The account you tried to deliver to does not
S: 550-exist. Please double-check the recipient's
S: 550 address for typos and try again.

a sample email:
** start line ************
From: Alice Doe <alice@example.net>
To: Bob Doe <bob@example.com>
Subject: Re: The Cake
Date: Fri, 03 May 2019 02:31:20 +0000

Hi Bob

,Do NOT forget to bring the cake!

Best,
Alice
** end line ************

email 分成两部分组成:
1. header:
    From
    To
    Subject
    Data
2. body
    邮件内容本身

header 和 body 由第一行空行分隔开来.

 */


#include "chap08.h"
#include <ctype.h>
#include <stdarg.h>

#define MAXINPUT 512 /** 用户输入长度限制 */
#define MAXRESPONSE 1024

/**
 * 提示用户输入信息
 * @prompt: 提示语指针
 * @buffer: 返回获取到的输入时间
 *
 *
 * C 有 gets() 函数, 但是再新的 C 标准中废除了这个函数. 这里自己实现.
 * */
void get_input(const char *prompt, char *buffer)
{
    printf("%s", prompt);

    buffer[0] = 0;
    fgets(buffer, MAXINPUT, stdin); /** 用标准输入获取用户输入数据 */
    /**
     * 检查输入数据长度, 用 '0' 标记字符串结束
     *
     * The fgets() function does not remove a newline character from the received input;
     * therefore, we overwrite the last character inputted with a terminating null character.
     *
     * */
    const int read = strlen(buffer);
    if (read > 0)
        buffer[read-1] = 0;
}

/**
 * 发送数据到 server
 *
 * @server: socker 通讯描述符
 * @text:   可以带格式化的字符串, 带格式化的需要按需增加参数
 *
 * 例如:
 *   send_format(server, "HELO HONPWC\r\n");
 *   send_format(server, "MAIL FROM:<%s>\r\n", sender);
 * */
void send_format(SOCKET server, const char *text, ...) {
    /**
     * buffer 是临时缓存, 将参数组合成的数据, 是最终发送的字符串.
     * 注意要检查有没有溢出, 这里并没有检查溢出情况, 需要调用者保证不溢出
     * */
    char buffer[1024];
    va_list args;

    /** 组合格式化数据 */
    va_start(args, text);
    vsprintf(buffer, text, args);
    va_end(args);

    send(server, buffer, strlen(buffer), 0);

    printf("C: %s", buffer); /** 打印 client 发送的数据 */
}

/**
 * 解析 server 响应, 只解析完整的数据包, 数据包不完整的时候返回 0,
 * 等待接收完整后再来解析
 *
 * 这里不关心响应码的说明部分, 只解析响应码
 *
 * 发送请求后, 没收到服务器响应前, 不能再次发送请求, 不然服务器会中断连接.
 * If the SMTP client sends a new command before the server is ready,
 * then the server will likely terminate the connection.
 *
 * 这两个响应是等价的:
 * 1 是单独一行, 2 是两行.
 * 2 行的时候, 第一行数字后面紧跟着一个破折号, 第二回不用破折号.
 *
 *  1. 
 *      250 Message received!
 *  2. 
 *      250-Message
 *      250 received!
 *
 * 未全部理解
 * */
int parse_response(const char *response)
{
    const char *k = response;
    /** 检查前三位不为 null */
    if (!k[0] || !k[1] || !k[2]) return 0;
    for (; k[3]; ++k) {
        if (k == response || k[-1] == '\n') { /** 检查换行 */
            if (isdigit(k[0]) && isdigit(k[1]) && isdigit(k[2])) { /** 检查前三位是数字字符 */
                if (k[3] != '-') { /** 检查第四位是否是破折号, 不是破折号的为单行响应 */
                    if (strstr(k, "\r\n")) {
                        return strtol(k, 0, 10); /** 转换响应码为整数 */
                    }
                }
            }
        }
    }
    return 0;
}

/**
 * 等待服务器的响应码
 *
 * expecting 是期望返回的响应码
 * */
void wait_on_response(SOCKET server, int expecting)
{
    char response[MAXRESPONSE+1]; /** 注意缓存不能溢出 */
    char *p = response;
    char *end = response + MAXRESPONSE;

    int code = 0;

    do { /** 接收数据并解析响应码 */
        int bytes_received = recv(server, p, end - p, 0);
        if (bytes_received < 1) {
            fprintf(stderr, "Connection dropped.\n");
            exit(1);
        }

        p += bytes_received;
        *p = 0; /** 接收末尾设置 '0', 变成字符串好处理 */

        if (p == end) { /** 溢出检查 */
            fprintf(stderr, "Server response too large:\n");
            fprintf(stderr, "%s", response);
            exit(1);
        }
        /** 数据包不完整, code = 0, 需要继续接收数据 */
        code = parse_response(response);

    } while (code == 0); /** 得到非 0 的 code 后, 结束循环 */

    if (code != expecting) {
        fprintf(stderr, "Error from server:\n");
        fprintf(stderr, "%s", response);
        exit(1);
    }

    printf("S: %s", response);
}

/**
 * 连接服务器
 * */
SOCKET connect_to_host(const char *hostname, const char *port)
{
    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM; /** 基于 TCP */
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



int main() {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif


    char hostname[MAXINPUT];
    get_input("mail server: ", hostname);

    printf("Connecting to host: %s:25\n", hostname);

    SOCKET server = connect_to_host(hostname, "25"); /** email SMTP 端口 25 */
    /**
     * After the connection is established,
     * our SMTP client must not issue any
     * commands until the server responds with a 220 code
     * */
    wait_on_response(server, 220);
    /** If you are running this client from a server,
     * then you should change
     * the HONPWC string to a domain that points to your server */
    send_format(server, "HELO HONPWC\r\n"); /** 根据协议结尾要加'\r\n' */
    wait_on_response(server, 250);



    char sender[MAXINPUT];
    get_input("from: ", sender);
    send_format(server, "MAIL FROM:<%s>\r\n", sender);
    wait_on_response(server, 250);

    char recipient[MAXINPUT];
    get_input("to: ", recipient);
    send_format(server, "RCPT TO:<%s>\r\n", recipient);
    wait_on_response(server, 250);

    send_format(server, "DATA\r\n");
    wait_on_response(server, 354);

    char subject[MAXINPUT];
    get_input("subject: ", subject);



    send_format(server, "From:<%s>\r\n", sender);
    send_format(server, "To:<%s>\r\n", recipient);
    send_format(server, "Subject:%s\r\n", subject);


    time_t timer;
    time(&timer);

    struct tm *timeinfo;
    timeinfo = gmtime(&timer);

    char date[128];
    /** 格式化时间 */
    strftime(date, 128, "%a, %d %b %Y %H:%M:%S +0000", timeinfo);

    send_format(server, "Date:%s\r\n", date);

    send_format(server, "\r\n"); /** header 和 body 的空行 */




    printf("Enter your email text, end with \".\" on a line by itself.\n");

    while (1) {
        char body[MAXINPUT];
        get_input("> ", body);
        send_format(server, "%s\r\n", body);
        if (strcmp(body, ".") == 0) {
            break;
        }
    }


    wait_on_response(server, 250);

    send_format(server, "QUIT\r\n");
    wait_on_response(server, 221);

    printf("\nClosing socket...\n");
    CLOSESOCKET(server);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}

