//
// Created by sina-mazaheri on 11/26/24.
//

#include "e1000.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/ethernet.h"
#include "lwip/etharp.h"

// Register definitions
#define E1000_TDBAL    0x03800  // TX Descriptor Base Low
#define E1000_TDBAH    0x03804  // TX Descriptor Base High
#define E1000_TDLEN    0x03808  // TX Descriptor Length
#define E1000_TDH      0x03810  // TX Descriptor Head
#define E1000_TDT      0x03818  // TX Descriptor Tail
#define E1000_TCTL     0x00400  // TX Control
#define E1000_TIPG     0x00410  // TX Inter-packet gap
#define E1000_RDBAL    0x02800  // RX Descriptor Base Low
#define E1000_RDBAH    0x02804  // RX Descriptor Base High
#define E1000_RDLEN    0x02808  // RX Descriptor Length
#define E1000_RDH      0x02810  // RX Descriptor Head
#define E1000_RDT      0x02818  // RX Descriptor Tail
#define E1000_RCTL     0x00100  // RX Control

// Ring buffer sizes
#define TX_RING_SIZE 16
#define RX_RING_SIZE 16

// Transmit descriptor
struct tx_desc {
    uint64 addr;    // Address of packet data
    uint16 length;  // Data length
    uint8  cso;     // Checksum offset
    uint8  cmd;     // Command field
    uint8  status;  // Status field
    uint8  css;     // Checksum start field
    uint16 special;
};

// Receive descriptor
struct rx_desc {
    uint64 addr;    // Address of packet data
    uint16 length;  // Data length
    uint16 csum;    // Checksum
    uint8  status;  // Status field
    uint8  errors;  // Error field
    uint16 special;
};

// Device state
static volatile uint32 *regs;           // Device registers
static struct tx_desc *tx_ring;         // Transmit ring
static struct rx_desc *rx_ring;         // Receive ring
static uint8 tx_ring_buf[TX_RING_SIZE][1518]; // Transmit buffers
static uint8 rx_ring_buf[RX_RING_SIZE][1518]; // Receive buffers
static uint32 tx_next, rx_next;         // Next descriptor to use

err_t
e1000_init(struct netif *netif)
{
    // Set netif parameters
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif->mtu = 1500;
    netif->linkoutput = e1000_output;
    netif->output = etharp_output;

    // Set MAC address
    netif->hwaddr[0] = 0x52;
    netif->hwaddr[1] = 0x54;
    netif->hwaddr[2] = 0x00;
    netif->hwaddr[3] = 0x12;
    netif->hwaddr[4] = 0x34;
    netif->hwaddr[5] = 0x56;

    // Initialize rings
    tx_ring = kalloc();
    rx_ring = kalloc();
    memset(tx_ring, 0, sizeof(struct tx_desc) * TX_RING_SIZE);
    memset(rx_ring, 0, sizeof(struct rx_desc) * RX_RING_SIZE);

    // Setup receive buffer addresses
    for(int i = 0; i < RX_RING_SIZE; i++){
        rx_ring[i].addr = (uint64)rx_ring_buf[i];
        rx_ring[i].status = 0;
    }

    // Initialize hardware
    // ... Configure e1000 registers ...

    return ERR_OK;
}

err_t
e1000_output(struct netif *netif, struct pbuf *p)
{
    // Get next transmit descriptor
    struct tx_desc *desc = &tx_ring[tx_next];

    // Wait if no descriptors available
    while(desc->status & 0x1)
        ;

    // Copy pbuf to transmit buffer
    uint8 *buf = tx_ring_buf[tx_next];
    for(struct pbuf *q = p; q != NULL; q = q->next){
        memcpy(buf, q->payload, q->len);
        buf += q->len;
    }

    // Setup descriptor
    desc->addr = (uint64)tx_ring_buf[tx_next];
    desc->length = p->tot_len;
    desc->cmd = 0x1 | 0x8;  // End of packet and report status
    desc->status = 0;

    // Update tail pointer
    tx_next = (tx_next + 1) % TX_RING_SIZE;
    regs[E1000_TDT] = tx_next;

    return ERR_OK;
}

struct pbuf*
e1000_recv(void)
{
    // Get next receive descriptor
    struct rx_desc *desc = &rx_ring[rx_next];

    // Return NULL if no packet available
    if(!(desc->status & 0x1))
        return NULL;

    // Allocate pbuf
    struct pbuf *p = pbuf_alloc(PBUF_RAW, desc->length, PBUF_POOL);
    if(!p)
        return NULL;

    // Copy packet data to pbuf
    pbuf_take(p, (void*)rx_ring_buf[rx_next], desc->length);

    // Reset descriptor
    desc->status = 0;
    rx_next = (rx_next + 1) % RX_RING_SIZE;
    regs[E1000_RDT] = rx_next;

    return p;
}
