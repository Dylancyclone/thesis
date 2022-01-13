/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2017 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "video.h"
#include "stats.h"
#include "egl.h"
#include "ffmpeg.h"
#ifdef HAVE_VAAPI
#include "ffmpeg_vaapi.h"
#endif

#include "../input/x11.h"
#include "../loop.h"

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define DECODER_BUFFER_SIZE 92*1024
#define X11_VDPAU_ACCELERATION ENABLE_HARDWARE_ACCELERATION_1
#define X11_VAAPI_ACCELERATION ENABLE_HARDWARE_ACCELERATION_2
#define SLICES_PER_FRAME 4

static char* ffmpeg_buffer = NULL;

static Display *display = NULL;
static Window window;

static int pipefd[2];

static int display_width;
static int display_height;

int m_LastFrameNumber = 0;
VIDEO_STATS m_ActiveWndVideoStats;
VIDEO_STATS m_GlobalVideoStats;

static int frame_handle(int pipefd) {
  AVFrame* frame = NULL;
  while (read(pipefd, &frame, sizeof(void*)) > 0);
  if (frame) {
    if (ffmpeg_decoder == SOFTWARE)
      egl_draw(frame->data);
    #ifdef HAVE_VAAPI
    else if (ffmpeg_decoder == VAAPI)
      vaapi_queue(frame, window, display_width, display_height);
    #endif
  }

  return LOOP_OK;
}

int x11_init(bool vdpau, bool vaapi) {
  SDL_zero(m_ActiveWndVideoStats);
  SDL_zero(m_GlobalVideoStats);
  
  XInitThreads();
  display = XOpenDisplay(NULL);
  if (!display)
    return 0;

  #ifdef HAVE_VAAPI
  if (vaapi && vaapi_init_lib(display) == 0)
    return INIT_VAAPI;
  #endif

  return INIT_EGL;
}

