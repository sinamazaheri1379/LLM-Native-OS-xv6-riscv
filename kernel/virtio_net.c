//
// Created by sina-mazaheri on 11/27/24.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "virtio_net.h"

// Memory mapped registers
static volatile uint32 *net_regs;

// Device info
static struct net_info{
    struct spinlock lock;
    int initialized;
    uint8 mac[6];     // Device MAC address
    uint32 features;  // Negotiated features
    int status;       // Device status
} net_info;

// Queue structures
struct virtqueue {
    struct virtq_desc *desc;   // Descriptor array
    struct virtq_avail *avail; // Available ring
    struct virtq_used *used;   // Used ring
    char free[NET_QUEUE_SIZE]; // Is descriptor free?
    uint16 used_idx;          // We've seen this many used by device
    struct spinlock lock;     // Protect access to this queue
    char *name;              // Queue name for debugging
};

// The device queues
static struct virtqueue rx_queue;
static struct virtqueue tx_queue;

// Network buffers
static struct net_buffer *buffers;  // Array of network buffers

static void
virtio_net_error(char *msg)
{
    printf("virtio-net error: %s\n", msg);
    acquire(&net_info.lock);
    net_info.status |= VIRTIO_NET_S_FAILED;
    release(&net_info.lock);
}

// Initialize a virtqueue
static int
vqueue_init(struct virtqueue *vq, char *name)
{
    // Initialize spinlock first
    initlock(&vq->lock, name);
    vq->name = name;

    // Allocate descriptor table
    if((vq->desc = kalloc()) == 0)
        return VIRTIO_NET_ERR_NOMEM;

    // Allocate available ring
    if((vq->avail = kalloc()) == 0) {
        kfree(vq->desc);
        vq->desc = 0;
        return VIRTIO_NET_ERR_NOMEM;
    }

    // Allocate used ring
    if((vq->used = kalloc()) == 0) {
        kfree(vq->desc);
        kfree(vq->avail);
        vq->desc = 0;
        vq->avail = 0;
        return VIRTIO_NET_ERR_NOMEM;
    }

    // Initialize all memory regions to zero
    memset(vq->desc, 0, PGSIZE);
    memset(vq->avail, 0, PGSIZE);
    memset(vq->used, 0, PGSIZE);

    // Mark all descriptors as free
    memset(vq->free, 1, NET_QUEUE_SIZE);
    vq->used_idx = 0;

    return VIRTIO_NET_ERR_OK;
}

// Find a free descriptor
static int
alloc_desc(struct virtqueue *vq)
{
    for(int i = 0; i < NET_QUEUE_SIZE; i++) {
        if(vq->free[i]) {
            vq->free[i] = 0;
            return i;
        }
    }
    return -1;
}

// Free a chain of descriptors
static void
free_chain(struct virtqueue *vq, int i)
{
    while(1) {
        if(i >= NET_QUEUE_SIZE) {
            panic("virtio_net: invalid descriptor index");
        }
        vq->free[i] = 1;
        if(vq->desc[i].flags & VRING_DESC_F_NEXT)
            i = vq->desc[i].next;
        else
            break;
    }
}

// Clean up a virtqueue
static void
vqueue_cleanup(struct virtqueue *vq)
{
    if(vq->desc) {
        kfree(vq->desc);
        vq->desc = 0;
    }
    if(vq->avail) {
        kfree(vq->avail);
        vq->avail = 0;
    }
    if(vq->used) {
        kfree(vq->used);
        vq->used = 0;
    }
}

