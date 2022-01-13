/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2021 Dylan Lathrum
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

#include <Limelight.h>
#include <SDL.h>

typedef struct _VIDEO_STATS {
    uint32_t receivedFrames;
    uint32_t decodedFrames;
    uint32_t renderedFrames;
    uint32_t totalFrames;
    uint32_t networkDroppedFrames;
    uint32_t totalDecodeTime;
    uint32_t totalRenderTime;
    uint32_t lastRtt;
    uint32_t lastRttVariance;
    float receivedFps;
    float decodedFps;
    float renderedFps;
    uint32_t measurementStartTimestamp;
} VIDEO_STATS;

VIDEO_STATS m_ActiveWndVideoStats;
VIDEO_STATS m_GlobalVideoStats;

int m_FramesIn;
int m_FramesOut;

int m_LastFrameNumber;