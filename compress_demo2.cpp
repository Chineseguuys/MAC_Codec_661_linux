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
#include <stdio.h>
#include <stdlib.h>
#include "All.h"
#include "MACLib.h"
#include "logs.h"
using namespace APE;

#define WAVFORMATEX_USELESS_FLAG 0

/***************************************************************************************
Main (the main function)
***************************************************************************************/
int main(int argc, char *argv[]) {
    /**
     * @brief 定义原始音频的比特数。以及输出文件的名称
     *
     */
    const char   *input_pcm = argv[1]; // 输入的文件名称
    const wchar_t cOutputFile[MAX_PATH] = _T("output_ape.ape");
#ifdef LOG_TEST
    log_info("Creating file: %ls", cOutputFile);
#endif /* LOG_TEST */

    /* wav 格式 */
    APE::WAVEFORMATEX wfeAudioFormat;

    FillWaveFormatEx(&wfeAudioFormat, WAVFORMATEX_USELESS_FLAG, 44100, 16, 2);

    // create the encoder interface
    IAPECompress *pAPECompress = CreateIAPECompress();

    // start the encoder
    // 从最简单的 FAST 压缩等级开始
    int nRetVal = pAPECompress->Start(cOutputFile, &wfeAudioFormat, MAX_AUDIO_BYTES_UNKNOWN,
                                      MAC_COMPRESSION_LEVEL_INSANE, NULL, CREATE_WAV_HEADER_ON_DECOMPRESSION);

    if (nRetVal != 0) {
        SAFE_DELETE(pAPECompress);
#ifdef LOG_TEST
        log_error("Create APECompress error!");
#endif /* LOG_TEST */
        return -1;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // 从 pcm 文件当中读取数据进行编码
    ///////////////////////////////////////////////////////////////////////////////

    FILE *input_file_desc = fopen(input_pcm, "rb");
    if (input_file_desc == NULL) {
        log_error("open source file error");
        SAFE_DELETE(pAPECompress);
        return 1;
    }

    size_t read_bytes = 0;
    while (true) {
        int64          nBufferBytesAvailable = 0;
        unsigned char *buffer = pAPECompress->LockBuffer(&nBufferBytesAvailable);
        read_bytes = fread(buffer, 1, nBufferBytesAvailable, input_file_desc);
        if (read_bytes <= 0)
            break;
        APE::int64 nRetVal = pAPECompress->UnlockBuffer(read_bytes, TRUE);
        if (nRetVal != 0) {
            log_error("Error Encoding frame (error:%ld)", nRetVal);
            break;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // finalize the file (could append a tag, or WAV terminating data)
    ///////////////////////////////////////////////////////////////////////////////
    if (pAPECompress->Finish(NULL, 0, 0) != 0) {
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