// Initialize the virtio queues
static int
virtio_net_queue_init(void)
{
    static char rx_name[] = "virtio_net_rx";
    static char tx_name[] = "virtio_net_tx";
    int r;

    // Initialize the queues
    if((r = vqueue_init(&rx_queue, rx_name)) != VIRTIO_NET_ERR_OK) {
        return r;
    }

    if((r = vqueue_init(&tx_queue, tx_name)) != VIRTIO_NET_ERR_OK) {
        vqueue_cleanup(&rx_queue);
        return r;
    }

    // Allocate network buffers
    if((buffers = kalloc()) == 0) {
        vqueue_cleanup(&rx_queue);
        vqueue_cleanup(&tx_queue);
        return VIRTIO_NET_ERR_NOMEM;
    }
    memset(buffers, 0, PGSIZE);

    // Configure RX queue
    net_regs[VIRTIO_NET_QUEUE_SEL/4] = VIRTIO_NET_Q_RX;
    net_regs[VIRTIO_NET_QUEUE_NUM/4] = NET_QUEUE_SIZE;
    net_regs[VIRTIO_NET_QUEUE_DESC_LOW/4] = (uint64)rx_queue.desc;
    net_regs[VIRTIO_NET_QUEUE_DESC_HIGH/4] = ((uint64)rx_queue.desc >> 32);
    net_regs[VIRTIO_NET_DRIVER_DESC_LOW/4] = (uint64)rx_queue.avail;
    net_regs[VIRTIO_NET_DRIVER_DESC_HIGH/4] = ((uint64)rx_queue.avail >> 32);
    net_regs[VIRTIO_NET_DEVICE_DESC_LOW/4] = (uint64)rx_queue.used;
    net_regs[VIRTIO_NET_DEVICE_DESC_HIGH/4] = ((uint64)rx_queue.used >> 32);

    // Configure TX queue
    net_regs[VIRTIO_NET_QUEUE_SEL/4] = VIRTIO_NET_Q_TX;
    net_regs[VIRTIO_NET_QUEUE_NUM/4] = NET_QUEUE_SIZE;
    net_regs[VIRTIO_NET_QUEUE_DESC_LOW/4] = (uint64)tx_queue.desc;
    net_regs[VIRTIO_NET_QUEUE_DESC_HIGH/4] = ((uint64)tx_queue.desc >> 32);
    net_regs[VIRTIO_NET_DRIVER_DESC_LOW/4] = (uint64)tx_queue.avail;
    net_regs[VIRTIO_NET_DRIVER_DESC_HIGH/4] = ((uint64)tx_queue.avail >> 32);
    net_regs[VIRTIO_NET_DEVICE_DESC_LOW/4] = (uint64)tx_queue.used;
    net_regs[VIRTIO_NET_DEVICE_DESC_HIGH/4] = ((uint64)tx_queue.used >> 32);

    __sync_synchronize();

    // Setup initial receive buffers
    for(int i = 0; i < NET_QUEUE_SIZE; i++) {
        int desc = alloc_desc(&rx_queue);
        if(desc < 0) {
            virtio_net_error("out of rx descriptors");
            return VIRTIO_NET_ERR_QUEUE;
        }

        rx_queue.desc[desc].addr = (uint64)&buffers[i];
        rx_queue.desc[desc].len = NET_BUFFER_SIZE;
        rx_queue.desc[desc].flags = VRING_DESC_F_WRITE;
        rx_queue.avail->ring[rx_queue.avail->idx % NET_QUEUE_SIZE] = desc;
        rx_queue.avail->idx++;
    }

    // Mark queues as ready
    net_regs[VIRTIO_NET_QUEUE_SEL/4] = VIRTIO_NET_Q_RX;
    net_regs[VIRTIO_NET_QUEUE_READY/4] = 1;
    net_regs[VIRTIO_NET_QUEUE_SEL/4] = VIRTIO_NET_Q_TX;
    net_regs[VIRTIO_NET_QUEUE_READY/4] = 1;

    return VIRTIO_NET_ERR_OK;
}

