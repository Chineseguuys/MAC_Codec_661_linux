#include <stdio.h>
#include <time.h>
#include "All.h"
#include "MACLib.h"
#include "CharacterHelper.h"
#include "StdLibFileIO.h"
#include "APEInfo.h"

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

    APE::str_utfn* sFilename = APE::CAPECharacterHelper::GetUTF16FromANSI(pFilename);
    APE::CSmartPtr<APE::CStdLibFileIO> mIO(new APE::CStdLibFileIO(), false, true);
    mIO->Open(sFilename, true);

    APE::CSmartPtr<APE::CAPEInfo> pInfo(new APE::CAPEInfo(&nRetVal, mIO.GetPtr(), NULL), false, true);

    int nVersion  = pInfo->GetInfo(APE::APE_INFO_FILE_VERSION);
    printf("file version is %d\n", nVersion);

	APE::CSmartPtr<APE::IAPEDecompress> pDec(CreateIAPEDecompressEx(mIO.GetPtr(), &nRetVal, true), false, true);
	APE::CAPETag pTag(mIO.GetPtr());
	
	bool hasID3Tags = pTag.GetHasAPETag();
	bool hasAPETags = pTag.GetHasAPETag();

	if (hasID3Tags || hasAPETags) {
		printf("Music %s has tags\n", pFilename);

		char pBuffer[256];
		int bufferMaxSize = 255;
		pTag.GetFieldBinary(APE_TAG_FIELD_TITLE, pBuffer, &bufferMaxSize);
		printf("Music %s's title is %s\n", pFilename, pBuffer);
		bufferMaxSize = 255;
		pTag.GetFieldBinary(APE_TAG_FIELD_ARTIST, pBuffer, &bufferMaxSize);
		printf("Music %s's Artist is %s\n", pFilename, pBuffer);
		bufferMaxSize = 255;
		pTag.GetFieldBinary(APE_TAG_FIELD_COMMENT, pBuffer, &bufferMaxSize);
		printf("Music %s's Commits is %s\n", pFilename, pBuffer);
		bufferMaxSize = 255;
		pTag.GetFieldBinary(APE_TAG_FIELD_GENRE, pBuffer, &bufferMaxSize);
		printf("Music %s's Genre is %s\n", pFilename, pBuffer);
	}

	///////////////////////////////////////////////////////////////////////////////
	// attempt to verify the file
	///////////////////////////////////////////////////////////////////////////////
    return 0;
}