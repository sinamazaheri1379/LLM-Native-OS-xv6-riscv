//
// Created by sina-mazaheri on 11/27/24.
//

#ifndef _VIRTIO_NET_H_
#define _VIRTIO_NET_H_

// virtio-net device registers, mapped starting at 0x10001000
#define VIRTIO_NET_MAGIC         0x74726976
#define VIRTIO_NET_VERSION       2
#define VIRTIO_NET_DEVICEID     1  // network device
#define VIRTIO_NET_VENDORID     0x554d4551

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

// Device status register bits
#define VIRTIO_NET_S_ACKNOWLEDGE    1
#define VIRTIO_NET_S_DRIVER         2
#define VIRTIO_NET_S_DRIVER_OK      4
#define VIRTIO_NET_S_FEATURES_OK    8

// virtio-net header format
struct virtio_net_hdr {
    uint8 flags;
    uint8 gso_type;
    uint16 hdr_len;
    uint16 gso_size;
    uint16 csum_start;
    uint16 csum_offset;
    uint16 num_buffers;
};

// Function declarations
void virtio_net_init(void);
int virtio_net_transmit(uint8 *data, uint16 len);
void virtio_net_intr(void);

#endif
