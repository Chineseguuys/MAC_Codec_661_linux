#ifndef _WAV_HEADER_
#define _WAV_HEADER_

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int

typedef struct
{
    u32 dwSize;
    u16 wFormatTag;
    u16 wChannels;
    u32 dwSamplesPerSec;
    u32 dwAvgBytesPerSec;
    u16 wBitsPerSample;
} WAVFORMAT;

typedef struct
{
    u8  RiffID[4];
    u32 RiffSize;
    u8  WaveID[4];
    u8  FmtID[4];
    u32 FmtSize;
    u16 wFormatTag;
    u16 nChannels;
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    u16 nBlockAlien;
    u16 wBitsPerSample;
    u8  DataID[4];
    u32 nDataBytes;
} WAV_HEADER;

#endif /* _WAV_HEADER_ */