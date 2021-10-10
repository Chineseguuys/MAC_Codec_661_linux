#include "All.h"
#include "APEDecompress.h"
#include "APEInfo.h"
#include "Prepare.h"
#include "UnBitArray.h"
#include "NewPredictor.h"

namespace APE
{

#define DECODE_BLOCK_SIZE        4096

CAPEDecompress::CAPEDecompress(int * pErrorCode, CAPEInfo * pAPEInfo, int64 nStartBlock, int64 nFinishBlock)
{
    *pErrorCode = ERROR_SUCCESS;

    // open / analyze the file
    m_spAPEInfo.Assign(pAPEInfo);

    // version check (this implementation only works with 3.93 and later files)
    if (GetInfo(APE_INFO_FILE_VERSION) < 3930)
    {
        *pErrorCode = ERROR_UNDEFINED;
        return;
    }

    // get format information
    GetInfo(APE_INFO_WAVEFORMATEX, (int64) &m_wfeInput);
    m_nBlockAlign = GetInfo(APE_INFO_BLOCK_ALIGN);

    // initialize other stuff
    m_bDecompressorInitialized = false;
    m_nCurrentFrame = 0;
    m_nCurrentBlock = 0;
    m_nCurrentFrameBufferBlock = 0;
    m_nFrameBufferFinishedBlocks = 0;
    m_bErrorDecodingCurrentFrame = false;
    m_nErrorDecodingCurrentFrameOutputSilenceBlocks = 0;
    m_bLegacyMode = false;
    m_nLastX = 0;
    m_nSpecialCodes = 0;
    m_nCRC = 0;
    m_nStoredCRC = 0;

    // set the "real" start and finish blocks
    m_nStartBlock = (nStartBlock < 0) ? 0 : ape_min(nStartBlock, GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_nFinishBlock = (nFinishBlock < 0) ? GetInfo(APE_INFO_TOTAL_BLOCKS) : ape_min(nFinishBlock, GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_bIsRanged = (m_nStartBlock != 0) || (m_nFinishBlock != GetInfo(APE_INFO_TOTAL_BLOCKS));

    // channel data
    m_paryChannelData = new int64 [32];

    // predictors
    memset(m_aryPredictor, 0, sizeof(m_aryPredictor));
}

CAPEDecompress::~CAPEDecompress()
{
    delete[] m_paryChannelData;
    for (int z = 0; z < 32; z++)
    {
        if (m_aryPredictor[z] != NULL)
            delete m_aryPredictor[z];
    }
}

int CAPEDecompress::InitializeDecompressor()
{
    // check if we have anything to do
    if (m_bDecompressorInitialized)
        return ERROR_SUCCESS;

    // update the initialized flag
    m_bDecompressorInitialized = true;

    // check the block align
    if ((m_nBlockAlign <= 0) || (m_nBlockAlign > 256))
        return ERROR_INVALID_INPUT_FILE;

    // create a frame buffer
    m_cbFrameBuffer.CreateBuffer((GetInfo(APE_INFO_BLOCKS_PER_FRAME) + DECODE_BLOCK_SIZE) * m_nBlockAlign, m_nBlockAlign * 64);
    
    // create decoding components
    m_spUnBitArray.Assign((CUnBitArrayBase *) CreateUnBitArray(this, (int) GetInfo(APE_INFO_FILE_VERSION)));
    if (m_spUnBitArray == NULL)
        return ERROR_UNSUPPORTED_FILE_VERSION;

    // create the predictors
    int64 nChannels = ape_min(ape_max(GetInfo(APE_INFO_CHANNELS), 1), 32);
    if (GetInfo(APE_INFO_FILE_VERSION) >= 3950)
    {
        // 一般情况下，不再支持老版本的 APE 文件系统。处理起来比较的复杂，而且现在多见的几个APE文件的版本在 v3.95 v3.97 v3.98 和 v3.99 
        /**
         * 压缩的过程，针对 L 和 R 的双声道的音频，不会直接对其进行编码，我们一般会先获取 mid sum 和 diff 值来进行处理。
         * X = (L + R) /2 ; Y = (L - R)
         * 
         * 我们依然不会对上面的 X Y 进行直接的编码，因为这些值可能非常的大，编码需要非常多的比特个数。我们希望参与编码的数字是比较小的数。所以，我们需要一个预测器（Predictor）
         * 预测器需要完成的工作就是根据历史时刻的值，来预测当前时刻的值，我们希望预测的结果和真实的值的偏差越小越好。我们记 $X_{diff} = X - P_{X}$ 。那么 $X_{diff}$ 应当是比较小的值
         * 对这个比较小的值进行编码，显然需要的比特的个数非常的少。
         * 
         * 这就是预测器的作用
        */
        for (int64 nChannel = 0; nChannel < nChannels; nChannel++)
            m_aryPredictor[nChannel] = new CPredictorDecompress3950toCurrent((intn) GetInfo(APE_INFO_COMPRESSION_LEVEL), (intn) GetInfo(APE_INFO_FILE_VERSION), (intn) GetInfo(APE_INFO_BITS_PER_SAMPLE));
    }
    else
    {
        for (int64 nChannel = 0; nChannel < nChannels; nChannel++)
            m_aryPredictor[nChannel] = new CPredictorDecompressNormal3930to3950((intn) GetInfo(APE_INFO_COMPRESSION_LEVEL), (intn) GetInfo(APE_INFO_FILE_VERSION));
    }
    
    // seek to the beginning
    return Seek(0);
}

int CAPEDecompress::GetData(char * pBuffer, int64 nBlocks, int64 * pBlocksRetrieved)
{
    int nResult = ERROR_SUCCESS;
    if (pBlocksRetrieved) *pBlocksRetrieved = 0;
    
    // make sure we're initialized
    RETURN_ON_ERROR(InitializeDecompressor())
            
    // cap
    int64 nBlocksUntilFinish = m_nFinishBlock - m_nCurrentBlock;
    const int64 nBlocksToRetrieve = ape_min(nBlocks, nBlocksUntilFinish);
    
    // get the data
    unsigned char * pOutputBuffer = (unsigned char *) pBuffer;
    int64 nBlocksLeft = nBlocksToRetrieve; int64 nBlocksThisPass = 1;
    while ((nBlocksLeft > 0) && (nBlocksThisPass > 0))
    {
        // fill up the frame buffer
        int nDecodeRetVal = FillFrameBuffer();
        if (nDecodeRetVal != ERROR_SUCCESS)
            nResult = nDecodeRetVal;

        // analyze how much to remove from the buffer
        const int64 nFrameBufferBlocks = ape_min(m_nFrameBufferFinishedBlocks, m_cbFrameBuffer.MaxGet() / m_nBlockAlign);
        nBlocksThisPass = ape_min(nBlocksLeft, nFrameBufferBlocks);

        // remove as much as possible
        if (nBlocksThisPass > 0)
        {
            m_cbFrameBuffer.Get(pOutputBuffer, nBlocksThisPass * m_nBlockAlign);
            pOutputBuffer += nBlocksThisPass * m_nBlockAlign;
            nBlocksLeft -= nBlocksThisPass;
            m_nFrameBufferFinishedBlocks -= nBlocksThisPass;
        }
    }

    // calculate the blocks retrieved
    /**
     * 通过当前的 block 减去初始的 block ，得到本次读取数据获得的 block 的数量
    */
    int64 nBlocksRetrieved = int64(nBlocksToRetrieve - nBlocksLeft);

    // update position  记录当前所在的 block 的位置
    m_nCurrentBlock += nBlocksRetrieved;
    if (pBlocksRetrieved) *pBlocksRetrieved = nBlocksRetrieved;

    return nResult;
}

int CAPEDecompress::Seek(int64 nBlockOffset)
{
    // 在第一次调用 Seek() 的时候，需要对解压缩器进行初始化。
    RETURN_ON_ERROR(InitializeDecompressor())

    // use the offset
    nBlockOffset += m_nStartBlock;
    
    // cap (to prevent seeking too far)
    if (nBlockOffset >= m_nFinishBlock)
        nBlockOffset = m_nFinishBlock - 1;
    if (nBlockOffset < m_nStartBlock)
        nBlockOffset = m_nStartBlock;

    // seek to the perfect location
    // 每一个 block 当中包含了所有通道的信息，每个 block 所占用的 Bytes 记录在 m_nBlockAlign 当中
    int64 nBaseFrame = nBlockOffset / GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    // 定位的 block 的位置未必是 frame 的开头部分（大部分的情况下都不是） 
    // 因此，从当前的 frame 开始，到定位位置的 block，需要进行 frame 内部的跳转
    int64 nBlocksToSkip = nBlockOffset % GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    int64 nBytesToSkip = nBlocksToSkip * m_nBlockAlign;
        
    m_nCurrentBlock = nBaseFrame * GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    m_nCurrentFrameBufferBlock = nBaseFrame * GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    m_nCurrentFrame = nBaseFrame;
    m_nFrameBufferFinishedBlocks = 0;
    m_cbFrameBuffer.Empty();
    RETURN_ON_ERROR(SeekToFrame(m_nCurrentFrame));

    // skip necessary blocks
    CSmartPtr<char> spTempBuffer(new char [(size_t) nBytesToSkip], true);
    if (spTempBuffer == NULL) return ERROR_INSUFFICIENT_MEMORY;
    
    int64 nBlocksRetrieved = 0;
    // 如果定位的位置不在 frame 的开始部分，那么定位位置之前需要跳过的所有的数据也需要进行解压缩。
    GetData(spTempBuffer, nBlocksToSkip, &nBlocksRetrieved);
    if (nBlocksRetrieved != nBlocksToSkip)
        return ERROR_UNDEFINED;

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Decodes blocks of data
**************************************************************************************************/
int CAPEDecompress::FillFrameBuffer()
{
    int nResult = ERROR_SUCCESS;

    // determine the maximum blocks we can decode
    // note that we won't do end capping because we can't use data
    // until EndFrame(...) successfully handles the frame
    // that means we may decode a little extra in end capping cases
    // but this allows robust error handling of bad frames

    // loop and decode data
    int64 nBlocksLeft = m_cbFrameBuffer.MaxAdd() / m_nBlockAlign;
    while ((nBlocksLeft > 0) && (nResult == ERROR_SUCCESS))
    {
        // output silence from previous error
        if (m_nErrorDecodingCurrentFrameOutputSilenceBlocks > 0)
        {
            // output silence
            int64 nOutputSilenceBlocks = ape_min(m_nErrorDecodingCurrentFrameOutputSilenceBlocks, nBlocksLeft);
            unsigned char cSilence = (GetInfo(APE_INFO_BITS_PER_SAMPLE) == 8) ? 127 : 0;
            for (int z = 0; z < nOutputSilenceBlocks * m_nBlockAlign; z++)
            {
                *m_cbFrameBuffer.GetDirectWritePointer() = cSilence;
                m_cbFrameBuffer.UpdateAfterDirectWrite(1);
            }

            // decrement
            m_nErrorDecodingCurrentFrameOutputSilenceBlocks -= nOutputSilenceBlocks;
            nBlocksLeft -= nOutputSilenceBlocks;
            m_nFrameBufferFinishedBlocks += nOutputSilenceBlocks;
            m_nCurrentFrameBufferBlock += nOutputSilenceBlocks;
            if (nBlocksLeft <= 0)
                break;
        }

        // get frame size  除了最后一个 frame 之外，其他的所有的 frame 的 block 数都是相同的，因此只需要简单的判断是否是最后 frame 就可以了
        int64 nFrameBlocks = GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame);
        if (nFrameBlocks < 0)
            break;

        // analyze  查看我们要获取的第一个 block 在 frame 当中的偏移位置。并且计算在当前的 frame 当中有多少个需要读取的 block 
        int64 nFrameOffsetBlocks = m_nCurrentFrameBufferBlock % GetInfo(APE_INFO_BLOCKS_PER_FRAME);
        int64 nFrameBlocksLeft = nFrameBlocks - nFrameOffsetBlocks;
        int64 nBlocksThisPass = ape_min(nFrameBlocksLeft, nBlocksLeft); // 当前的 circule buffer 是否可以容纳下当前frame剩余的所有的数据？防止数据溢出

        // start the frame if we need to
        if (nFrameOffsetBlocks == 0)
            StartFrame();

        // decode data
        DecodeBlocksToFrameBuffer(nBlocksThisPass);
            
        // end the frame if we decoded all the blocks from the current frame
        bool bEndedFrame = false;
        if ((nFrameOffsetBlocks + nBlocksThisPass) >= nFrameBlocks)
        {
            EndFrame();
            bEndedFrame = true;
        }

        // handle errors (either mid-frame or from a CRC at the end of the frame)
        if (m_bErrorDecodingCurrentFrame)
        {
            int64 nFrameBlocksDecoded = 0;
            if (bEndedFrame)
            {   
                // remove the frame buffer blocks that have been marked as good
                m_nFrameBufferFinishedBlocks -= GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame - 1);
                
                // assume that the frame buffer contains the correct number of blocks for the entire frame
                nFrameBlocksDecoded = GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame - 1);
            }
            else
            {
                // move to the next frame
                m_nCurrentFrame++;

                // calculate how many blocks were output before we errored
                nFrameBlocksDecoded = m_nCurrentFrameBufferBlock - (GetInfo(APE_INFO_BLOCKS_PER_FRAME) * (m_nCurrentFrame - 1));
            }

            // remove any decoded data for this frame from the buffer
            int64 nFrameBytesDecoded = nFrameBlocksDecoded * m_nBlockAlign;
            m_cbFrameBuffer.RemoveTail(nFrameBytesDecoded);

            // seek to try to synchronize after an error
            m_nCurrentFrame--;
            if (m_nCurrentFrame < GetInfo(APE_INFO_TOTAL_FRAMES))
                SeekToFrame(m_nCurrentFrame);

            // reset our frame buffer position to the beginning of the frame
            m_nCurrentFrameBufferBlock = (m_nCurrentFrame - 1) * GetInfo(APE_INFO_BLOCKS_PER_FRAME);

            if (m_bLegacyMode == false)
            {
                m_bLegacyMode = true;
                for (int z = 0; z < 32; z++)
                {
                    if (m_aryPredictor[z] != NULL)
                        m_aryPredictor[z]->SetLegacyDecode(true);
                }
            }
            else
            {
                // output silence for the duration of the error frame (we can't just dump it to the
                // frame buffer here since the frame buffer may not be large enough to hold the
                // duration of the entire frame)
                m_nErrorDecodingCurrentFrameOutputSilenceBlocks += nFrameBlocks;

                // save the return value
                nResult = ERROR_INVALID_CHECKSUM;
            }
        }

        // update the number of blocks that still fit in the buffer
        nBlocksLeft = m_cbFrameBuffer.MaxAdd() / m_nBlockAlign;
    }

    return nResult;
}

void CAPEDecompress::DecodeBlocksToFrameBuffer(int64 nBlocks)
{
    // decode the samples
    int64 nBlocksProcessed = 0;
    int64 nFrameBufferBytes = m_cbFrameBuffer.MaxGet();

    try
    {
        if (m_wfeInput.nChannels > 2)
        {
            for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
            {
                for (int nChannel = 0; nChannel < m_wfeInput.nChannels; nChannel++)
                {
                    int64 nValue = m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[nChannel]);
                    int64 nValue2 = m_aryPredictor[nChannel]->DecompressValue(nValue, 0);
                    m_paryChannelData[nChannel] = nValue2;
                }
                m_Prepare.Unprepare(m_paryChannelData, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
            }
        }
        else if (m_wfeInput.nChannels == 2)
        {
            /**
             * 这里有三种情况，分别是特殊帧左声道静音    特殊帧右声道静音   特殊帧伪双声道
            */
            if ((m_nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE) && 
                (m_nSpecialCodes & SPECIAL_FRAME_RIGHT_SILENCE)) 
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    /**
                     * 这种情况说明当前的 frame 的左声道和右声道都是静音的，那么当前的帧就不需要进行解压缩了。直接输出 0 就可以了
                    */
                    int64 aryValues[2] = { 0, 0 };
                    m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                    m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                }
            }
            else if (m_nSpecialCodes & SPECIAL_FRAME_PSEUDO_STEREO)
            {
                /**
                 * 所谓的伪双声道就是指在音频采样时是单声道音频。另外一个声道完全复制这个单声道的音频，达到输出双声道的目的。
                 * 实际使用两个收音器录制的左右双声道由于声源的个数，位置，和声源之间距离的差异，左右声道的数据是不可能完全相同的。
                 * 单声道音频扩展成双声道就被称做伪双声道
                 * 
                 * 注意：
                 * 只有单个声道，所以，我们有 L == R ，根据差分计算公式
                 * X = (L+R) / 2 = L = R 
                 * Y = (L -R) = 0 
                */
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int64 aryValues[2] = { m_aryPredictor[0]->DecompressValue(m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[0])), 0 };

                    m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                    m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                }
            }    
            else
            {
                if (m_spAPEInfo->GetInfo(APE_INFO_FILE_VERSION) >= 3950)
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
                        int64 nY = m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[1]);
                        int64 nX = m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[0]);
                        int64 Y = m_aryPredictor[1]->DecompressValue(nY, m_nLastX);
                        int64 X = m_aryPredictor[0]->DecompressValue(nX, Y);
                        m_nLastX = X;

                        int64 aryValues[2] = { X, Y };
                        unsigned char * pOutput = m_cbFrameBuffer.GetDirectWritePointer();
                        m_Prepare.Unprepare(aryValues, &m_wfeInput, pOutput);
                        m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                    }
                }
                else
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
                        int64 X = m_aryPredictor[0]->DecompressValue(m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[0]));
                        int64 Y = m_aryPredictor[1]->DecompressValue(m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[1]));

                        int64 aryValues[2] = { X, Y };
                        m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                        m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                    }
                }
            }
        }
        else if (m_wfeInput.nChannels == 1)
        {
            if (m_nSpecialCodes & SPECIAL_FRAME_MONO_SILENCE)
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int64 aryValues[2] = { 0, 0 };
                    m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                    m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                }
            }
            else
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int64 aryValues[2] = { m_aryPredictor[0]->DecompressValue(m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[0])), 0 };
                    m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                    m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                }
            }
        }
    }
    catch(...)
    {
        m_bErrorDecodingCurrentFrame = true;
    }

    // get actual blocks that have been decoded and added to the frame buffer
    int64 nActualBlocks = (m_cbFrameBuffer.MaxGet() - nFrameBufferBytes) / m_nBlockAlign;
    nActualBlocks = ape_max(nActualBlocks, 0);
    if (nBlocks != nActualBlocks)
        m_bErrorDecodingCurrentFrame = true;

    // update CRC
    m_nCRC = m_cbFrameBuffer.UpdateCRC(m_nCRC, nActualBlocks * m_nBlockAlign);

    // bump frame decode position
    m_nCurrentFrameBufferBlock += nActualBlocks;
}

