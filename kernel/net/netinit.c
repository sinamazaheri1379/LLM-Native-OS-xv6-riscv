//
// Created by sina-mazaheri on 11/26/24.
//

#include "include/netif.h"
#include "lwip/src/include/lwip/netif.h"
#include "lwip/src/include/lwip/init.h"
#include "lwip/src/include/lwip/netif.h"
#include "lwip/src/include/lwip/ip4_addr.h"
#include "lwip/src/include/lwip/prot/ethernet.h"

void
networkinit(void)
{
    ip4_addr_t ipaddr, netmask, gw;
    struct netif *netif;

    // Initialize lwIP
    lwip_init();

    // Create network interface
    netif = (struct netif*)kalloc();
    if(!netif) {
        panic("networkinit");
    }

    // Setup IP addresses
    IP4_ADDR(&ipaddr, 10,0,2,15);
    IP4_ADDR(&netmask, 255,255,255,0);
    IP4_ADDR(&gw, 10,0,2,2);

    // Add network interface using lwIP's ethernet input
    netif = netif_add(netif, &ipaddr, &netmask, &gw,
                      NULL, e1000_init, ethernet_input);

    if(!netif) {
        panic("netif_add failed");
    }

    netif_set_default(netif);
    netif_set_up(netif);
}




