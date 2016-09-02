#ifndef __CTRCOMMON_SOUND_HPP__
#define __CTRCOMMON_SOUND_HPP__

#include "ctrcommon/types.hpp"

#define SOUND_CHANNEL_COUNT 8

typedef enum {
    SAMPLE_PCM8,
    SAMPLE_PCM16
} SampleFormat;

void* soundAlloc(u32 size);
void soundFree(void* mem);
bool soundPlay(u32 channel, void *samples, u32 numSamples, SampleFormat format, u32 sampleRate, float leftVolume, float rightVolume, bool loop);
bool soundStop(u32 channel);
bool soundFlush();

#endif
