#ifndef STREAM_PACKET_H
#define STREAM_PACKET_H
 
#include <stdint.h>
 
#ifdef __cplusplus
extern "C" {
#endif
 
typedef enum {
    STREAM_COLOR = 0,
    STREAM_DEPTH = 1
} StreamType;
 
/*
    Packet header sent before raw image data.
    Packed to avoid padding across C <-> Python boundaries.
 
    Changes from v1:
    - Added frame_index for receiver-side sync/ordering
    - Added channels: color = 3 (RGB), depth = 1
    - size field retained for easy malloc on the receiver
*/
#pragma pack(push, 1)
typedef struct {
    uint8_t  type;         // StreamType (STREAM_COLOR or STREAM_DEPTH)
    uint32_t frame_index;  // monotonic frame counter for sync
    uint16_t width;        // image width in pixels
    uint16_t height;       // image height in pixels
    uint8_t  channels;     // 3 = RGB, 1 = depth (int16)
    uint32_t size;         // payload size in bytes
} StreamHeader;
#pragma pack(pop)
 
#define STREAM_HEADER_SIZE (sizeof(StreamHeader))
 
#ifdef __cplusplus
}
#endif
 
#endif // STREAM_PACKET_H