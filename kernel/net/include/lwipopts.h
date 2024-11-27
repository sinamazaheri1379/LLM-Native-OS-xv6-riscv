//
// Created by sina-mazaheri on 11/26/24.
//

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

// No operating system
#define NO_SYS                  1

// Memory options
#define MEM_ALIGNMENT          4
#define MEM_SIZE              16000

// Enable protocols
#define LWIP_ARP               1
#define LWIP_ETHERNET         1
#define LWIP_IPV4             1
#define LWIP_ICMP             1
#define LWIP_TCP              1
#define LWIP_UDP              1

// ARP options
#define ARP_TABLE_SIZE        10
#define ARP_QUEUEING          0

// Debugging
#define LWIP_DEBUG            0

#endif
