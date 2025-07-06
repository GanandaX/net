//
// Created by Administrator on 2025/6/28.
//
#include "ping.h"
#include "sys_plat.h"
#include "time.h"
#include "net_api.h"

uint16_t check_sum(void *buf, uint16_t len) {
    uint16_t *curr_buf = (uint16_t *) buf;
    uint32_t checksum = 0;

    while (len > 1) {
        checksum += *curr_buf++;
        len -= 2;
    }

    if (len == 1) {
        checksum += *(uint8_t *) curr_buf;
    }

    uint16_t high;
    while ((high = checksum >> 16) != 0) {
        checksum = (checksum & 0xffff) + high;
    }

    return ~checksum;
}

void ping_run(ping_t *ping, const char *dest, uint16_t times, uint32_t size, uint16_t interval) {
    static uint16_t start_id = PING_DEFAULT_ID;

    WSADATA wsdata;
    WSAStartup(MAKEWORD(2, 2), &wsdata);

    int s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (s < 0) {
        plat_printf("ping: open socket error");
        return;
    }

    // int tmo = 3000;
    struct timeval tmo;
    tmo.tv_sec = 3;
    tmo.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &tmo, sizeof(tmo));

    struct sockaddr_in addr;
    plat_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(dest);
    addr.sin_port = 0;

    // connect(s, (const struct sockaddr *)&addr, sizeof(addr));

    int fill_size = size > PING_BUFFER_SIZE ? PING_BUFFER_SIZE : size;
    for (int i = 0; i < fill_size; i++) {
        ping->req.buf[i] = i;
    }

    uint32_t total_size = sizeof(icmp_hdr_t) + fill_size;
    for (int i = 0, seq = 0; i < times; i++, seq++) {
        ping->req.echo_hdr.type = 8;
        ping->req.echo_hdr.code = 0;
        ping->req.echo_hdr.checksum = 0;
        ping->req.echo_hdr.id = start_id;
        ping->req.echo_hdr.seq = seq;
        ping->req.echo_hdr.checksum = check_sum(&ping->req, total_size);

        int size = sendto(s, (const char *) &ping->req, total_size, 0, (const struct sockaddr *) &addr, sizeof(addr));
#if 0
        int size = send(s, (const char *) &ping->req, total_size, 0);
#endif
        if (size < 0) {
            plat_printf("ping: send to error\n");
            break;
        }

        clock_t time = clock();

        do {
            memset(&ping->reply, 0, sizeof(ping->reply));
            struct sockaddr_in from_addr;
            socklen_t addr_len = sizeof(addr);
            size = recvfrom(s, (char *) &ping->reply, sizeof(ping->reply), 0, (struct sockaddr *) &from_addr, &addr_len);
#if 0
            size = recv(s, (char *) &ping->reply, sizeof(ping->reply), 0);
#endif
            if (size < 0) {
                plat_printf("ping: recvfrom error\n");
                break;
            }

            if ((ping->req.echo_hdr.id == ping->reply.echo_hdr.id) &&
                (ping->req.echo_hdr.seq == ping->reply.echo_hdr.seq)) {
                break;
            }
        } while (1);

        if (size > 0) {
            int recv_size = size - sizeof(ip_header_t) - sizeof(icmp_hdr_t);
            if (memcmp(ping->req.buf, ping->reply.buf, recv_size) &&
                (fill_size != recv_size)) {
                plat_printf("recv data error\n");
                continue;
            }

            ip_header_t *ip_hdr = &ping->reply.ip_hdr;
            plat_printf("reply from %s: bytes: %d", inet_ntoa(addr.sin_addr), recv_size);

            int diff_ms = (clock() - time) / (CLOCKS_PER_SEC / 1000);
            if (diff_ms < 1) {
                plat_printf("  time < 1 ms, TTL = %d", ip_hdr->ttl);
            } else {
                plat_printf("  time < %d ms, TTL = %d", diff_ms, ip_hdr->ttl);
            }

            plat_printf("\n");
        }

        sys_sleep(interval);
    }

    closesocket(s);
}
