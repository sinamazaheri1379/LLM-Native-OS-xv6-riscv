//
// Created by sina-mazaheri on 11/26/24.
//

#ifndef XV6_NETIF_H
#define XV6_NETIF_H

#include "lwip/netif.h"

// Global network interface
extern struct netif *default_netif;

// Network initialization
void networkinit(void);

#endif
