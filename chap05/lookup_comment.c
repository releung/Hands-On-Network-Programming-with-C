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

#include "chap05.h"

#ifndef AI_ALL
#define AI_ALL 0x0100
#endif

#define NO_CARE_PORT // 不查询端口/服务

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage:\n\tlookup hostname\n");
        printf("Example:\n\tlookup example.com\n");
        exit(0);
    }

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    printf("Resolving hostname '%s'\n", argv[1]);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_ALL;
    struct addrinfo *peer_address;
#ifdef NO_CARE_PORT
    if (getaddrinfo(argv[1], 0, &hints, &peer_address)) {   /** 第二个参数为 0 的时候, 不考虑端口/服务 */
#else
/** 
 * 扫描主机端口/服务开放情况:
 *      _ nmap -sT -p- baidu.com        # 扫描所有端口
 *      _ nmap -sT baidu.com            # 默认扫描 1-1000 端口
 *      _ nmap -sT -p-1000 baidu.com    # 扫描 1000 个端口
 * 扫描结果实例:
 *      PORT    STATE SERVICE
 *      80/tcp  open  http
 *      443/tcp open  https
 **/
    //const char *service = "80";
    //const char *service = "http";
    const char *service = "https";
    if (getaddrinfo(argv[1], service, &hints, &peer_address)) {
#endif
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Remote address is:\n");
    struct addrinfo *address = peer_address;
    do {
        char address_buffer[100];
#ifndef NO_CARE_PORT
        char service_buffer[100];
#endif
        getnameinfo(address->ai_addr, address->ai_addrlen,
                address_buffer, sizeof(address_buffer),
#ifndef NO_CARE_PORT
                service_buffer, sizeof(service_buffer),
#else
                0, 0,
#endif
                NI_NUMERICHOST | NI_NUMERICSERV);
#ifndef NO_CARE_PORT
        printf("\t%s, %s\n", address_buffer, service_buffer);
#else
        printf("\t%s\n", address_buffer);
#endif
    } while ((address = address->ai_next));


    freeaddrinfo(peer_address);

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}

