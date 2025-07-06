//
// Created by Administrator on 2025/6/30.
//
#include "sock.h"
#include "ex_msg.h"
#include "raw.h"
#include "sys_plat.h"
#include "socket.h"

#define SOCKET_MAX_NR       (RAW_MAX_CNT)

static x_socket_t socket_table[SOCKET_MAX_NR];

static int get_index(x_socket_t *socket) {
    return (int) (socket - socket_table);
}

static x_socket_t *get_socket(int index) {
    if (index >= SOCKET_MAX_NR || index < 0) {
        return (void *) 0;
    }

    return socket_table + index;
}

static x_socket_t *socket_alloc(void) {
    x_socket_t *socket = (void *) 0;
    for (int i = 0; i < SOCKET_MAX_NR; i++) {
        x_socket_t *cur_socket = socket_table + i;
        if (cur_socket->state == SOCKET_STATE_FREE) {
            cur_socket->state = SOCKET_STATE_USED;
            socket = cur_socket;
            break;
        }
    }

    return socket;
}

static void socket_free(x_socket_t *socket) {
    socket->state = SOCKET_STATE_FREE;
}

net_status_t socket_init(void) {
    plat_memset(socket_table, 0, sizeof(socket_table));

    return NET_OK;
}

net_status_t sock_wait_init(sock_wait_t *wait) {
    wait->waiting = 0;
    wait->status = NET_OK;
    wait->sem = sys_sem_create(0);
    return wait->sem == SYS_SEM_INVALID ? NET_ERROR_SYSTEM : NET_OK;
}

void sock_wait_add(sock_wait_t *wait, int tmo, sock_req_t *req) {
    wait->waiting++;

    req->wait = wait;
    req->wait_tmo = tmo;
}


net_status_t sock_wait_enter(sock_wait_t *wait, int tmo) {
    if (sys_sem_wait(wait->sem, tmo) < 0) {
        return NET_ERROR_TIMEOUT;
    }
    return wait->status;
}

void sock_wait_exit(sock_wait_t *wait, net_status_t status) {
    if (wait->waiting > 0) {
        wait->waiting--;
        sys_sem_notify(wait->sem);
        wait->status = status;
    }
}

void sock_wakeup(sock_t *sock, int type, net_status_t status) {
    if (type & SOCK_WAIT_CONN) {
        sock_wait_exit(sock->cnn_wait, status);
    }
    if (type & SOCK_WAIT_WRITE) {
        sock_wait_exit(sock->snd_wait, status);
    }
    if (type & SOCK_WAIT_READ) {
        sock_wait_exit(sock->rcv_wait, status);
    }
}

net_status_t sock_wait_destory(sock_wait_t *wait) {
    if (wait->sem != SYS_SEM_INVALID) {
        sys_sem_free(wait->sem);
    }
    return NET_OK;
}

net_status_t socket_create_req_in(func_msg_t *msg) {
    static const struct sock_info_t {
        int protocol;

        sock_t * (*create)(int family, int protocol);
    } sock_table[] = {
                [SOCK_RAW] = {.protocol = IPPROTO_ICMP, .create = raw_create},
            };

    sock_req_t *req = (sock_req_t *) msg->param;
    sock_create_t *param = &req->create;

    x_socket_t *socket = socket_alloc();
    if (!socket) {
        debug(DEBUG_WARNING, "socket alloc error");
        return NET_ERROR_MEM;
    }

    if (param->type < 0 || param->type >= sizeof(sock_table) / sizeof(sock_table[0])) {
        debug(DEBUG_WARNING, "create sock error");
        socket_free(socket);
        return NET_ERROR_PARAM;
    }

    const struct sock_info_t *info = sock_table + param->type;

    if (param->protocol == 0) {
        param->protocol = info->protocol;
    }
    sock_t *sock = info->create(param->family, param->protocol);
    if (!sock) {
        debug(DEBUG_WARNING, "create sock error");
        socket_free(socket);
        return NET_ERROR_MEM;
    }

    socket->sock = sock;
    req->sockfd = get_index(socket);
    return NET_OK;
}


net_status_t socket_sendto_req_in(func_msg_t *msg) {
    sock_req_t *req = (sock_req_t *) msg->param;
    sock_data_t *param = &req->data;

    x_socket_t *socket = get_socket(req->sockfd);
    if (!socket) {
        debug(DEBUG_WARNING, "param error");
        return NET_ERROR_PARAM;
    }

    sock_t *sock = socket->sock;
    sock_data_t *data = &req->data;
    if (!sock->ops->sendto) {
        debug(DEBUG_ERROR, "sock_ops->sendto error");
        return NET_ERROR_NOT_SUPPORT;
    }

    net_status_t status = sock->ops->sendto(sock, data->buf, data->len, data->flags, data->addr, *data->saddr_len, &data->cmp_len);

    if (status == NET_ERROR_NEED_WAIT) {
        if (sock->snd_wait) {
            sock_wait_add(sock->snd_wait, sock->snd_tmo, req);
        }
    }

    return status;
}

