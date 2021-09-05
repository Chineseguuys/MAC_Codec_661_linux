#include <stdio.h>
#include <time.h>
#include "All.h"
#include "MACLib.h"


clock_t        g_nInitialTickCount;

void __stdcall ProgressCallback(int nPercentageDone) {
    double dProgress = double(nPercentageDone) / 1000;
    double dElapsedMS = static_cast<double> (clock() - g_nInitialTickCount);

    double dSecondsRemaining = (((dElapsedMS * 100) / dProgress) - dElapsedMS) / 1000;
	printf("Progress: %.1f%% (%.1f seconds remaining)          \r", dProgress, dSecondsRemaining);	
}

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

	///////////////////////////////////////////////////////////////////////////////
	// attempt to verify the file
	///////////////////////////////////////////////////////////////////////////////

	// set the start time and display the starting message
	g_nInitialTickCount = clock();
	printf("Verifying '%s'...\n", pFilename);

	// do the verify (call unmac.dll)
	nRetVal = VerifyFile(pFilename, &nPercentageDone, ProgressCallback, &nKillFlag);

	// process the return value
	if (nRetVal == 0) 
		printf("\nPassed...\n");
	else 
		printf("\nFailed (error: %d)\n", nRetVal);

	///////////////////////////////////////////////////////////////////////////////
	// quit
	///////////////////////////////////////////////////////////////////////////////
	return 0;
}