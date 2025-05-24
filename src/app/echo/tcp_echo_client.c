//
// Created by Administrator on 2025/5/21.
//


#include "winsock2.h"
#include "sys_plat.h"

int tcp_echo_client_start(const char *ip, int port) {
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        plat_printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }

    plat_printf("tcp echo client, ip: %s, port: %d\n", ip, port);

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (s < 0) {
        plat_printf("tcp client: open socket error");
    }

    struct sockaddr_in server_addr;
    plat_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (connect(s, (const struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        plat_printf("connect error");
        goto end;
    }

    struct sockaddr_in local_addr;
    char local_ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, sizeof(local_ip));
    plat_printf("local ip: %s, local port: %d\n", local_ip, ntohs(local_addr.sin_port));

    char buf[128];
    plat_printf(">>");
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (send(s, buf, plat_strlen(buf) - 1, 0) <= 0) {
            plat_printf("write error");
            goto end;
        }


        plat_memset(buf, 0, sizeof(buf));
        int len = recv(s, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            plat_printf("read error");
            goto end;
        }

        plat_printf("%s\n", buf);
        plat_printf(">>");
    }

    closesocket(s);

    end:

    if (s >= 0) {
        closesocket(s);
    }

    return -1;

}