// Initialize virtio network device
void
virtio_net_init(void)
{
    uint32 status = 0;
    initlock(&net_info.lock, "virtio_net");
    net_info.status = 0;

    // Reset device
    net_regs = (volatile uint32*)VIRTIO_NET_REG_BASE;  // Using correct base address
    net_regs[VIRTIO_NET_STATUS/4] = status;

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

    // Set ACKNOWLEDGE status bit
    status |= VIRTIO_NET_S_ACKNOWLEDGE;
    net_regs[VIRTIO_NET_STATUS/4] = status;

    // Set DRIVER status bit
    status |= VIRTIO_NET_S_DRIVER;
    net_regs[VIRTIO_NET_STATUS/4] = status;

    // Negotiate features
    uint32 features = net_regs[VIRTIO_NET_DEVICE_FEATURES/4];
    features &= ~(VIRTIO_NET_F_CSUM); // We don't support checksum offload
    features &= ~(VIRTIO_NET_F_HOST_TSO4 | VIRTIO_NET_F_GUEST_TSO4); // No TSO support
    net_info.features = features;

    // Write supported features
    net_regs[VIRTIO_NET_DRIVER_FEATURES/4] = features;

    // Set FEATURES_OK status bit
    status |= VIRTIO_NET_S_FEATURES_OK;
    net_regs[VIRTIO_NET_STATUS/4] = status;

    // Verify FEATURES_OK is still set
    if(!(net_regs[VIRTIO_NET_STATUS/4] & VIRTIO_NET_S_FEATURES_OK)) {
        virtio_net_error("feature negotiation failed");
        return;
    }
    // Initialize the queues
    if(virtio_net_queue_init() != VIRTIO_NET_ERR_OK) {
        status |= VIRTIO_NET_S_FAILED;
        net_regs[VIRTIO_NET_STATUS/4] = status;
        return;
    }

    // Set DRIVER_OK status bit
    status |= VIRTIO_NET_S_DRIVER_OK;
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
}

void
virtio_net_cleanup(void)
{
    acquire(&net_info.lock);
    if(!net_info.initialized) {
        release(&net_info.lock);
        return;
    }

    net_info.initialized = 0;
    release(&net_info.lock);

    if(buffers) {
        kfree(buffers);
        buffers = 0;
    }

    vqueue_cleanup(&rx_queue);
    vqueue_cleanup(&tx_queue);
}

// Transmit a packet
int
virtio_net_transmit(uint8 *data, uint16 len)
{
    // Check device state
    if(!net_info.initialized || net_info.status & VIRTIO_NET_S_FAILED)
        return VIRTIO_NET_ERR_DEV;

    // Validate packet length
    if(len > MAX_PKT_SIZE || len == 0)
        return VIRTIO_NET_ERR_PKT;

    acquire(&tx_queue.lock);

    // Get a free descriptor
    int desc = alloc_desc(&tx_queue);
    if(desc < 0) {
        release(&tx_queue.lock);
        return VIRTIO_NET_ERR_QUEUE;
    }

    // Setup virtio header and copy data
    struct net_buffer *buf = &buffers[desc];
    memset(&buf->vh, 0, sizeof(struct virtio_net_hdr));

    // Verify buffer space
    if(len > sizeof(buf->data)) {
        free_chain(&tx_queue, desc);
        release(&tx_queue.lock);
        return VIRTIO_NET_ERR_PKT;
    }

    // Copy data and setup length
    memmove(buf->data, data, len);
    buf->len = len;

    // Setup descriptor
    tx_queue.desc[desc].addr = (uint64)buf;
    tx_queue.desc[desc].len = len + sizeof(struct virtio_net_hdr);
    tx_queue.desc[desc].flags = 0;  // Device reads this buffer

    // Add to available ring
    tx_queue.avail->ring[tx_queue.avail->idx % NET_QUEUE_SIZE] = desc;
    __sync_synchronize();
    tx_queue.avail->idx++;

    // Notify device
    net_regs[VIRTIO_NET_QUEUE_NOTIFY/4] = VIRTIO_NET_Q_TX;

    release(&tx_queue.lock);
    return VIRTIO_NET_ERR_OK;
}