void CAPEDecompress::StartFrame()
{
    m_nCRC = 0xFFFFFFFF;
    
    // get the frame header //每一个 frame 开始的 32 bit 是 CRC 校验值
    m_nStoredCRC = (unsigned int) m_spUnBitArray->DecodeValue(DECODE_VALUE_METHOD_UNSIGNED_INT);
    m_bErrorDecodingCurrentFrame = false;
    m_nErrorDecodingCurrentFrameOutputSilenceBlocks = 0;
   
    // get any 'special' codes if the file uses them (for silence, false stereo, etc.)
    m_nSpecialCodes = 0;
    if (GET_USES_SPECIAL_FRAMES(m_spAPEInfo))
    {
        if (m_nStoredCRC & 0x80000000) 
        {
            m_nSpecialCodes = (int) m_spUnBitArray->DecodeValue(DECODE_VALUE_METHOD_UNSIGNED_INT);
        }
        m_nStoredCRC &= 0x7FFFFFFF;
    }

    for (int z = 0; z < 32; z++)
    {
        if (m_aryPredictor[z] != NULL)
            m_aryPredictor[z]->Flush();
    }

    for (int z = 0; z < 32; z++)
    {
        m_spUnBitArray->FlushState(m_aryBitArrayStates[z]);
    }
    
    m_spUnBitArray->FlushBitArray();
    m_nLastX = 0;
}

