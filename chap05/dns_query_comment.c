/*
 * MIT License
 *
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

/** 下表号就是 type 类型号, 对应到字符串. index 0 排除 */
const char *dns_type[] = {
    "NO_THIS_TYPE", /** 0 */
    "A",      /** 1 */
    "NS",     /** 2 */
    "MD",     /** 3 */
    "MF",     /** 4 */
    "CNAME",  /** 5 */
    "SOA",    /** 6 */
    "MB",     /** 7 */
    "MG",     /** 8 */
    "MR",     /** 9 */
    "NULL",   /** 10 */
    "WKS",    /** 11 */
    "PTR",    /** 12 */
    "HINFO",  /** 13 */
    "MINFO",  /** 14 */
    "MX",     /** 15 */
    "TXT"     /** 16 */
};


/** 
 * 打印 QNAME 的名字
 * msg to be a pointer to the message's beginning,          报文起始位置
 * p to be a pointer to the name to print,                  qdcount(qname) 起始位置
 * end to be a pointer to one past the end of the message,  报文结束位置
 *
 * */
const unsigned char *print_name(const unsigned char *msg,
        const unsigned char *p, const unsigned char *end) {
    /** 
     * Because a name should consist of at least a length and some text,
     * we can return an error if p is already within two characters of the end
     * */
    if (p + 2 > end) {
        fprintf(stderr, "End of message.\n"); exit(1);}

    /**
     * 域名（2字节或不定长）：它的格式和Queries区域的查询名字字段是一样的。
     *          有一点不同就是，当报文中域名重复出现的时候，该字段使用2个字节的偏移指针来表示。
     *          比如: 
     *              在资源记录中，域名通常是查询问题部分的域名的重复，
     *              因此用2字节的指针来表示，具体格式是最前面的两个高位是 11，用于识别指针。
     *              其余的14位从DNS报文的开始处计数（从0开始），指出该报文中的相应字节数。
     *              一个典型的例子，C00C(1100000000001100，12正好是头部的长度，其正好指向Queries区域的查询名字字段
     *
     * 根据协议检查是否是 NAME 指针
     * */
    if ((*p & 0xC0) == 0xC0) {
        const int k = ((*p & 0x3F) << 8) + p[1]; // 提取距离报文开头的 offset 位置
        p += 2;
        printf(" (pointer %d) ", k);
        print_name(msg, msg+k, end); // 打印 name 信息
        return p; // 返回

    } else { // 不是指针，就是 name 数据了
        const int len = *p++;
        if (p + len + 1 > end) {
            fprintf(stderr, "End of message.\n"); exit(1);}

        printf("%.*s", len, p); // 注意这里的打印 %.*s
        p += len;
        if (*p) { // 不是 0x00 结尾, 表明还有数据
            printf("."); // 按照协议打印分割 '.'. example.com 中的 'example' 和 'com' 之间的 '.'
            return print_name(msg, p, end); // 继续打印
        } else { // 数据结束
            return p+1;
        }
    }
}

/**
 * 根据协议解析各个字段
 * */
