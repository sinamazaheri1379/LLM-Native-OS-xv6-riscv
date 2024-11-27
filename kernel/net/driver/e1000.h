//
// Created by sina-mazaheri on 11/26/24.
//

#ifndef XV6_E1000_H
#define XV6_E1000_H

#include "types.h"
#include "lwip/netif.h"

// E1000 initialization
err_t e1000_init(struct netif *netif);

// E1000 transmit/receive
err_t e1000_output(struct netif *netif, struct pbuf *p);
struct pbuf* e1000_recv(void);

// IRQ number for QEMU e1000
#define E1000_IRQ 33

#endif
