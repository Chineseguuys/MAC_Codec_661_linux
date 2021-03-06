/**************************************************************************************************
APEInfo.h
Copyright (C) 2000-2021 by Matthew T. Ashland   All Rights Reserved.

Simple method for working with APE files... it encapsulates reading, writing and getting
file information.  Just create a CAPEInfo class, call OpenFile(), and use the class methods
to do whatever you need... the destructor will take care of any cleanup

Notes:
    -Most all functions return 0 upon success, and some error code (other than 0) on
    failure.  However, all of the file functions that are wrapped from the Win32 API
    return 0 on failure and some other number on success.  This applies to ReadFile, 
    WriteFile, SetFilePointer, etc...
**************************************************************************************************/

#pragma once

#include "IO.h"
#include "APETag.h"
#include "MACLib.h"

namespace APE
{

/**************************************************************************************************
APE_FILE_INFO - structure which describes most aspects of an APE file 
(used internally for speed and ease)
**************************************************************************************************/
struct APE_FILE_INFO
{
    int nVersion;                                   // file version number * 1000 (3.93 = 3930)
    int nCompressionLevel;                          // the compression level
    int nFormatFlags;                               // format flags
    uint32 nTotalFrames;                            // the total number frames (frames are used internally)
    uint32 nBlocksPerFrame;                         // the samples in a frame (frames are used internally)
    uint32 nFinalFrameBlocks;                       // the number of samples in the final frame
    int nChannels;                                  // audio channels
    int nSampleRate;                                // audio samples per second
    int nBitsPerSample;                             // audio bits per sample
    int nBytesPerSample;                            // audio bytes per sample
    int nBlockAlign;                                // audio block align (channels * bytes per sample)
    int64 nWAVHeaderBytes;                          // header bytes of the original WAV
    int64 nWAVDataBytes;                            // data bytes of the original WAV
    uint32 nWAVTerminatingBytes;                    // terminating bytes of the original WAV
    int64 nWAVTotalBytes;                           // total bytes of the original WAV
    uint32 nAPETotalBytes;                          // total bytes of the APE file
    int nTotalBlocks;                               // the total number audio blocks
    int nLengthMS;                                  // the length in milliseconds
    int nAverageBitrate;                            // the kbps (i.e. 637 kpbs)
    int nDecompressedBitrate;                       // the kbps of the decompressed audio (i.e. 1440 kpbs for CD audio)
    int nJunkHeaderBytes;                           // used for ID3v2, etc.
    int nSeekTableElements;                         // the number of elements in the seek table(s)
    int nMD5Invalid;                                // whether the MD5 is valid
    
    CSmartPtr<uint32> spSeekByteTable;              // the seek table (byte)
    CSmartPtr<unsigned char> spSeekBitTable;        // the seek table (bits -- legacy)
    CSmartPtr<unsigned char> spWaveHeaderData;      // the pre-audio header data
    CSmartPtr<APE_DESCRIPTOR> spAPEDescriptor;      // the descriptor (only with newer files)
};

/**************************************************************************************************
Helper macros (sort of hacky)
**************************************************************************************************/
#define GET_USES_CRC(APE_INFO) (((APE_INFO)->GetInfo(APE_INFO_FORMAT_FLAGS) & MAC_FORMAT_FLAG_CRC) ? true : false)
#define GET_FRAMES_START_ON_BYTES_BOUNDARIES(APE_INFO) (((APE_INFO)->GetInfo(APE_INFO_FILE_VERSION) > 3800) ? true : false)
#define GET_USES_SPECIAL_FRAMES(APE_INFO) (((APE_INFO)->GetInfo(APE_INFO_FILE_VERSION) > 3820) ? true : false)
#define GET_IO(APE_INFO) ((CIO *) (APE_INFO)->GetInfo(APE_INFO_IO_SOURCE))
#define GET_TAG(APE_INFO) ((CAPETag *) (APE_INFO)->GetInfo(APE_INFO_TAG))

/**************************************************************************************************
CAPEInfo - use this for all work with APE files
**************************************************************************************************/
class CAPEInfo
{
public:
    
    // construction and destruction
    CAPEInfo(int * pErrorCode, const wchar_t * pFilename, CAPETag * pTag = NULL, bool bAPL = false, bool bReadOnly = false);
    CAPEInfo(int * pErrorCode, APE::CIO * pIO, CAPETag * pTag = NULL);
    virtual ~CAPEInfo();

    // query for information
    int64 GetInfo(APE_DECOMPRESS_FIELDS Field, int64 nParam1 = 0, int64 nParam2 = 0);
    
private:
    // internal functions
    int GetFileInformation(bool bGetTagInformation = true);
    int CloseFile();
    int CheckHeaderInformation();
    
    // internal variables
    bool m_bHasFileInformationLoaded;
    CSmartPtr<APE::CIO> m_spIO;
    CSmartPtr<CAPETag> m_spAPETag;
    APE_FILE_INFO m_APEFileInfo;
    bool m_bAPL;
};

}
