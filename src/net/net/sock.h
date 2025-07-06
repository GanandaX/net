//
// Created by Administrator on 2025/6/30.
//
#ifndef SOCK_H
#define SOCK_H

#include "ex_msg.h"
#include "net_status.h"
#include "sys.h"

#define SOCK_WAIT_READ      (1 << 0)
#define SOCK_WAIT_WRITE     (1 << 1)
#define SOCK_WAIT_CONN      (1 << 2)
#define SOCK_WAIT_ALL       (SOCK_WAIT_READ | SOCK_WAIT_WRITE | SOCK_WAIT_CONN)

struct _sock_t;
struct x_sockaddr;
typedef int x_socklen_t;

typedef struct _sock_wait_t {
    sys_sem_t sem;
    net_status_t status;
    int waiting;
} sock_wait_t;

typedef struct _sock_ops_t {
    net_status_t (*close)(struct _sock_t *s);

    net_status_t (*sendto)(struct _sock_t *s, const void *buf, size_t len, int flags,
                           const struct x_sockaddr *dest, x_socklen_t dest_len, ssize_t *result_len);

    net_status_t (*recvfrom)(struct _sock_t *s, void *buf, size_t len, int flags,
                             const struct x_sockaddr *src, x_socklen_t *src_len, ssize_t *result_len);

    net_status_t (*setopt)(struct _sock_t *s, int level, int optname, const char *optval, int optlen);

    void (*destroy)(struct _sock_t *s);
} sock_ops_t;

typedef struct _sock_t {
    ipaddr_t local_ip;
    uint16_t local_port;
    ipaddr_t remote_ip;
    uint16_t remote_port;

    const sock_ops_t *ops;
    int family;
    int protocol;

    int state;
    int snd_tmo;
    int rcv_tmo;

    sock_wait_t *snd_wait;
    sock_wait_t *rcv_wait;
    sock_wait_t *cnn_wait;
    n_list_node_t node;
} sock_t;

typedef struct _x_socket_t {
    enum {
        SOCKET_STATE_FREE,
        SOCKET_STATE_USED,
    } state;

    sock_t *sock;
} x_socket_t;

typedef struct _sock_create_t {
    int family;
    int type;
    int protocol;
} sock_create_t;

typedef struct _sock_data_t {
    uint8_t *buf;
    size_t len;
    int flags;
    struct x_sockaddr *addr;
    x_socklen_t *saddr_len;

    ssize_t cmp_len;
} sock_data_t;

typedef struct _sock_opt_t {
    int level;
    int optname;;
    const char *optval;
    int len;
} sock_opt_t;

typedef struct _sock_req_t {
    sock_wait_t *wait;
    int wait_tmo;

    int sockfd;

    union {
        sock_create_t create;
        sock_data_t data;
        sock_opt_t opt;
    };
} sock_req_t;


net_status_t socket_init(void);

net_status_t sock_wait_init(sock_wait_t *wait);

void sock_wait_add(sock_wait_t *wait, int tmo, sock_req_t *req);

net_status_t sock_wait_enter(sock_wait_t *wait, int tmo);

void sock_wait_exit(sock_wait_t *wait, net_status_t status);

void sock_wakeup(sock_t *sock, int type, net_status_t status);

net_status_t sock_wait_destory(sock_wait_t *wait);

net_status_t socket_create_req_in(func_msg_t *msg);

net_status_t socket_sendto_req_in(func_msg_t *msg);

net_status_t socket_recvfrom_req_in(func_msg_t *msg);

net_status_t socket_setsockopt_req_in(func_msg_t *msg);

net_status_t sock_setopt(struct _sock_t *s, int level, int optname, const char *optval, int optlen);

net_status_t sock_init(sock_t *sock, int family, int protocol, const sock_ops_t *ops);

net_status_t sock_uninit(sock_t *sock);

net_status_t socket_close(func_msg_t *msg);

#endif //SOCK_H
