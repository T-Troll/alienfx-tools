//#include "pch.h"
#include "KDL.h"
#include "global.h"
//#include <ntstatus.h>
//#include <windows.h>
//#include <winternl.h>

INT KDUProcessDrvMapSwitch(
    _In_ ULONG HvciEnabled,
    _In_ ULONG NtBuildNumber,
    _In_ ULONG ProviderId,
    _In_ ULONG ShellVersion,
    _In_ LPWSTR DriverFileName,
    _In_opt_ LPWSTR DriverObjectName,
    _In_opt_ LPWSTR DriverRegistryPath
) {
    INT retVal = 0;
    KDU_CONTEXT *provContext;

    if (!RtlDoesFileExists_U(DriverFileName)) {

        //supPrintfEvent(kduEventError,
        //               "[!] Input file cannot be found, abort.\r\n");

        return 0;
    }

    //printf_s("[*] Driver mapping using shellcode version: %lu\r\n", ShellVersion);

    if (ShellVersion == KDU_SHELLCODE_V3) {

        if (DriverObjectName == NULL) {

            /*supPrintfEvent(kduEventError, "[!] Driver object name is required when working with this shellcode\r\n"\
                           "[?] Use the following commands to supply object name and optionally registry key name\r\n"\
                           "\t-drvn [ObjectName] and/or\r\n"\
                           "\t-drvr [ObjectKeyName]\r\n"\
                           "\te.g. kdu -scv 3 -drvn MyName -map MyDriver.sys\r\n"
            );*/

            return 0;
        } /*else {
            printf_s("[+] Driver object name: \"%ws\"\r\n", DriverObjectName);
        }*/

        //if (DriverRegistryPath) {
        //    printf_s("[+] Registry key name: \"%ws\"\r\n", DriverRegistryPath);
        //} else {
        //    printf_s("[+] No driver registry key name specified, driver object name will be used instead\r\n");
        //}

    }

    PVOID pvImage = NULL;
    NTSTATUS ntStatus = supLoadFileForMapping(DriverFileName, &pvImage);

    if ((!NT_SUCCESS(ntStatus)) || (pvImage == NULL)) {

        /*supPrintfEvent(kduEventError,
                       "[!] Error while loading input driver file, NTSTATUS (0x%lX)\r\n", ntStatus);*/

        return 0;
    } else {
        /*printf_s("[+] Input driver file loaded at 0x%p\r\n", pvImage);*/

        provContext = KDUProviderCreate(ProviderId,
                                        HvciEnabled,
                                        NtBuildNumber,
                                        ShellVersion,
                                        ActionTypeMapDriver);

        if (provContext) {

            if (ShellVersion == KDU_SHELLCODE_V3) {

                if (DriverObjectName) {
                    ScCreateFixedUnicodeString(&provContext->DriverObjectName,
                                               DriverObjectName);

                }

                //
                // Registry path name is optional.
                // If not specified we will assume its the same name as driver object.
                //
                if (DriverRegistryPath) {
                    ScCreateFixedUnicodeString(&provContext->DriverRegistryPath,
                                               DriverRegistryPath);
                }

            }

            retVal = KDUMapDriver(provContext, pvImage);
            KDUProviderRelease(provContext);
        }

        LdrUnloadDll(pvImage);
    }

    return retVal;
}

BOOLEAN
APIENTRY
LoadKernelDriver(
	LPWSTR DriverName,
    LPWSTR DriverDevice
) {

    BOOLEAN hvciEnabled;
    BOOLEAN hvciStrict;
    BOOLEAN hvciIUM;

    OSVERSIONINFO osv = {0};

    RtlSecureZeroMemory(&osv, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(osv);
    RtlGetVersion((PRTL_OSVERSIONINFOW) &osv);
    if ((osv.dwMajorVersion < 6) ||
        (osv.dwMajorVersion == 6 && osv.dwMinorVersion == 0) ||
        (osv.dwBuildNumber == 7600)) {

        return FALSE;
    }

    //
        // Providers maybe *not* HVCI compatible.
        //
    supQueryHVCIState(&hvciEnabled, &hvciStrict, &hvciIUM);

    return (BOOLEAN) KDUProcessDrvMapSwitch(
        hvciEnabled,
        osv.dwBuildNumber,
        6, // Provider number
        3, //3, // ShellCode version
        DriverName,
        DriverDevice,
        DriverDevice // Registry path
    );
}