/***************************************************************************************
Compress - Sample 1
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.
Feel free to use this code in any way that you like.

This example illustrates using MACLib.lib to create an APE file by encoding some
random data.

The IAPECompress interface fully supports on-the-fly encoding.  To use on-
the-fly encoding, be sure to tell the encoder to create the proper WAV header on
decompression.  Also, you need to specify the absolute maximum audio bytes that will
get encoded. (trying to encode more than the limit set on Start() will cause failure)
This maximum is used to allocates space in the seek table at the front of the file.  Currently,
it takes around 8k per hour of CD music, so it isn't a big deal to allocate more than
needed.  You can also specify MAX_AUDIO_BYTES_UNKNOWN to allocate as much space as possible. (2 GB)

Notes for use in a new project:
	-you need to include "MACLib.lib" in the included libraries list
	-life will be easier if you set the [MAC SDK]\\Shared directory as an include 
	directory and an additional library input path in the project settings
	-set the runtime library to "Mutlithreaded"

WARNING:
	-This class driven system for using Monkey's Audio is still in development, so
	I can't make any guarantees that the classes and libraries won't change before
	everything gets finalized.  Use them at your own risk
***************************************************************************************/

// includes
#include "stdio.h"
#include "All.h"
#include "MACLib.h"
#include "logs.h"
using namespace APE;

#define WAVFORMATEX_USELESS_FLAG    0

/***************************************************************************************
Main (the main function)
***************************************************************************************/
int main(int argc, char* argv[]) 
{
	/**
	 * @brief 定义原始音频的比特数。以及输出文件的名称
	 * 
	 */
	int nAudioBytes = 1048576*10;
	const wchar_t cOutputFile[MAX_PATH] = _T("./Noise.ape");
#ifdef LOG_TEST
    log_info("Creating file: %ls", cOutputFile);
#endif /* LOG_TEST */

        /* wav 格式 */
	APE::WAVEFORMATEX wfeAudioFormat; 
    
    FillWaveFormatEx(&wfeAudioFormat, WAVFORMATEX_USELESS_FLAG, 44100, 16, 2);
	
	// create the encoder interface
	IAPECompress * pAPECompress = CreateIAPECompress();
	
	// start the encoder
    // 从最简单的 FAST 压缩等级开始
	int nRetVal = pAPECompress->Start(cOutputFile, &wfeAudioFormat, nAudioBytes, 
		MAC_COMPRESSION_LEVEL_FAST, NULL, CREATE_WAV_HEADER_ON_DECOMPRESSION);

	if (nRetVal != 0)
	{
		SAFE_DELETE(pAPECompress);
#ifdef LOG_TEST
        log_error("Create APECompress error!");
#endif /* LOG_TEST */
        return -1;
    }
	
	///////////////////////////////////////////////////////////////////////////////
	// pump through and feed the encoder audio data (white noise for the sample)
	///////////////////////////////////////////////////////////////////////////////
	int64 nAudioBytesLeft = nAudioBytes;
	
	while (nAudioBytesLeft > 0)
	{
		///////////////////////////////////////////////////////////////////////////////
		// NOTE: we're locking the buffer used internally by MAC and copying the data
		//		 directly into it... however, you could also use the AddData(...) command
		//       to avoid the added complexity of locking and unlocking 
		//       the buffer (but it may be a little slower )
		///////////////////////////////////////////////////////////////////////////////
	
		// lock the compression buffer
		int64 nBufferBytesAvailable = 0;
		unsigned char * pBuffer = pAPECompress->LockBuffer(&nBufferBytesAvailable);
	
		// fill the buffer with white noise
		int64 nNoiseBytes = ape_min(nBufferBytesAvailable, nAudioBytesLeft);
		for (int z = 0; z < nNoiseBytes; z++)
		{
			pBuffer[z] = rand() % 255;
		}

		// unlock the buffer and let it get processed
		APE::int64 nRetVal = pAPECompress->UnlockBuffer(nNoiseBytes, TRUE);
		if (nRetVal != 0)
		{
#ifdef LOG_TEST
            log_error("Error Encoding Frame (error:%ld)", nRetVal);
#else /* LOG_TEST */
            printf("Error Encoding Frame (error: %ld)\n", nRetVal);
#endif /* LOG_TEST */
            break;
		}

		// update the audio bytes left
		nAudioBytesLeft -= nNoiseBytes;
	}

	///////////////////////////////////////////////////////////////////////////////
	// finalize the file (could append a tag, or WAV terminating data)
	///////////////////////////////////////////////////////////////////////////////
	if (pAPECompress->Finish(NULL, 0, 0) != 0)
	{
#ifdef LOG_TEST
        log_error("Error finishing encoder.");
#endif /* LOG_TEST */
    }

	///////////////////////////////////////////////////////////////////////////////
	// clean up and quit
	///////////////////////////////////////////////////////////////////////////////
	SAFE_DELETE(pAPECompress)
#ifdef LOG_TEST
    log_info("Done.");
#endif /* LOG_TEST */
    return 0;
}