//
// Created by Administrator on 2025/6/30.
//
#include "socket.h"

#include "ex_msg.h"
#include "sock.h"

int x_socket(int family, int type, int protocol) {
    sock_req_t req;
    req.wait = (sock_wait_t *) 0;
    req.wait_tmo = 0;
    req.sockfd = -1;
    req.create.family = family;
    req.create.type = type;
    req.create.protocol = protocol;

    net_status_t status = exmsg_func_exec(socket_create_req_in, &req);
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "socket create error");
        return -1;
    }

    return req.sockfd;
}

ssize_t x_sendto(int s, const void *buf, size_t len, int flags, const struct x_sockaddr *dest, x_socklen_t dest_len) {
    if (!buf || !len) {
        debug(DEBUG_WARNING, "param error");
        return -1;
    }

    if ((dest->sin_family != AF_INET) || (dest_len != sizeof(struct x_sockaddr_in))) {
        debug(DEBUG_WARNING, "dest addr error");
        return -1;
    }

    ssize_t send_size = 0;
    uint8_t *start = (uint8_t *) buf;
    while (len > 0) {
        sock_req_t req;
        req.sockfd = s;
        req.wait = (sock_wait_t *) 0;
        req.wait_tmo = 0;
        req.data.buf = start;
        req.data.len = len;
        req.data.flags = 0;
        req.data.addr = (struct x_sockaddr *) dest;
        req.data.saddr_len = &dest_len;
        req.data.cmp_len = 0;

        net_status_t status = exmsg_func_exec(socket_sendto_req_in, &req);
        if (status < NET_OK) {
            debug(DEBUG_ERROR, "socket send error");
            return -1;
        }

        if (req.wait && (status = sock_wait_enter(req.wait, req.wait_tmo)) < 0) {
            debug(DEBUG_ERROR, "socket send wait error");
            return -1;
        }

        len -= req.data.cmp_len;
        start += req.data.cmp_len;
        send_size += req.data.cmp_len;
    }

    return send_size;
}


ssize_t x_recvfrom(int s, void *buf, size_t len, int flags, const struct x_sockaddr *from, x_socklen_t *from_len) {
    if (!buf || !len || !from) {
        debug(DEBUG_WARNING, "param error");
        return -1;
    }

    while (1) {
        sock_req_t req;
        req.wait = (sock_wait_t *) 0;
        req.wait_tmo = 0;
        req.sockfd = s;
        req.data.buf = buf;
        req.data.len = len;
        req.data.flags = 0;
        req.data.addr = (struct x_sockaddr *) from;
        req.data.saddr_len = from_len;
        req.data.cmp_len = 0;

        net_status_t status = exmsg_func_exec(socket_recvfrom_req_in, &req);

        if (status < NET_OK) {
            debug(DEBUG_ERROR, "socket recv error");
            return -1;
        }

        if (req.data.cmp_len) {
            return (ssize_t) req.data.cmp_len;
        }

        status = sock_wait_enter(req.wait, req.wait_tmo);

        if (status < 0) {
            debug(DEBUG_ERROR, "recv failed.");
            return -1;
        }
    }
}

int x_setsockopt(int s, int level, int opt_name, const char *opt_val, int len) {
    if (!opt_val || !len) {
        debug(DEBUG_WARNING, "param error");
        return -1;
    }

    sock_req_t req;
    req.wait = (sock_wait_t *) 0;
    req.wait_tmo = 0;
    req.sockfd = s;

    req.opt.level = level;
    req.opt.optname = opt_name;
    req.opt.optval = opt_val;
    req.opt.len = len;

    net_status_t status = exmsg_func_exec(socket_setsockopt_req_in, &req);
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "socket socket_setsockopt_req_in error");
        return -1;
    }

    return 0;
}

int x_closesocket(int s) {
    sock_req_t req;
    req.wait = (sock_wait_t *) 0;
    req.wait_tmo = 0;
    req.sockfd = s;

    net_status_t status = exmsg_func_exec(socket_close, &req);
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "socket create error");
        return -1;
    }

    return 0;
}