// Receive a packet into the provided buffer
// Returns length of received packet or error code
int
virtio_net_recv(uint8 *data, uint16 *len)
{
    if(!net_info.initialized || net_info.status & VIRTIO_NET_S_FAILED)
        return VIRTIO_NET_ERR_DEV;

    if(!data || !len)
        return VIRTIO_NET_ERR_INVAL;

    acquire(&rx_queue.lock);

    // Check if there are any received packets
    if(rx_queue.used->idx == rx_queue.used_idx) {
        release(&rx_queue.lock);
        return 0;  // No packets available
    }

    // Get descriptor of received packet
    int id = rx_queue.used->ring[rx_queue.used_idx % NET_QUEUE_SIZE].id;
    int pkt_len = rx_queue.used->ring[rx_queue.used_idx % NET_QUEUE_SIZE].len;

    if(id >= NET_QUEUE_SIZE) {
        release(&rx_queue.lock);
        return VIRTIO_NET_ERR_PKT;
    }

    struct net_buffer *buf = &buffers[id];

    // Skip virtio header
    pkt_len -= sizeof(struct virtio_net_hdr);
    if(pkt_len <= 0 || pkt_len > MAX_PKT_SIZE) {
        release(&rx_queue.lock);
        return VIRTIO_NET_ERR_PKT;
    }

    // Copy data to user buffer
    memmove(data, buf->data, pkt_len);
    *len = pkt_len;

    // Re-queue the buffer for receive
    rx_queue.desc[id].len = NET_BUFFER_SIZE;
    rx_queue.avail->ring[rx_queue.avail->idx % NET_QUEUE_SIZE] = id;
    __sync_synchronize();
    rx_queue.avail->idx++;
    rx_queue.used_idx++;

    release(&rx_queue.lock);
    return pkt_len;
}

// Improved interrupt handler
void
virtio_net_intr(void)
{
    acquire(&net_info.lock);
    if(!net_info.initialized) {
        printf("virtio-net: interrupt before initialization\n");
        release(&net_info.lock);
        return;
    }
    release(&net_info.lock);

    // Read interrupt status
    uint32 status = net_regs[VIRTIO_NET_INTERRUPT_STATUS/4];

    // Handle received packets
    if(status & 0x1) {  // RX interrupt
        acquire(&rx_queue.lock);

        while(rx_queue.used->idx != rx_queue.used_idx) {
            int id = rx_queue.used->ring[rx_queue.used_idx % NET_QUEUE_SIZE].id;
            int len = rx_queue.used->ring[rx_queue.used_idx % NET_QUEUE_SIZE].len;

            if(id >= NET_QUEUE_SIZE) {
                printf("virtio-net: invalid descriptor id %d\n", id);
                break;
            }

            if(len > NET_BUFFER_SIZE) {
                printf("virtio-net: received oversized packet (%d bytes)\n", len);
            } else if(len > sizeof(struct virtio_net_hdr)) {
//                struct net_buffer *buf = &buffers[id];
//                uint16 pkt_len = len - sizeof(struct virtio_net_hdr);

                // Check Ethernet header
//                struct eth_hdr *eth = (struct eth_hdr *)buf->data;

                // Print packet info for debugging
//                printf("virtio-net: received packet type 0x%x, length %d\n",
//                       ntohs(eth->type), pkt_len);

                // Here you would normally pass the packet to a network stack
                // e.g., process_packet(buf->data, pkt_len);
            }

            // Re-queue the buffer for receive
            rx_queue.desc[id].len = NET_BUFFER_SIZE;
            rx_queue.avail->ring[rx_queue.avail->idx % NET_QUEUE_SIZE] = id;
            __sync_synchronize();
            rx_queue.avail->idx++;
            rx_queue.used_idx++;
        }

        release(&rx_queue.lock);
    }

    // Handle transmitted packets
    if(status & 0x2) {  // TX interrupt
        acquire(&tx_queue.lock);

        while(tx_queue.used->idx != tx_queue.used_idx) {
            int id = tx_queue.used->ring[tx_queue.used_idx % NET_QUEUE_SIZE].id;

            if(id >= NET_QUEUE_SIZE) {
                printf("virtio-net: invalid tx descriptor id %d\n", id);
                break;
            }

            // Free the descriptor chain
            free_chain(&tx_queue, id);
            tx_queue.used_idx++;
        }

        release(&tx_queue.lock);
    }

    // Acknowledge the interrupt
    net_regs[VIRTIO_NET_INTERRUPT_ACK/4] = status;
}