int x11_setup(int videoFormat, int width, int height, int redrawRate, void* context, int drFlags) {
  ffmpeg_buffer = malloc(DECODER_BUFFER_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
  if (ffmpeg_buffer == NULL) {
    fprintf(stderr, "Not enough memory\n");
    return -1;
  }

  if (!display) {
    fprintf(stderr, "Error: failed to open X display.\n");
    return -1;
  }

  if (drFlags & DISPLAY_FULLSCREEN) {
    Screen* screen = DefaultScreenOfDisplay(display);
    display_width = WidthOfScreen(screen);
    display_height = HeightOfScreen(screen);
  } else {
    display_width = width;
    display_height = height;
  }

  Window root = DefaultRootWindow(display);
  XSetWindowAttributes winattr = { .event_mask = PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask };
  window = XCreateWindow(display, root, 0, 0, display_width, display_height, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &winattr);
  XMapWindow(display, window);
  XStoreName(display, window, "Moonlight");

  if (drFlags & DISPLAY_FULLSCREEN) {
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent xev = {0};
    xev.type = ClientMessage;
    xev.xclient.window = window;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = fullscreen;
    xev.xclient.data.l[2] = 0;

    XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
  }
  XFlush(display);

  int avc_flags;
  if (drFlags & X11_VDPAU_ACCELERATION)
    avc_flags = VDPAU_ACCELERATION;
  else if (drFlags & X11_VAAPI_ACCELERATION)
    avc_flags = VAAPI_ACCELERATION;
  else
    avc_flags = SLICE_THREADING;

  if (ffmpeg_init(videoFormat, width, height, avc_flags, 2, SLICES_PER_FRAME) < 0) {
    fprintf(stderr, "Couldn't initialize video decoding\n");
    return -1;
  }

  if (ffmpeg_decoder == SOFTWARE)
    egl_init(display, window, width, height);

  if (pipe(pipefd) == -1) {
    fprintf(stderr, "Can't create communication channel between threads\n");
    return -2;
  }
  loop_add_fd(pipefd[0], &frame_handle, POLLIN);
  fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

  x11_input_init(display, window);

  return 0;
}

int x11_setup_vdpau(int videoFormat, int width, int height, int redrawRate, void* context, int drFlags) {
  return x11_setup(videoFormat, width, height, redrawRate, context, drFlags | X11_VDPAU_ACCELERATION);
}

int x11_setup_vaapi(int videoFormat, int width, int height, int redrawRate, void* context, int drFlags) {
  return x11_setup(videoFormat, width, height, redrawRate, context, drFlags | X11_VAAPI_ACCELERATION);
}

void stringifyVideoStats(VIDEO_STATS* stats, char* output)
{
    int offset = 0;

    // Start with an empty string
    output[offset] = 0;

    if (stats->receivedFps > 0) {
        offset += sprintf(&output[offset],
                          "Incoming frame rate from network: %.2f FPS\n"
                          "Decoding frame rate: %.2f FPS\n"
                          "Rendering frame rate: %.2f FPS\n",
                          stats->receivedFps,
                          stats->decodedFps,
                          stats->renderedFps);
    }

    if (stats->renderedFrames != 0) {
        char rttString[32];

        if (stats->lastRtt != 0) {
            sprintf(rttString, "%u ms (variance: %u ms)", stats->lastRtt, stats->lastRttVariance);
        }
        else {
            sprintf(rttString, "N/A");
        }

        offset += sprintf(&output[offset],
                          "Frames dropped by your network connection: %.2f%%\n"
                          "Average network latency: %s\n"
                          "Average decoding time: %.2f ms\n"
                          "Average rendering time (including monitor V-sync latency): %.2f ms\n",
                          (float)stats->networkDroppedFrames / stats->totalFrames * 100,
                          rttString,
                          (float)stats->totalDecodeTime / stats->decodedFrames,
                          (float)stats->totalRenderTime / stats->renderedFrames);
    }
}

void x11_cleanup() {
  if (m_GlobalVideoStats.renderedFps > 0 || m_GlobalVideoStats.renderedFrames != 0) {
    char videoStatsStr[512];
    stringifyVideoStats(&m_GlobalVideoStats, videoStatsStr);

    printf("\n\nGlobal Video Stats\n----------------------------------------------------------\n");
    printf("%s\n\n", videoStatsStr);
    printf("m_GlobalVideoStats.networkDroppedFrames%d\n\n", m_GlobalVideoStats.networkDroppedFrames);
  }
  ffmpeg_destroy();
  egl_destroy();
}

void addVideoStats(VIDEO_STATS* src, VIDEO_STATS* dst)
{
    dst->receivedFrames += src->receivedFrames;
    dst->decodedFrames += src->decodedFrames;
    dst->renderedFrames += src->renderedFrames;
    dst->totalFrames += src->totalFrames;
    dst->networkDroppedFrames += src->networkDroppedFrames;
    dst->totalDecodeTime += src->totalDecodeTime;
    dst->totalRenderTime += src->totalRenderTime;

    if (!LiGetEstimatedRttInfo(&dst->lastRtt, &dst->lastRttVariance)) {
        dst->lastRtt = 0;
        dst->lastRttVariance = 0;
    }
    else {
        // Our logic to determine if RTT is valid depends on us never
        // getting an RTT of 0. ENet currently ensures RTTs are >= 1.
        SDL_assert(dst->lastRtt > 0);
    }

    uint32_t now = SDL_GetTicks();

    // Initialize the measurement start point if this is the first video stat window
    if (!dst->measurementStartTimestamp) {
        dst->measurementStartTimestamp = src->measurementStartTimestamp;
    }

    // The following code assumes the global measure was already started first
    SDL_assert(dst->measurementStartTimestamp <= src->measurementStartTimestamp);

    dst->receivedFps = (float)dst->receivedFrames / ((float)(now - dst->measurementStartTimestamp) / 1000);
    dst->decodedFps = (float)dst->decodedFrames / ((float)(now - dst->measurementStartTimestamp) / 1000);
    dst->renderedFps = (float)dst->renderedFrames / ((float)(now - dst->measurementStartTimestamp) / 1000);
}

int x11_submit_decode_unit(PDECODE_UNIT decodeUnit) {
  if (decodeUnit->fullLength < DECODER_BUFFER_SIZE) {
    PLENTRY entry = decodeUnit->bufferList;
    int length = 0;
    if (!m_LastFrameNumber) {
        m_ActiveWndVideoStats.measurementStartTimestamp = SDL_GetTicks();
        m_LastFrameNumber = decodeUnit->frameNumber;
    }
    else {
        // Any frame number greater than m_LastFrameNumber + 1 represents a dropped frame
        m_ActiveWndVideoStats.networkDroppedFrames += decodeUnit->frameNumber - (m_LastFrameNumber + 1);
        m_ActiveWndVideoStats.totalFrames += decodeUnit->frameNumber - (m_LastFrameNumber + 1);
        m_LastFrameNumber = decodeUnit->frameNumber;
    }

    // Flip stats windows roughly every second
    if (SDL_TICKS_PASSED(SDL_GetTicks(), m_ActiveWndVideoStats.measurementStartTimestamp + 1000)) {
        // Accumulate these values into the global stats
        
        addVideoStats(&m_ActiveWndVideoStats, &m_GlobalVideoStats);

        // Clear it for next window
        SDL_zero(m_ActiveWndVideoStats);
        m_ActiveWndVideoStats.measurementStartTimestamp = SDL_GetTicks();
    }

    m_ActiveWndVideoStats.receivedFrames++;
    m_ActiveWndVideoStats.totalFrames++;

    while (entry != NULL) {
      memcpy(ffmpeg_buffer+length, entry->data, entry->length);
      length += entry->length;
      entry = entry->next;
    }
    ffmpeg_decode(ffmpeg_buffer, length);
    AVFrame* frame = ffmpeg_get_frame(true);
    if (frame != NULL)
      m_ActiveWndVideoStats.totalDecodeTime += LiGetMillis() - decodeUnit->enqueueTimeMs;
      m_ActiveWndVideoStats.decodedFrames++;

      u_int32_t beforeRender = SDL_GetTicks();

      write(pipefd[1], &frame, sizeof(void*));

      u_int32_t afterRender = SDL_GetTicks();
      m_ActiveWndVideoStats.totalRenderTime += afterRender - beforeRender;

      m_ActiveWndVideoStats.renderedFrames++;
  }

  return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_x11 = {
  .setup = x11_setup,
  .cleanup = x11_cleanup,
  .submitDecodeUnit = x11_submit_decode_unit,
  .capabilities = CAPABILITY_SLICES_PER_FRAME(SLICES_PER_FRAME) | CAPABILITY_REFERENCE_FRAME_INVALIDATION_AVC | CAPABILITY_DIRECT_SUBMIT,
};

DECODER_RENDERER_CALLBACKS decoder_callbacks_x11_vdpau = {
  .setup = x11_setup_vdpau,
  .cleanup = x11_cleanup,
  .submitDecodeUnit = x11_submit_decode_unit,
  .capabilities = CAPABILITY_DIRECT_SUBMIT,
};

DECODER_RENDERER_CALLBACKS decoder_callbacks_x11_vaapi = {
  .setup = x11_setup_vaapi,
  .cleanup = x11_cleanup,
  .submitDecodeUnit = x11_submit_decode_unit,
  .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
