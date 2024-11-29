//
// Created by sina-mazaheri on 11/27/24.
//
#ifndef _VIRTIO_NET_H
#define _VIRTIO_NET_H

#include "types.h"

// virtio-net device registers
#define VIRTIO_NET_MAGIC         0x74726976
#define VIRTIO_NET_VERSION       2
#define VIRTIO_NET_DEVICEID     1  // network device
#define VIRTIO_NET_VENDORID     0x554d4551

// Register offsets and ranges
#define VIRTIO_NET_REG_BASE      0x10002000  // Changed from 0x10001000
#define VIRTIO_NET_REG_SIZE      0x1000

// Register offsets
#define VIRTIO_NET_MAGIC_VALUE      0x000
#define VIRTIO_NET_VERSION_R        0x004
#define VIRTIO_NET_DEVICE_ID        0x008
#define VIRTIO_NET_VENDOR_ID        0x00c
#define VIRTIO_NET_DEVICE_FEATURES  0x010
#define VIRTIO_NET_DRIVER_FEATURES  0x020
#define VIRTIO_NET_QUEUE_SEL        0x030
#define VIRTIO_NET_QUEUE_NUM_MAX    0x034
#define VIRTIO_NET_QUEUE_NUM        0x038
#define VIRTIO_NET_QUEUE_READY      0x044
#define VIRTIO_NET_QUEUE_NOTIFY     0x050
#define VIRTIO_NET_INTERRUPT_STATUS 0x060
#define VIRTIO_NET_INTERRUPT_ACK    0x064
#define VIRTIO_NET_STATUS           0x070

// Queue configuration registers
#define VIRTIO_NET_QUEUE_DESC_LOW   0x080
#define VIRTIO_NET_QUEUE_DESC_HIGH  0x084
#define VIRTIO_NET_DRIVER_DESC_LOW  0x090
#define VIRTIO_NET_DRIVER_DESC_HIGH 0x094
#define VIRTIO_NET_DEVICE_DESC_LOW  0x0a0
#define VIRTIO_NET_DEVICE_DESC_HIGH 0x0a4

// Device features bits
#define VIRTIO_NET_F_CSUM       (1 << 0)  // Device handles packets with partial checksum
#define VIRTIO_NET_F_MAC        (1 << 5)  // Device has given MAC address
#define VIRTIO_NET_F_STATUS     (1 << 16) // Configuration status field is available
#define VIRTIO_NET_F_HOST_TSO4  (1 << 11) // Host handles TSOv4
#define VIRTIO_NET_F_GUEST_TSO4 (1 << 12) // Guest handles TSOv4
#define VIRTIO_NET_F_HOST_UFO   (1 << 13) // Host handles UFO
#define VIRTIO_NET_F_HOST_ECN   (1 << 14) // Host handles ECN
#define VIRTIO_NET_F_GUEST_UFO  (1 << 15) // Guest handles UFO

// Device status register bits
#define VIRTIO_NET_S_ACKNOWLEDGE    1
#define VIRTIO_NET_S_DRIVER         2
#define VIRTIO_NET_S_DRIVER_OK      4
#define VIRTIO_NET_S_FEATURES_OK    8
#define VIRTIO_NET_S_FAILED         0x80

// Queue numbers and configuration
#define VIRTIO_NET_Q_RX        0
#define VIRTIO_NET_Q_TX        1
#define NET_QUEUE_SIZE       256  // Queue size must be power of 2
#define NET_BUFFER_SIZE      1526 // Max ethernet frame size + header

// Ring descriptor flags
#define VRING_DESC_F_NEXT    1  // Buffer continues via next
#define VRING_DESC_F_WRITE   2  // Buffer is write-only (otherwise read-only)

// Ethernet protocol types
#define ETH_P_IP    0x0800  // Internet Protocol packet
#define ETH_P_ARP   0x0806  // Address Resolution packet
#define ETH_P_IPV6  0x86DD  // IPv6 packet

// Header sizes
#define VNET_HDR_SIZE    sizeof(struct virtio_net_hdr)
#define ETH_HDR_SIZE     sizeof(struct eth_hdr)
#define MAX_PKT_SIZE     (NET_BUFFER_SIZE - VNET_HDR_SIZE)

// Error Codes
#define VIRTIO_NET_ERR_OK       0
#define VIRTIO_NET_ERR_NOMEM   -1
#define VIRTIO_NET_ERR_QUEUE   -2
#define VIRTIO_NET_ERR_DEV     -3
#define VIRTIO_NET_ERR_PKT     -4
#define VIRTIO_NET_ERR_INIT    -5
#define VIRTIO_NET_ERR_INVAL   -6

// Device ring structures
struct virtq_desc {
    uint64 addr;
    uint32 len;
    uint16 flags;
    uint16 next;
};

struct virtq_avail {
    uint16 flags;
    uint16 idx;
    uint16 ring[NET_QUEUE_SIZE];
    uint16 unused;
};

struct virtq_used_elem {
    uint32 id;
    uint32 len;
};

struct virtq_used {
    uint16 flags;
    uint16 idx;
    struct virtq_used_elem ring[NET_QUEUE_SIZE];
};

// virtio-net header format for each packet
struct virtio_net_hdr {
    uint8 flags;
    uint8 gso_type;
    uint16 hdr_len;
    uint16 gso_size;
    uint16 csum_start;
    uint16 csum_offset;
    uint16 num_buffers;
};

// Network buffer structure
struct net_buffer {
    struct virtio_net_hdr vh;           // virtio header
    char data[NET_BUFFER_SIZE - VNET_HDR_SIZE]; // actual packet data
    uint16 len;                         // actual length of data
    uint16 next;                        // index of next buffer in chain
};

// Ethernet frame structure
struct eth_hdr {
    uint8 dhost[6];   // Destination MAC
    uint8 shost[6];   // Source MAC
    uint16 type;      // Protocol type
};

// Function declarations
void virtio_net_init(void);
void virtio_net_cleanup(void);
int virtio_net_transmit(uint8* data, uint16 len);
void virtio_net_intr(void);
int virtio_net_recv(uint8* data, uint16* len);

#endif // _VIRTIO_NET_H
