//
// Created by Administrator on 2025/6/30.
//

#ifndef NET_SOCKET_H
#define NET_SOCKET_H
#include <stdint.h>
#include "ipv4.h"
#include "sock.h"
#include "sys_plat.h"

#undef INADDR_ANY
#define INADDR_ANY (u_long)0x00000000

#undef AF_INET
#define AF_INET         2

#undef SOCK_RAW
#define SOCK_RAW        0

#undef IPPROTO_ICMP
#define IPPROTO_ICMP    1

#undef SOL_SOCKET
#define SOL_SOCKET      0

#undef SO_RCVTIMEO
#define SO_RCVTIMEO      1

#undef SO_SNDTIMEO
#define SO_SNDTIMEO      2

struct x_timeval {
    int tv_sec;
    int tv_usec;
};

struct x_in_addr {
    union {
        struct {
            uint8_t addr0;
            uint8_t addr1;
            uint8_t addr2;
            uint8_t addr3;
        };

        uint8_t addr_array[IPV4_ADDR_SIZE];
#undef s_addr
        uint32_t s_addr;
    };
};

struct x_sockaddr {
    uint8_t sin_len;
    uint8_t sin_family;
    uint8_t sa_data[14];
};

struct x_sockaddr_in {
    uint8_t sin_len;
    uint8_t sin_family;
    u_short sin_port;
    struct x_in_addr sin_addr;
    char sin_zero[8];
};

int x_socket(int family, int type, int protocol);

ssize_t x_sendto(int s, const void *buf, size_t len, int flags, const struct x_sockaddr *dest, x_socklen_t dest_len);

ssize_t x_recvfrom(int s, void *buf, size_t len, int flags, const struct x_sockaddr *from, x_socklen_t *from_len);

int x_setsockopt(int s, int level, int opt_name, const char *opt_val, int len);

int x_closesocket(int s);
#endif //SOCKET_H
