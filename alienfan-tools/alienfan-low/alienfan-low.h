#pragma once
#include <ctype.h>
#include <tchar.h>
#include <windows.h>
#include <Setupapi.h>
#include <devioctl.h>
#include <tchar.h>
#include <strsafe.h>
#include <acpiioct.h>

#pragma warning(disable:4996)

#define DRIVER_NAME _T("HwAcc")

#ifdef __cplusplus
extern "C" {
#endif

    BOOLEAN
        GetServiceName(
            __inout_bcount_full(BufferLength) TCHAR *DriverLocation,
            __in ULONG BufferLength
        );

    BOOLEAN
        InstallService(
            __in SC_HANDLE SchSCManager,
            __in LPTSTR    ServiceExe
        );

    BOOLEAN
        RemoveService(
            __in SC_HANDLE SchSCManager
        );

    BOOLEAN
        DemandService(
            __in SC_HANDLE SchSCManager
        );

    BOOLEAN
        StopService(
            __in SC_HANDLE SchSCManager
        );

    HANDLE
        APIENTRY
        OpenAcpiDevice();

    void
        APIENTRY
        CloseAcpiDevice(
            __in HANDLE hDriver
        );

    BOOLEAN
        APIENTRY
        EvalAcpiMethod(
            __in HANDLE hDriver,
            __in const char* puNameSeg,
            __in PVOID *outputBuffer
        );

    BOOLEAN
        APIENTRY
        EvalAcpiMethodArgs(
            __in HANDLE hDriver,
            __in const char* puNameSeg,
            __in PVOID pArgs,
            __in PVOID *outputBuffer
        );

    PVOID
        PutIntArg(
            PVOID   pArgs,
            UINT64  value
        );

    PVOID
        PutStringArg(
            PVOID   pArgs,
            UINT    Length,
            TCHAR *pString
        );

    PVOID
        PutBuffArg(
            PVOID   pArgs,
            UINT    Length,
            UCHAR *pBuf
        );

#ifdef __cplusplus
}
#endif