void print_dns_message(const char *message, int msg_length) {
    /** dns 协议头固定 12 字节 */
    if (msg_length < 12) {
        fprintf(stderr, "Message is too short to be valid.\n");
        exit(1);
    }

    const unsigned char *msg = (const unsigned char *)message;

    /*
    // 打印原始数据
    int i;
    for (i = 0; i < msg_length; ++i) {
        unsigned char r = msg[i];
        printf("%02d:   %02X  %03d  '%c'\n", i, r, r, r);
    }
    printf("\n");
    */

    /** ID 字段打印 */
    printf("ID = %0X %0X\n", msg[0], msg[1]);

    /** QR 字段打印 */
    const int qr = (msg[2] & 0x80) >> 7;
    printf("QR = %d %s\n", qr, qr ? "response" : "query");

    /** OPCODE 字段打印 */
    const int opcode = (msg[2] & 0x78) >> 3;
    printf("OPCODE = %d ", opcode);
    switch(opcode) {
        case 0: printf("standard\n"); break;
        case 1: printf("reverse\n"); break;
        case 2: printf("status\n"); break;
        default: printf("?\n"); break;
    }

    /** AA 字段打印 */
    const int aa = (msg[2] & 0x04) >> 2;
    printf("AA = %d %s\n", aa, aa ? "authoritative" : "");

    /** TC 字段打印 */
    const int tc = (msg[2] & 0x02) >> 1;
    printf("TC = %d %s\n", tc, tc ? "message truncated" : "");

    /** RD 字段打印 */
    const int rd = (msg[2] & 0x01);
    printf("RD = %d %s\n", rd, rd ? "recursion desired" : "");

    /** qr=1 是响应, qt=0 是请求 */
    if (qr) { /** 处理响应 */
        /** RCODE(Response code) 字段打印 */
        const int rcode = msg[3] & 0x0F;
        printf("RCODE = %d ", rcode);
        switch(rcode) {
            case 0: printf("success\n"); break;
            case 1: printf("format error\n"); break;
            case 2: printf("server failure\n"); break;
            case 3: printf("name error\n"); break;
            case 4: printf("not implemented\n"); break;
            case 5: printf("refused\n"); break;
            default: printf("?\n"); break;
        }
        if (rcode != 0) return;
    }

    /**
     * QDCOUNT ANCOUNT NSCOUNT ARCOUNT 数据个数提取
     * 各占 16 字节
     * */
    const int qdcount = (msg[4] << 8) + msg[5]; /** 高八位左移 8 bit, 再加上低八位 */
    const int ancount = (msg[6] << 8) + msg[7];
    const int nscount = (msg[8] << 8) + msg[9];
    const int arcount = (msg[10] << 8) + msg[11];

    printf("QDCOUNT = %d\n", qdcount);
    printf("ANCOUNT = %d\n", ancount);
    printf("NSCOUNT = %d\n", nscount);
    printf("ARCOUNT = %d\n", arcount);


    const unsigned char *p = msg + 12; /** qdcount 起始位置 */
    const unsigned char *end = msg + msg_length; /** 结束位置 */

    /** 需要 review 从新理解 */
    if (qdcount) { /** qdcount 处理 */
        int i;
        for (i = 0; i < qdcount; ++i) { /** 逐个处理 Question 数据 */
            if (p >= end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            printf("Query %2d\n", i + 1);
            printf("  name: ");

            /** Answer、Authority、Additional  和 Question 都使用同一个打印接口? 协议不一致 */
            /**
             * 参考
             * https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.2 Question section format
             * https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.3 Resource record format(answer, authority, and additional)
             * 描述
             * name 部分是一样的, 所以用一样的方法打印 name
             * */
            p = print_name(msg, p, end); printf("\n");

            if (p + 4 > end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            const int type = (p[0] << 8) + p[1];
            printf("  type: %d(%s)\n", type, dns_type[type]);
            p += 2;

            const int qclass = (p[0] << 8) + p[1];
            printf(" class: %d\n", qclass);
            p += 2;
        }
    }

    if (ancount || nscount || arcount) {
        int i;
        for (i = 0; i < ancount + nscount + arcount; ++i) { /** 逐个处理 Answer、Authority、Additional 数据 */
            if (p >= end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            printf("Answer %2d\n", i + 1);
            printf("  name: ");

            /** Answer、Authority、Additional  和 Question 都使用同一个打印接口? 协议不一致 */
            p = print_name(msg, p, end); printf("\n");

            if (p + 10 > end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            /** type 打印 */
            const int type = (p[0] << 8) + p[1];
            printf("  type: %d(%s)\n", type, dns_type[type]);
            p += 2;

            /** qclass 打印 */
            const int qclass = (p[0] << 8) + p[1];
            printf(" class: %d\n", qclass);
            p += 2;

            /** ttl 打印 */
            const unsigned int ttl = (p[0] << 24) + (p[1] << 16) +
                (p[2] << 8) + p[3];
            printf("   ttl: %u\n", ttl);
            p += 4;

            /** rdlen 打印 */
            const int rdlen = (p[0] << 8) + p[1];
            printf(" rdlen: %d\n", rdlen);
            p += 2;

            if (p + rdlen > end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            /** 判断记录类型处理 */
            if (rdlen == 4 && type == 1) {
                /* A Record */
                printf("Address ");
                printf("%d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

            } else if (rdlen == 16 && type == 28) {
                /* AAAA Record */
                printf("Address ");
                int j;
                for (j = 0; j < rdlen; j+=2) {
                    printf("%02x%02x", p[j], p[j+1]);
                    if (j + 2 < rdlen) printf(":");
                }
                printf("\n");

            } else if (type == 15 && rdlen > 3) {
                /* MX Record */
                const int preference = (p[0] << 8) + p[1];
                printf("  pref: %d\n", preference);
                printf("MX: ");
                print_name(msg, p+2, end); printf("\n");

            } else if (type == 16) {
                /* TXT Record */
                printf("TXT: '%.*s'\n", rdlen-1, p+1);

            } else if (type == 5) {
                /* CNAME Record */
                printf("CNAME: ");
                print_name(msg, p, end); printf("\n");
            }

            p += rdlen;
        }
    }

    if (p != end) {
        printf("There is some unread data left over.\n");
    }

    printf("\n");
}


int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("Usage:\n\tdns_query hostname type\n");
        printf("\t  type can use: a, aaaa, txt, mx, or any\n");
        printf("Example:\n\tdns_query example.com aaaa\n");
        exit(0);
    }

    if (strlen(argv[1]) > 255) {
        fprintf(stderr, "Hostname too long.");
        exit(1);
    }

    unsigned char type;
    if (strcmp(argv[2], "a") == 0) {
        type = 1;
    } else if (strcmp(argv[2], "mx") == 0) {
        type = 15;
    } else if (strcmp(argv[2], "txt") == 0) {
        type = 16;
    } else if (strcmp(argv[2], "aaaa") == 0) {
        type = 28;
    } else if (strcmp(argv[2], "any") == 0) {
        type = 255;
    } else {
        fprintf(stderr, "Unknown type '%s'. Use a, aaaa, txt, mx, or any.",
                argv[2]);
        exit(1);
    }

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM; // dns 的底层是基于 udp 协议的.
    struct addrinfo *peer_address;
    /**
     * 8.8.8.8 是 google 公共 dns 服务器
     * 53 端口是 dns 服务端口
     * */
    if (getaddrinfo("8.8.8.8", "53", &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    /** 准确理解 dns 协议就可以了 */
    char query[1024] = {0xAB, 0xCD, /* ID */
                        0x01, 0x00, /* Set recursion */
                        0x00, 0x01, /* QDCOUNT */
                        0x00, 0x00, /* ANCOUNT */
                        0x00, 0x00, /* NSCOUNT */
                        0x00, 0x00 /* ARCOUNT */};

    char *p = query + 12;
    char *h = argv[1];

    /** 填充域名 */
    while(*h) {
        char *len = p;
        p++;
        if (h != argv[1]) ++h;

        while(*h && *h != '.') *p++ = *h++;
        *len = p - len - 1;
    }

    *p++ = 0;
    *p++ = 0x00; *p++ = type; /* QTYPE */
    *p++ = 0x00; *p++ = 0x01; /* QCLASS */


    const int query_size = p - query;

    /** 发送请求 */
    int bytes_sent = sendto(socket_peer,
            query, query_size,
            0,
            peer_address->ai_addr, peer_address->ai_addrlen);
    printf("Sent %d bytes.\n", bytes_sent);

    print_dns_message(query, query_size);

    /** 接收响应 */
    char read[1024];
    int bytes_received = recvfrom(socket_peer,
            read, 1024, 0, 0, 0);

    printf("Received %d bytes.\n", bytes_received);

    print_dns_message(read, bytes_received);
    printf("\n");


    freeaddrinfo(peer_address);
    CLOSESOCKET(socket_peer);

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}