net_status_t socket_recvfrom_req_in(func_msg_t *msg) {
    sock_req_t *req = (sock_req_t *) msg->param;
    sock_data_t *param = &req->data;

    x_socket_t *socket = get_socket(req->sockfd);
    if (!socket) {
        debug(DEBUG_WARNING, "param error");
        return NET_ERROR_PARAM;
    }

    sock_t *sock = socket->sock;
    sock_data_t *data = &req->data;
    if (!sock->ops->recvfrom) {
        debug(DEBUG_ERROR, "sock_ops->recvfrom error");
        return NET_ERROR_NOT_SUPPORT;
    }

    net_status_t status = sock->ops->recvfrom(sock, data->buf, data->len, data->flags, data->addr, data->saddr_len, &data->cmp_len);

    if (status == NET_ERROR_NEED_WAIT) {
        if (sock->rcv_wait) {
            sock_wait_add(sock->rcv_wait, sock->rcv_tmo, req);
        }
    }
    return status;
}

net_status_t sock_setopt(struct _sock_t *s, int level, int optname, const char *optval, int optlen) {
    if (level != SOL_SOCKET) {
        debug(DEBUG_WARNING, "sock_setopt error");
        return NET_ERROR_PARAM;
    }

    switch (optname) {
        case SO_RCVTIMEO:
        case SO_SNDTIMEO:
            if (optlen != sizeof(struct x_timeval)) {
                debug(DEBUG_WARNING, "time size error");
                return NET_ERROR_PARAM;
            }

            struct x_timeval *time = (struct x_timeval *) optval;
            int time_ms = time->tv_sec * 1000 + time->tv_usec / 1000;
            if (optname == SO_RCVTIMEO) {
                s->rcv_tmo = time_ms;
                return NET_OK;
            }
            if (optname == SO_SNDTIMEO) {
                s->snd_tmo = time_ms;
                return NET_OK;
            }
        default:
            break;
    }
    return NET_ERROR_PARAM;
}

net_status_t socket_setsockopt_req_in(func_msg_t *msg) {
    sock_req_t *req = (sock_req_t *) msg->param;
    x_socket_t *socket = get_socket(req->sockfd);
    if (!socket) {
        debug(DEBUG_WARNING, "param error");
        return NET_ERROR_PARAM;
    }

    sock_opt_t *opt = &req->opt;
    sock_t *sock = socket->sock;

    return sock->ops->setopt(sock, opt->level, opt->optname, opt->optval, opt->len);
}

net_status_t sock_init(sock_t *sock, int family, int protocol, const sock_ops_t *ops) {
    sock->protocol = protocol;

    sock->family = family;
    sock->ops = ops;

    ipaddr_set_any(&sock->local_ip);
    ipaddr_set_any(&sock->remote_ip);
    sock->local_port = 0;
    sock->remote_port = 0;
    sock->state = NET_OK;
    sock->rcv_tmo = 0;
    sock->snd_tmo = 0;
    n_list_node_init(&sock->node);

    sock->rcv_wait = (sock_wait_t *) 0;
    sock->snd_wait = (sock_wait_t *) 0;
    sock->cnn_wait = (sock_wait_t *) 0;

    return NET_OK;
}

net_status_t sock_uninit(sock_t *sock) {
    if (sock->rcv_wait) {
        sock_wait_destory(sock->rcv_wait);
    }

    if (sock->cnn_wait) {
        sock_wait_destory(sock->cnn_wait);
    }

    if (sock->snd_wait) {
        sock_wait_destory(sock->snd_wait);
    }
}

net_status_t socket_close(func_msg_t *msg) {
    sock_req_t *req = (sock_req_t *) msg->param;
    x_socket_t *socket = get_socket(req->sockfd);
    if (!socket) {
        debug(DEBUG_WARNING, "param error");
        return NET_ERROR_PARAM;
    }

    sock_t *sock = socket->sock;
    if (!sock->ops->close) {
        debug(DEBUG_WARNING, "sock_ops->close error");
        return NET_ERROR_NOT_SUPPORT;
    }

    net_status_t status =  sock->ops->close(sock);
    socket_free(socket);

    return status;
}
