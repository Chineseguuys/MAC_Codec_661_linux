#include "All.h"
#include "UnBitArrayBase.h"
#include "APEInfo.h"
#include "UnBitArray.h"
#ifdef APE_BACKWARDS_COMPATIBILITY
    #include "Old/APEDecompressOld.h"
    #include "Old/UnBitArrayOld.h"
#endif

namespace APE
{

const uint32 POWERS_OF_TWO_MINUS_ONE[33] = {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535,131071,262143,524287,1048575,2097151,4194303,8388607,16777215,33554431,67108863,134217727,268435455,536870911,1073741823,2147483647,4294967295U};

CUnBitArrayBase * CreateUnBitArray(IAPEDecompress * pAPEDecompress, intn nVersion)
{
    // determine the furthest position we should read in the I/O object
    int64 nFurthestReadByte = GET_IO(pAPEDecompress)->GetSize();
    if (nFurthestReadByte > 0)
    {
       // terminating data
       nFurthestReadByte -= pAPEDecompress->GetInfo(APE_INFO_WAV_TERMINATING_BYTES);

       // tag (not worth analyzing the tag since we could be a remote file, etc.)
       // also don't not read the tag if we're working on an APL file
       if (pAPEDecompress->GetInfo(APE_INFO_APL) == 0)
       {
           CAPETag* pAPETag = (CAPETag*)pAPEDecompress->GetInfo(APE_INFO_TAG);
           if ((pAPETag != NULL) && pAPETag->GetAnalyzed())
               nFurthestReadByte -= pAPETag->GetTagBytes();
       }
    }

#ifdef APE_BACKWARDS_COMPATIBILITY
    if (nVersion >= 3900)
        return (CUnBitArrayBase * ) new CUnBitArray(GET_IO(pAPEDecompress), nVersion, nFurthestReadByte);
    else
        return (CUnBitArrayBase * ) new CUnBitArrayOld(pAPEDecompress, nVersion, nFurthestReadByte);
#else
    // create the appropriate object
    if (nVersion < 3900)
    {
        // we should no longer be trying to decode files this old (this check should be redundant since 
        // the main gate keeper should be CreateIAPEDecompressCore(...)
        ASSERT(false);
        return NULL;
    }

    return (CUnBitArrayBase * ) new CUnBitArray(GET_IO(pAPEDecompress), nVersion, nFurthestReadByte);
#endif
}

CUnBitArrayBase::CUnBitArrayBase(int64 nFurthestReadByte)
{
    m_nFurthestReadByte = nFurthestReadByte;
}

CUnBitArrayBase::~CUnBitArrayBase()
{
}

void CUnBitArrayBase::AdvanceToByteBoundary() 
{
    int nMod = m_nCurrentBitIndex % 8;
    if (nMod != 0) { m_nCurrentBitIndex += 8 - nMod; }
}

bool CUnBitArrayBase::EnsureBitsAvailable(uint32 nBits, bool bThrowExceptionOnFailure)
{
    bool bResult = true;

    // get more data if necessary
    /**
     * 这是一种异常的情况，有可能是因为没有将源文件的数据读入到 BitArray buffer 当中。
     * 先尝试去读取数据。如果读取数据之后异常的情况依然的存在，那么就必须抛出异常了。
     * 因此下面进行了两次条件的判断。
    */
    if ((int64(m_nCurrentBitIndex) + int64(nBits)) >= (int64(m_nGoodBytes) * 8))
    {
        // fill
        FillBitArray();

        // if we still don't have enough good bytes, we don't have the bits available
        /**
         * 因为解码的过程以 nBits 为单位进行译码。如果当前的 BitArray Buffer 当中可用的 bit 数量小于 nBits 
         * 那么就是出错了
        */
        if ((int64(m_nCurrentBitIndex) + int64(nBits)) >= (int64(m_nGoodBytes) * 8))
        {
            // overread error
            ASSERT(false);

            // throw exception if specified
            if (bThrowExceptionOnFailure)
                throw(1);

            // data not available
            bResult = false;
        }
    }

    return bResult;
}

uint32 CUnBitArrayBase::DecodeValueXBits(uint32 nBits) 
{
    // get more data if necessary
    EnsureBitsAvailable(nBits, true);

    // variable declares
    uint32 nLeftBits = 32 - (m_nCurrentBitIndex & 31);
    uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;    // (BitArray 的类型是 uint32 所以数组的下标是 BitIndex / 32)
    m_nCurrentBitIndex += nBits;
    
    // if their isn't an overflow to the right value, get the value and exit
    if (nLeftBits >= nBits)
        return (m_pBitArray[nBitArrayIndex] & (POWERS_OF_TWO_MINUS_ONE[nLeftBits])) >> (nLeftBits - nBits);
    
    // must get the "split" value from left and right
    int nRightBits = nBits - nLeftBits;
    
    uint32 nLeftValue = ((m_pBitArray[nBitArrayIndex] & POWERS_OF_TWO_MINUS_ONE[nLeftBits]) << nRightBits);
    uint32 nRightValue = (m_pBitArray[nBitArrayIndex + 1] >> (32 - nRightBits));
    return (nLeftValue | nRightValue);
}

int CUnBitArrayBase::FillAndResetBitArray(int64 nFileLocation, int64 nNewBitIndex)
{
    if (nNewBitIndex < 0) return ERROR_INVALID_INPUT_FILE;

    // seek if necessary
    // 直接定位到相应的 frame 的位置
    if (nFileLocation != -1)
    {
        m_pIO->SetSeekMethod(APE_FILE_BEGIN);
        m_pIO->SetSeekPosition(nFileLocation);
        if (m_pIO->PerformSeek() != 0)
            return ERROR_IO_READ;
    }

    // fill
    // 为什么初始化的位置放在末尾 因为这个函数的关键在于 Reset。Reset 意味着，buffer当中以前存储的数据全部要清除
    // 自然 m_nCurrentBitIndex 要指向 buffer 的最末尾的位置，表示 buffer 当前没有可以读取的数据
    /**
     * 注意 FillAndResetBitArray() 函数和 FillBitArray() 的区别
    */
    m_nCurrentBitIndex = m_nBits; // position at the end of the buffer
    int nResult = FillBitArray();

    // set bit index
    m_nCurrentBitIndex = (uint32) nNewBitIndex;

    return nResult;
}

int CUnBitArrayBase::FillBitArray() 
{
    // get the bit array index
    /**
     * m_pBitArray 数组存储的数据类型是 uint32 。所以 Bit Index 是 BitArrayIndex 的 32 倍
     * uint32 类型长度为 32 bits
    */
    uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;
    
    // move the remaining data to the front 
    // 从 m_nCurrentBitIndex 向后面的位置的数据都是目前还没有读取的数据。这些数据应当先被读取，所以需要移动到 buffer 的最前面。
    int nBytesToMove = m_nBytes - (nBitArrayIndex * 4);
    if (nBytesToMove > 0)
        /**
         * 注意 m_pBitArray 的数据类型是 uint32* 类型。在移动指针的时候，移动长度为 4 字节
        */
        memmove((void *) (m_pBitArray), (const void *) (m_pBitArray + nBitArrayIndex), nBytesToMove);

    // get the number of bytes to read   
    // 每一次读，都将整个 buffer 读满？ 尽可能的填满，但是也有可能填不满
    int64 nBytesToRead = nBitArrayIndex * 4;
    if (m_nFurthestReadByte > 0)
    {   /**
         nFurthestReadBytes 存储的是文件还剩余的可以读取的 Bytes 的个数。所以初始的大小应当是文件的大小
        */
        int64 nFurthestReadBytes = m_nFurthestReadByte - m_pIO->GetPosition();
        if (nBytesToRead > nFurthestReadBytes)  // 如果文件剩余可读取的部分小于 buffer 当中可以填充的部分，需要进行处理。否则文件 IO 会出错
        {
            nBytesToRead = nFurthestReadBytes;
            if (nBytesToRead < 0)
                nBytesToRead = 0;
        }
    }

    // read the new data
    unsigned int nBytesRead = 0;
    int nResult = m_pIO->Read((unsigned char *) (m_pBitArray + m_nElements - nBitArrayIndex), (unsigned int) nBytesToRead, &nBytesRead);

    // zero anything at the tail we didn't fill
    m_nGoodBytes = ((m_nElements - nBitArrayIndex) * 4) + nBytesRead;
    /**
     * 读取到的数据未必可以将 buffer 填满。 当前的 buffer 当中
     * ((m_nElements - nBitArrayIndex) * 4) = m_nBytes - (nBitArrayIndex * 4) 表示本次读取之前，buffer当中已经存储的数据
     * nBytesRead 是本次读取的数据量
     * 那么 m_nGoodBytes 表示在本次读取结束之后，整个 buffer 当中剩余的有效的数据。如果当前的 buffer 没有被填满的话(m_nGoodBytes < m_nBytes)
     * 那么末尾空闲的空间全部置0
    */
    if (m_nGoodBytes < m_nBytes)
        memset(&((unsigned char *) m_pBitArray)[m_nGoodBytes], 0, m_nBytes - m_nGoodBytes);

    // adjust the m_Bit pointer
    // 
    m_nCurrentBitIndex = m_nCurrentBitIndex & 31;
    
    // return
    return (nResult == 0) ? 0 : ERROR_IO_READ;
}

int CUnBitArrayBase::CreateHelper(CIO * pIO, intn nBytes, intn nVersion)
{
    // check the parameters
    if ((pIO == NULL) || (nBytes <= 0)) { return ERROR_BAD_PARAMETER; }

    // save the size
    m_nElements = uint32(nBytes) / 4;
    m_nBytes = m_nElements * 4;
    m_nBits = m_nBytes * 8;
    m_nGoodBytes = 0;
    
    // set the variables
    m_pIO = pIO;
    m_nVersion = nVersion;
    m_nCurrentBitIndex = 0;
    
    // create the bitarray (we allocate and empty a little extra as buffer insurance, although it should never be necessary)
    m_pBitArray = new uint32 [m_nElements + 64];
    memset(m_pBitArray, 0, (m_nElements + 64) * sizeof(uint32));
    
    return (m_pBitArray != NULL) ? 0 : ERROR_INSUFFICIENT_MEMORY;
}

}