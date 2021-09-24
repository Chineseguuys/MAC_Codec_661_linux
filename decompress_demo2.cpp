#include <stdio.h>
#include <time.h>
#include "All.h"
#include "MACLib.h"
#include "CharacterHelper.h"
#include "StdLibFileIO.h"
#include "APEInfo.h"

// for file operator
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAX_BLOCK_SIZE 8192
#define MAX_FRAME_SIZE (MAX_BLOCK_SIZE * 10)
#define MAX_HEADER_SIZE 1024

// 提取 APE 文件中原始的 PCM 内容

int main(int argc, char* argv[]) {
	///////////////////////////////////////////////////////////////////////////////
	// error check the command line parameters
	///////////////////////////////////////////////////////////////////////////////
	if (argc != 2) 
	{
		printf("~~~Improper Usage~~~\n\n");
		printf("Usage Example: Sample 1.exe \"c:\\1.ape\"\n\n");
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// variable declares
	///////////////////////////////////////////////////////////////////////////////
	int nPercentageDone = 0;		//the percentage done... continually updated by the decoder
	int nKillFlag = 0;				//the kill flag for controlling the decoder
	int nRetVal = 0;				//generic holder for return values
	char * pFilename = argv[1];		//the file to open

    APE::str_utfn* sFilename = APE::CAPECharacterHelper::GetUTF16FromANSI(pFilename);   // 需要手动进行析构
    APE::CSmartPtr<APE::CStdLibFileIO> mIO(new APE::CStdLibFileIO(), false, true);
    mIO->Open(sFilename, true);

    APE::CSmartPtr<APE::CAPEInfo> pInfo(new APE::CAPEInfo(&nRetVal, mIO.GetPtr(), NULL), false, true);
    APE::CSmartPtr<char> buffer(new char[MAX_FRAME_SIZE], true, true);
    APE::CSmartPtr<char> wavheaderBuffer(new char[MAX_HEADER_SIZE], true, true);

    int nVersion  = pInfo->GetInfo(APE::APE_INFO_FILE_VERSION);
    printf("file version is %d\n", nVersion);

	APE::CSmartPtr<APE::IAPEDecompress> pDec(CreateIAPEDecompressEx(mIO.GetPtr(), &nRetVal, true), false, true);
	APE::CAPETag pTag(mIO.GetPtr());

    int64_t nBlocksPerFrame = pDec->GetInfo(APE::APE_INFO_BLOCKS_PER_FRAME);
    int nBytesPerSample = pDec->GetInfo(APE::APE_INFO_BYTES_PER_SAMPLE);
    int nChannels = pDec->GetInfo(APE::APE_INFO_CHANNELS);

    int fd_out_pcm = open("./test.pcm", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
    int fd_out_wav = open("./test.wav", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);

    nRetVal =  pInfo->GetInfo(APE::APE_INFO_WAV_HEADER_DATA, static_cast<int64_t>((intptr_t) wavheaderBuffer.GetPtr() ), MAX_HEADER_SIZE);
    if (nRetVal == 0) {
        printf("Successfully get WAV Header\n");
    }

    // 把 WAV 文件的头写入文件
    write(fd_out_wav, wavheaderBuffer.GetPtr(), pInfo->GetInfo(APE::APE_INFO_WAV_HEADER_BYTES));

    pDec->Seek(0);
    int64_t seekPos = 0;
    int64_t one_size = 0;
    int64_t count = 1;
    while (true) {
        if(pDec->GetData(buffer.GetPtr(), MAX_BLOCK_SIZE, &one_size) != ERROR_SUCCESS || one_size == 0) {
            printf("ERROR IN READING STREAM OR REACH THE END\n");
            break;
        }
        /**
         * 音频有两个通道，每一个采样数据使用 16bits，也就是 2Bytes 来进行编码
        */
        write(fd_out_pcm, buffer.GetPtr(), one_size * nBytesPerSample * nChannels);
        write(fd_out_wav, buffer.GetPtr(), one_size * nBytesPerSample * nChannels);
        seekPos += one_size;
        //printf("for %4ld, catch %4ld blocks --> %ld All\n", count, one_size, seekPos);
        ++ count;
    }

    close(fd_out_pcm);
    close(fd_out_wav);
	delete[] sFilename;
    return 0;
}

/**
 * cmake -S . -B ./build/ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
 * make -C ./build/
 * ./build/test ./ape_resources/Bichang_Zhou.ape
 * 
 * 可以使用 ffplay 来播放 test.wav 
 * 注意，pcm 只是一个后缀，这个文件是一个原始音频的 PCM 文件。并不是一个标准的 WAV 文件。在这个文件的前面加上一个 WAV 文件的头，那么他就成了 WAV 文件
 * ffplay -ar 44100  -ac 2 -f s16le -i test.pcm
 * 
 * test.wav 文件可以使用任意的音乐播放其进行播放 
 * 
 * 详细的使用方法可以使用 ffplay -h 查看
*/

/**
 * Bichang_Zhou.ape 的 APEInfo 信息
 * m_APEFileInfo = {
    nVersion = 3990
    nCompressionLevel = 2000
    nFormatFlags = 0
    nTotalFrames = 158
    nBlocksPerFrame = 73728
    nFinalFrameBlocks = 23456
    nChannels = 2
    nSampleRate = 44100
    nBitsPerSample = 16
    nBytesPerSample = 2
    nBlockAlign = 4
    nWAVHeaderBytes = 44
    nWAVDataBytes = 46395008
    nWAVTerminatingBytes = 0
    nWAVTotalBytes = 46395052
    nAPETotalBytes = 28779936
    nTotalBlocks = 11598752
    nLengthMS = 263010
    nAverageBitrate = 875
    nDecompressedBitrate = 1411
    nJunkHeaderBytes = 0
    nSeekTableElements = 158
    nMD5Invalid = 0
    spSeekByteTable = {
      m_pObject = 0x000000000020b6b0
      m_bArray = true
      m_bDelete = true
    }
    spSeekBitTable = (m_pObject = 0x0000000000000000, m_bArray = false, m_bDelete = true)
    spWaveHeaderData = (m_pObject = "RIFF\xa4\xee\xc3\U00000002WAVEfmt \U00000010", m_bArray = true, m_bDelete = true)
    spAPEDescriptor = {
      m_pObject = 0x000000000020b670
      m_bArray = false
      m_bDelete = true
    }
 */