#include "common.h"

const uint8 conversion_rf5c68[256] =
{
0x7f,0x7f,0x7e,0x7d,0x7c,0x7b,0x7a,0x79,0x78,0x77,0x76,0x75,0x74,0x73,0x72,0x71,
0x70,0x6f,0x6e,0x6d,0x6c,0x6b,0x6a,0x69,0x68,0x67,0x66,0x65,0x64,0x63,0x62,0x61,
0x60,0x5f,0x5e,0x5d,0x5c,0x5b,0x5a,0x59,0x58,0x57,0x56,0x55,0x54,0x53,0x52,0x51,
0x50,0x4f,0x4e,0x4d,0x4c,0x4b,0x4a,0x49,0x48,0x47,0x46,0x45,0x44,0x43,0x42,0x41,
0x40,0x3f,0x3e,0x3d,0x3c,0x3b,0x3a,0x39,0x38,0x37,0x36,0x35,0x34,0x33,0x32,0x31,
0x30,0x2f,0x2e,0x2d,0x2c,0x2b,0x2a,0x29,0x28,0x27,0x26,0x25,0x24,0x23,0x22,0x21,
0x20,0x1f,0x1e,0x1d,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,
0x10,0xf ,0xe ,0xd ,0xc ,0xb ,0xa ,0x9 ,0x8 ,0x7 ,0x6 ,0x5 ,0x4 ,0x3 ,0x2 ,0x1 ,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xfe
};

const uint8 ADPCM4_ADAPT[] = { // IWRAM !
    192,192,136,136,128,128,128,128, // -8..-1
    112,128,128,128,128,136,136,192, //  0..+7
};

    extern const void* TRACKS_AD4;

    #define sndADPCM4_fill sndADPCM4_c
    #define sndPCM_fill    sndPCM_c
    #define sndPCM_mix     sndPCM_c
    #define sndClear(b)    dmaFill(b, SND_ENCODE(0), SND_SAMPLES * sizeof(b[0]))

#define DECODE_ADPCM4(n)\
    tap = zM2 + tap - (tap >> 3);\
    *buffer++ = conversion_rf5c68[SND_ENCODE(X_CLAMP(tap >> 8, SND_MIN, SND_MAX))];\
    res = ((n&0xF) ^ 8) - 8;\
    out = res*quant + (zM1 - zM2);\
    zM2 = zM1;\
    zM1 = out;\
    quant = (quant*(int32)ADPCM4_ADAPT[res+8] + 127) >> 7;\

void sndADPCM4_c(ADPCM4_STATE &state, uint8* buffer, const uint8* data, int32 size)
{
    int32 zM1   = state.zM1;
    int32 zM2   = state.zM2;
    int32 tap   = state.tap;
    int32 quant = state.quant;
    int32 res, out;
    
    for (int32 i=0; i < size; i++)
    {
        uint32 n = *data++;
        DECODE_ADPCM4(n);
        n >>= 4;
        DECODE_ADPCM4(n);
    }
    
    state.zM1   = zM1;
    state.zM2   = zM2;
    state.tap   = tap;
    state.quant = quant;
}

int32 sndPCM_c(int32 pos, int32 inc, int32 size, int32 volume, const uint8* data, uint8* buffer)
{
    int32 last = pos + SND_SAMPLES * inc;
    if (last > size) {
        last = size;
    }

    int32 amp;
    while (pos < last)
    {
	const uint32 temp = (*(uint8*)buffer);
	if(temp < 0x80)
	{
	    amp = temp;
	}
	else
	{
	    amp = -(int32)(temp & 0x7F);
	}
        amp += (SND_DECODE(data[pos >> SND_FIXED_SHIFT]) * volume) >> SND_VOL_SHIFT;
        *buffer++ = conversion_rf5c68[SND_ENCODE(X_CLAMP(amp, SND_MIN, SND_MAX))];
        pos += inc;
    }

    return pos;
}

struct Music
{
    const uint8*  data;
    int32         size;
    int32         pos;
    ADPCM4_STATE  state;

    void fill(uint8* buffer)
    {
        int32 len = X_MIN(size - pos, SND_SAMPLES >> 1);

        sndADPCM4_fill(state, buffer, data + pos, len);

        pos += len;

        if (pos >= size)
        {
            data = NULL;
            memset(buffer + (len << 1), SND_ENCODE(0), (SND_SAMPLES - (len << 1)) * sizeof(buffer[0]));
        }
    }
};

