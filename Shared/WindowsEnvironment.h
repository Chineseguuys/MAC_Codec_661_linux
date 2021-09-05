#pragma once

/**************************************************************************************************
Windows version
**************************************************************************************************/
#ifndef WINVER
    #define WINVER         0x0501
    #define _WIN32_WINNT   0x0501
    #define _WIN32_WINDOWS 0x0501
    #define _WIN32_IE      0x0501
#endif

/**************************************************************************************************
Unicode
**************************************************************************************************/
#ifndef UNICODE
    #define UNICODE
#endif
#ifndef _UNICODE
    #define _UNICODE
#endif

/**************************************************************************************************
Visual Studio defines
**************************************************************************************************/
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS
