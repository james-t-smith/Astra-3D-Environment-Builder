
// This file is part of the Orbbec Astra SDK [https://orbbec3d.com]
// Copyright (c) 2015-2017 Orbbec 3D
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Be excellent to each other.
#include <astra/capi/astra.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <key_handler.h>
#include <zmq.h>
 
#include "stream_packet.h"
 
// ---------------------------------------------------------------------------
// ZMQ globals — one context, two PUB sockets (one per stream type)
// ---------------------------------------------------------------------------
static void* g_zmq_ctx    = NULL;
static void* g_color_sock = NULL;
static void* g_depth_sock = NULL;
 
#define COLOR_ENDPOINT "tcp://*:5550"
#define DEPTH_ENDPOINT "tcp://*:5551"
 
// ---------------------------------------------------------------------------
// send_frame_zmq
//
// Sends color and depth as two separate two-part ZMQ messages:
//   Part 1: StreamHeader  (fixed-size, packed)
//   Part 2: raw pixel data
//
// Color payload : width * height * 3  bytes  (uint8 RGB)
// Depth payload : width * height * 2  bytes  (int16 mm)
// ---------------------------------------------------------------------------
void send_frame_zmq(astra_colorframe_t colorFrame, astra_depthframe_t depthFrame)
{
    // ---- COLOR ----
    if (colorFrame != NULL)
    {
        astra_image_metadata_t meta;
        astra_rgb_pixel_t*     pixels;
        uint32_t               byteLen;
 
        astra_colorframe_get_data_rgb_ptr(colorFrame, &pixels, &byteLen);
        astra_colorframe_get_metadata(colorFrame, &meta);
 
        astra_frame_index_t fi;
        astra_colorframe_get_frameindex(colorFrame, &fi);
 
        StreamHeader hdr;
        hdr.type        = (uint8_t)STREAM_COLOR;
        hdr.frame_index = (uint32_t)fi;
        hdr.width       = (uint16_t)meta.width;
        hdr.height      = (uint16_t)meta.height;
        hdr.channels    = 3;
        hdr.size        = byteLen;
 
        // Send header then payload as a two-part message (SNDMORE)
        zmq_send(g_color_sock, &hdr, STREAM_HEADER_SIZE, ZMQ_SNDMORE);
        zmq_send(g_color_sock, pixels, byteLen, 0);
    }
 
    // ---- DEPTH ----
    if (depthFrame != NULL)
    {
        astra_image_metadata_t meta;
        int16_t*               pixels;
        uint32_t               byteLen;
 
        astra_depthframe_get_data_ptr(depthFrame, &pixels, &byteLen);
        astra_depthframe_get_metadata(depthFrame, &meta);
 
        astra_frame_index_t fi;
        astra_depthframe_get_frameindex(depthFrame, &fi);
 
        StreamHeader hdr;
        hdr.type        = (uint8_t)STREAM_DEPTH;
        hdr.frame_index = (uint32_t)fi;
        hdr.width       = (uint16_t)meta.width;
        hdr.height      = (uint16_t)meta.height;
        hdr.channels    = 1;
        hdr.size        = byteLen;
 
        zmq_send(g_depth_sock, &hdr, STREAM_HEADER_SIZE, ZMQ_SNDMORE);
        zmq_send(g_depth_sock, pixels, byteLen, 0);
    }
}
 
// ---------------------------------------------------------------------------
// Debug helpers (kept from original)
// ---------------------------------------------------------------------------
void print_color(astra_colorframe_t colorFrame)
{
    astra_image_metadata_t metadata;
    astra_rgb_pixel_t*     colorData_rgb;
    uint32_t               colorByteLength;
 
    astra_colorframe_get_data_rgb_ptr(colorFrame, &colorData_rgb, &colorByteLength);
    astra_colorframe_get_metadata(colorFrame, &metadata);
 
    int    width  = metadata.width;
    int    height = metadata.height;
    size_t index  = (size_t)((width * (height / 2)) + (width / 2));
 
    astra_frame_index_t frameIndex;
    astra_colorframe_get_frameindex(colorFrame, &frameIndex);
 
    astra_rgb_pixel_t middle = colorData_rgb[index];
    printf("color frameIndex: %d  r: %d  g: %d  b: %d\n",
           frameIndex, (int)middle.r, (int)middle.g, (int)middle.b);
}
 
void print_depth(astra_depthframe_t depthFrame)
{
    astra_image_metadata_t metadata;
    int16_t*               depthData;
    uint32_t               depthLength;
 
    astra_depthframe_get_data_ptr(depthFrame, &depthData, &depthLength);
    astra_depthframe_get_metadata(depthFrame, &metadata);
 
    int    width  = metadata.width;
    int    height = metadata.height;
    size_t index  = (size_t)((width * (height / 2)) + (width / 2));
 
    astra_frame_index_t frameIndex;
    astra_depthframe_get_frameindex(depthFrame, &frameIndex);
 
    printf("depth frameIndex: %d  value: %d mm\n", frameIndex, (int)depthData[index]);
}
 
// ---------------------------------------------------------------------------
// Frame callback
// ---------------------------------------------------------------------------
void frame_ready(void* clientTag, astra_reader_t reader, astra_reader_frame_t frame)
{
    astra_colorframe_t colorFrame;
    astra_frame_get_colorframe(frame, &colorFrame);
 
    astra_depthframe_t depthFrame;
    astra_frame_get_depthframe(frame, &depthFrame);
 
    if (colorFrame != NULL) print_color(colorFrame);
    if (depthFrame != NULL) print_depth(depthFrame);
 
    send_frame_zmq(colorFrame, depthFrame);
}
 
// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    set_key_handler();
 
    // -- ZMQ setup --
    g_zmq_ctx = zmq_ctx_new();
    if (!g_zmq_ctx) { fprintf(stderr, "zmq_ctx_new failed\n"); return 1; }
 
    g_color_sock = zmq_socket(g_zmq_ctx, ZMQ_PUB);
    g_depth_sock = zmq_socket(g_zmq_ctx, ZMQ_PUB);
 
    if (zmq_bind(g_color_sock, COLOR_ENDPOINT) != 0) {
        fprintf(stderr, "zmq_bind color failed: %s\n", zmq_strerror(zmq_errno()));
        return 1;
    }
    if (zmq_bind(g_depth_sock, DEPTH_ENDPOINT) != 0) {
        fprintf(stderr, "zmq_bind depth failed: %s\n", zmq_strerror(zmq_errno()));
        return 1;
    }
 
    printf("Streaming color on %s, depth on %s\n", COLOR_ENDPOINT, DEPTH_ENDPOINT);
 
    // -- Astra setup --
    astra_initialize();
 
    astra_streamsetconnection_t sensor;
    astra_streamset_open("device/default", &sensor);
 
    astra_reader_t reader;
    astra_reader_create(sensor, &reader);
 
    astra_colorstream_t colorStream;
    astra_reader_get_colorstream(reader, &colorStream);
    astra_stream_start(colorStream);
 
    astra_depthstream_t depthStream;
    astra_reader_get_depthstream(reader, &depthStream);
    astra_stream_start(depthStream);
 
    astra_reader_callback_id_t callbackId;
    astra_reader_register_frame_ready_callback(reader, &frame_ready, NULL, &callbackId);
 
    do
    {
        astra_update();
    } while (shouldContinue);
 
    // -- Cleanup --
    astra_reader_unregister_frame_ready_callback(&callbackId);
    astra_reader_destroy(&reader);
    astra_streamset_close(&sensor);
    astra_terminate();
 
    zmq_close(g_color_sock);
    zmq_close(g_depth_sock);
    zmq_ctx_destroy(g_zmq_ctx);
 
    return 0;
}