struct Sample
{
    int32        pos;
    int32        inc;
    int32        size;
    int32        volume;
    const uint8* data;

    void mix(uint8* buffer)
    {
        pos = sndPCM_mix(pos, inc, size, volume, data, buffer);

        if (pos >= size)
        {
            data = NULL;
        }
    }

    void fill(uint8* buffer)
    {
        pos = sndPCM_fill(pos, inc, size, volume, data, buffer);

        if (pos >= size)
        {
            data = NULL;
        }
    }
};

EWRAM_DATA Music  music;
EWRAM_DATA Sample channels[SND_CHANNELS];
EWRAM_DATA int32  channelsCount;

#define CALC_INC (((SND_SAMPLE_FREQ << SND_FIXED_SHIFT) / SND_OUTPUT_FREQ) * pitch >> SND_PITCH_SHIFT)

void sndInit()
{
    // initialized in main.cpp
}

void sndInitSamples()
{
    // nothing to do
}

void sndFreeSamples()
{
    // nothing to do
}

void* sndPlaySample(int32 index, int32 volume, int32 pitch, int32 mode)
{
    if (!gSettings.audio_sfx)
        return NULL;

    const uint8 *data = level.soundData + level.soundOffsets[index];
    int32 size = *(int32*)data;
    data += 4;

    if (mode == UNIQUE || mode == REPLAY)
    {
        for (int32 i = 0; i < channelsCount; i++)
        {
            Sample* sample = channels + i;

            if (sample->data != data)
                continue;

            sample->inc = CALC_INC;
            sample->volume = volume;

            if (mode == REPLAY)
            {
                sample->pos = 0;
            }

            return sample;
        }
    }

    if (channelsCount >= SND_CHANNELS)
        return NULL;

    Sample* sample = channels + channelsCount++;
    sample->data = data;
    sample->size = size << SND_FIXED_SHIFT;
    sample->pos  = 0;
    sample->inc  = CALC_INC;
    sample->volume = volume;

    return sample;
}

void sndPlayTrack(int32 track)
{
    if (!gSettings.audio_music)
        return;

    if (track == gCurTrack)
        return;

    gCurTrack = track;

    if (track == -1) {
        sndStopTrack();
        return;
    }

    struct TrackInfo {
        int32 offset;
        int32 size;
    };
    
    const TrackInfo* info = (const TrackInfo*)TRACKS_AD4 + track;

    if (!info->size)
        return;

    // Clear music.data before setup, and write it after to ensure
    // music.fill() has a consistent state at any point in time
    music.data = NULL;
    music.size = info->size;
    music.pos = 0;
    //music.volume = (1 << SND_VOL_SHIFT);
    music.state.zM1   = 0;
    music.state.zM2   = 0;
    music.state.tap   = 0;
    music.state.quant = 0x0800;
    music.data = (const uint8*)TRACKS_AD4 + info->offset;
}

void sndStopTrack()
{
    music.data = NULL;
    music.size = 0;
    music.pos = 0;
}

bool sndTrackIsPlaying()
{
    return music.data != NULL;
}

void sndStopSample(int32 index)
{
    const uint8 *data = level.soundData + level.soundOffsets[index] + 4;

    int32 i = channelsCount;

    while (--i >= 0)
    {
        if (channels[i].data == data)
        {
            channels[i] = channels[--channelsCount];
        }
    }
}

void sndStop()
{
    channelsCount = 0;
    music.data = NULL;
}

void sndFill(uint8* buffer)
{
#ifdef PROFILE_SOUNDTIME
    PROFILE_CLEAR();
    PROFILE(CNT_SOUND);
#endif
    bool mix = (music.data != NULL);

    if (mix) {
        music.fill(buffer);
    } else {
        sndClear(buffer);
    }

    int32 ch = channelsCount;
    while (ch--)
    {
        Sample* sample = channels + ch;

        if (mix)
            sample->mix(buffer);
        else
            sample->fill(buffer);

        if (!sample->data) {
            channels[ch] = channels[--channelsCount];
        }

        mix = true;
    }
}
