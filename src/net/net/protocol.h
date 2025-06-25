//
// Created by GrandBand on 2025/6/14.
//

#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

typedef enum _protocol_t {

    NET_PROTOCOL_ARP = 0X0806,
    NET_PROTOCOL_IPv4 = 0X0800,
    NET_PROTOCOL_ICMPv4 = 1,
    NET_PROTOCOL_UDP = 0X11,
    NET_PROTOCOL_TCP = 0X6,
}protocol_t;

#endif //NET_PROTOCOL_H
