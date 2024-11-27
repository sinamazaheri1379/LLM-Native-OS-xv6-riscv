//
// Created by sina-mazaheri on 11/27/24.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "virtio_net.h"

// Memory mapped registers
static volatile uint32 *net_regs;

// Device info
static struct {
    struct spinlock lock;
    int initialized;
    uint8 mac[6];  // Device MAC address
} net_info;

// Initialize virtio network device
void
virtio_net_init(void)
{
    uint32 status = 0;

    initlock(&net_info.lock, "virtio_net");

    // Reset device
    net_regs = (volatile uint32*)VIRTIO1;

    // Check device ID
    if(net_regs[VIRTIO_NET_DEVICE_ID/4] != VIRTIO_NET_DEVICEID) {
        printf("virtio_net: wrong device id %d\n",
               net_regs[VIRTIO_NET_DEVICE_ID/4]);
        return;
    }

    // Check version
    if(net_regs[VIRTIO_NET_VERSION_R/4] != VIRTIO_NET_VERSION) {
        printf("virtio_net: wrong version %d\n",
               net_regs[VIRTIO_NET_VERSION_R/4]);
        return;
    }

    // Reset device
    net_regs[VIRTIO_NET_STATUS/4] = status;

    // Set ACKNOWLEDGE status bit
    status |= VIRTIO_NET_S_ACKNOWLEDGE;
    net_regs[VIRTIO_NET_STATUS/4] = status;

    // Set DRIVER status bit
    status |= VIRTIO_NET_S_DRIVER;
    net_regs[VIRTIO_NET_STATUS/4] = status;

    // Read and save MAC address
    acquire(&net_info.lock);
    uint32 mac_lo = net_regs[0x100/4];
    uint32 mac_hi = net_regs[0x104/4];
    net_info.mac[0] = mac_lo & 0xFF;
    net_info.mac[1] = (mac_lo >> 8) & 0xFF;
    net_info.mac[2] = (mac_lo >> 16) & 0xFF;
    net_info.mac[3] = (mac_lo >> 24) & 0xFF;
    net_info.mac[4] = mac_hi & 0xFF;
    net_info.mac[5] = (mac_hi >> 8) & 0xFF;
    net_info.initialized = 1;
    release(&net_info.lock);

    printf("virtio-net: initialized with MAC %x:%x:%x:%x:%x:%x\n",
           net_info.mac[0], net_info.mac[1], net_info.mac[2],
           net_info.mac[3], net_info.mac[4], net_info.mac[5]);

    // Enable interrupts
    plic_complete(VIRTIO1_IRQ);
}

// Interrupt handler
void
virtio_net_intr(void)
{
    // Read interrupt status
    uint32 status = net_regs[VIRTIO_NET_INTERRUPT_STATUS/4];

    // Acknowledge the interrupt
    net_regs[VIRTIO_NET_INTERRUPT_ACK/4] = status;

    printf("virtio-net: interrupt, status=%x\n", status);
}