void CAPEDecompress::EndFrame()
{
    m_nFrameBufferFinishedBlocks += GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame);
    m_nCurrentFrame++;

    // finalize
    m_spUnBitArray->Finalize();

    // check the CRC
    m_nCRC = m_nCRC ^ 0xFFFFFFFF;
    m_nCRC >>= 1;
    if (m_nCRC != m_nStoredCRC)
    {
        // error
        m_bErrorDecodingCurrentFrame = true;

        // We didn't use to check the CRC of the last frame in MAC 3.98 and earlier.  This caused some confusion for one
        // user that had a lot of 3.97 Extra High files that have CRC errors on the last frame.  They would verify
        // with old versions, but not with newer versions.  It's still unknown what corrupted the user's files but since
        // only the last frame was bad, it's likely to have been caused by a buggy tagger.
        //if ((m_nCurrentFrame >= GetInfo(APE_INFO_TOTAL_FRAMES)) && (GetInfo(APE_INFO_FILE_VERSION) < 3990))
        //    m_bErrorDecodingCurrentFrame = false;
    }
}

/**************************************************************************************************
Seek to the proper frame (if necessary) and do any alignment of the bit array
**************************************************************************************************/
int CAPEDecompress::SeekToFrame(int64 nFrameIndex)
{
    /**
     * seektable 存储值是 uint32 类型。每一个值代表了相应 frame 在文件当中的偏移位置。通过查表可以快速得到每一frame的位置。
     * 这个位置信息是无法自己计算得到的。因为存储在原始文件中的数据是经过压缩的。每一frame 每一个 block 压缩之后的大小都是未知的
     * 解压缩之后，原始的音频数据是每一个block包含左右声道，16bits采样的 4 字节数据。
    */
    int64 nSeekRemainder = (GetInfo(APE_INFO_SEEK_BYTE, nFrameIndex) - GetInfo(APE_INFO_SEEK_BYTE, 0)) % 4;
    return m_spUnBitArray->FillAndResetBitArray(GetInfo(APE_INFO_SEEK_BYTE, nFrameIndex) - nSeekRemainder, nSeekRemainder * 8);
}

