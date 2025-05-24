//
// Created by Administrator on 2025/5/21.
//

#include "tcp_echo_server.h"
#include "sys_plat.h"

int tcp_echo_server_start(int port) {
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        plat_printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }

    plat_printf("tcp echo client, port: %d\n", port);

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (s == INVALID_SOCKET) {
        plat_printf("tcp server: open socket error\n");
    }

    struct sockaddr_in server_addr;
    plat_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(s, (const struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        plat_printf("tcp server: connect error");
        goto end;
    }

    listen(s, 5);

    while (1) {

        struct sockaddr_in client_addr;
        int client_size = sizeof(client_addr);
        SOCKET client = accept(s, (struct sockaddr*)&client_addr, &client_size);

        if (client < 0) {
            plat_printf("tcp server: connect error\n");
            break;
        }

        plat_printf("tcp server: read from ip: %s, port: %d\n",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        char buf[100];
        int len;
        while ( (len = recv(client, buf, sizeof(buf), 0)) > 0) {
            plat_printf("tcp server: received data %s\n", buf);
            send(client, buf, len, 0);
        }

        closesocket(client);

    }


    end:
    if (s > 0) {
        closesocket(s);
    }

    return -1;
}