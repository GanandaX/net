//
// Created by Administrator on 2025/6/29.
//

#ifndef NET_API_H
#define NET_API_H

#include "tools.h"
#include "socket.h"
#include <stdint.h>

char *x_inet_ntoa(struct x_in_addr in);

uint32_t x_inet_addr(const char *str);

int x_inet_pton(int family, const char *strptr, void *addrptr);

const char *x_inet_ntop(int family, const void *addrptr, char *strptr, size_t len);

#undef htons
#define htons(x)  x_htons(x)

#undef ntohs
#define ntohs(x)  x_ntohs(x)

#undef htohl
#define htohl(x)  x_htohl(x)

#undef ntohl
#define ntohl(x)  x_ntohl(x)

#define inet_ntoa(in) x_inet_ntoa(in)
#define inet_addr(str) x_inet_addr(str)
#define inet_pton(family, strptr,addrptr) x_inet_pton(family, strptr,addrptr)
#define inet_ntop(family, addrptr,strptr,len) x_inet_ntop(family, addrptr,strptr,len)

#define sockaddr_in x_sockaddr_in
#define sockaddr x_sockaddr
#define socklen_t x_socklen_t
#define timeval x_timeval
#define socket(family, type, protocol) x_socket(family, type, protocol)
#define sendto(s, buf, len, flags, dest, d_len) x_sendto(s, buf, len, flags, dest, d_len)
#define recvfrom( s,buf, len, flags,from,from_len) x_recvfrom( s,buf, len, flags,from,from_len)
#define setsockopt(s, level, opt_name, opt_val, len) x_setsockopt(s, level, opt_name, opt_val, len)
#define closesocket(s) x_closesocket(s)

#endif //NET_API_H