/**************************************************************************************************
Get information from the decompressor
**************************************************************************************************/
int64 CAPEDecompress::GetInfo(APE_DECOMPRESS_FIELDS Field, int64 nParam1, int64 nParam2)
{
    int64 nResult = 0;
    bool bHandled = true;

    switch (Field)
    {
    case APE_DECOMPRESS_CURRENT_BLOCK:
        nResult = int64(m_nCurrentBlock - m_nStartBlock);
        break;
    case APE_DECOMPRESS_CURRENT_MS:
    {
        int64 nSampleRate = m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
            nResult = int64((double(m_nCurrentBlock) * double(1000)) / double(nSampleRate));
        break;
    }
    case APE_DECOMPRESS_TOTAL_BLOCKS:
        nResult = int64(m_nFinishBlock - m_nStartBlock);
        break;
    case APE_DECOMPRESS_LENGTH_MS:
    {
        int64 nSampleRate = (int64) m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
            nResult = int64((double(m_nFinishBlock - m_nStartBlock) * double(1000)) / double(nSampleRate));
        break;
    }
    case APE_DECOMPRESS_CURRENT_BITRATE:
        nResult = GetInfo(APE_INFO_FRAME_BITRATE, m_nCurrentFrame);
        break;
    case APE_DECOMPRESS_CURRENT_FRAME:
        nResult = m_nCurrentFrame;
        break;
    case APE_DECOMPRESS_AVERAGE_BITRATE:
    {
        if (m_bIsRanged)
        {
            // figure the frame range
            const int64 nBlocksPerFrame = GetInfo(APE_INFO_BLOCKS_PER_FRAME);
            int64 nStartFrame = int64(m_nStartBlock / nBlocksPerFrame);
            int64 nFinishFrame = int64(m_nFinishBlock + nBlocksPerFrame - 1) / nBlocksPerFrame;

            // get the number of bytes in the first and last frame
            int64 nTotalBytes = (GetInfo(APE_INFO_FRAME_BYTES, nStartFrame) * (m_nStartBlock % nBlocksPerFrame)) / nBlocksPerFrame;
            if (nFinishFrame != nStartFrame)
                nTotalBytes += (GetInfo(APE_INFO_FRAME_BYTES, nFinishFrame) * (m_nFinishBlock % nBlocksPerFrame)) / nBlocksPerFrame;

            // get the number of bytes in between
            const int64 nTotalFrames = GetInfo(APE_INFO_TOTAL_FRAMES);
            for (int64 nFrame = nStartFrame + 1; (nFrame < nFinishFrame) && (nFrame < nTotalFrames); nFrame++)
                nTotalBytes += GetInfo(APE_INFO_FRAME_BYTES, nFrame);

            // figure the bitrate
            int64 nTotalMS = int64((double(m_nFinishBlock - m_nStartBlock) * double(1000)) / double(GetInfo(APE_INFO_SAMPLE_RATE)));
            if (nTotalMS != 0)
                nResult = int64((nTotalBytes * 8) / nTotalMS);
        }
        else
        {
            nResult = GetInfo(APE_INFO_AVERAGE_BITRATE);
        }

        break;
    }
    case APE_INFO_WAV_HEADER_BYTES:
        if (m_bIsRanged)
            nResult = sizeof(WAVE_HEADER);
        else
            bHandled = false;
        break;
    case APE_INFO_WAV_HEADER_DATA:
        if (m_bIsRanged)
        {
            char * pBuffer = (char *) nParam1;
            int64 nMaxBytes = nParam2;

            if (sizeof(WAVE_HEADER) > nMaxBytes)
            {
                nResult = -1;
            }
            else
            {
                WAVEFORMATEX wfeFormat; GetInfo(APE_INFO_WAVEFORMATEX, (int64) &wfeFormat, 0);
                WAVE_HEADER WAVHeader; FillWaveHeader(&WAVHeader,
                    (m_nFinishBlock - m_nStartBlock) * GetInfo(APE_INFO_BLOCK_ALIGN),
                    &wfeFormat, 0);
                memcpy(pBuffer, &WAVHeader, sizeof(WAVE_HEADER));
                nResult = 0;
            }
        }
        else
            bHandled = false;
        break;
    case APE_INFO_WAV_TERMINATING_BYTES:
        if (m_bIsRanged)
            nResult = 0;
        else
            bHandled = false;
        break;
    case APE_INFO_WAV_TERMINATING_DATA:
        if (m_bIsRanged)
            nResult = 0;
        else
            bHandled = false;
        break;
    default:
        bHandled = false;
    }

    if (!bHandled)
        nResult = m_spAPEInfo->GetInfo(Field, nParam1, nParam2);

    return nResult;
}

}