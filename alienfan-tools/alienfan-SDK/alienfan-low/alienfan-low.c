#include "alienfan-low.h"
#include <acpiioct.h>
#include <stdlib.h>
#include "ioctl.h"

#pragma comment(lib,"setupapi.lib")

TCHAR           DevInstanceId[201];
TCHAR           DevInstanceId1[201];

BOOLEAN
GetServiceName(
    __inout_bcount_full(BufferLength) TCHAR *DriverLocation,
    __in ULONG BufferLength
)
/*++

Routine Description:

    Get the current full path service name for install/stop/remove service

Arguments:

    DriverLocation  - Buffer to receive location of service
    BufferLength    - Buffer size

Return Value:

    TRUE            - Get service full path successfully
    FALSE           - Failed to get service full path

--*/
{
    HANDLE  fileHandle;
    TCHAR   driver[MAX_PATH];
    TCHAR   file[MAX_PATH];
    size_t  pcbLength = 0;
    size_t  Idx;

    if (DriverLocation == NULL || BufferLength < 1) {
        return FALSE;
    }

    if (GetSystemDirectory(DriverLocation, BufferLength - 1) == 0) {
        return FALSE;
    }
    if (GetCurrentDirectory(MAX_PATH, driver) == 0) {
        return FALSE;
    }
    GetModuleFileName(NULL, file, MAX_PATH - 1);

    if (FAILED(StringCbLength(file, MAX_PATH, &pcbLength)) || pcbLength == 0) {
        return FALSE;
    }
    pcbLength = pcbLength / sizeof(TCHAR);
    for (Idx = (pcbLength) -1; Idx > 0; Idx--) {
        if (file[Idx] == '\\') {
            file[Idx + 1] = 0;
            break;
        }
    }

    if (FAILED(StringCbCat(file, MAX_PATH, _T("HwAcc.sys")))) {
        return FALSE;
    }

    StringCbPrintf(DriverLocation, BufferLength, file);

    // test file is existing
    if ((fileHandle = CreateFile(file,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL
    )) == INVALID_HANDLE_VALUE) {

        //
        // Indicate failure.
        //
        return FALSE;
    }


    //Close open file handle.

    if (fileHandle) {

        CloseHandle(fileHandle);
    }
    //MessageBox (NULL, file, file, MB_OK);

  //
  // Setup path name to driver file.
  //
  //if (FAILED(StringCbCat(driver, MAX_PATH, _T("\\HwAcc.sys")))) {
  //    return FALSE;
  //}

  /*if (GetLastError () == 0) {

      StringCbCat (DriverLocation, MAX_PATH, _T("\\HwAcc.sys"));
  }*/

    return TRUE;
}

BOOLEAN
InstallService(
    __in SC_HANDLE SchSCManager,
    __in LPTSTR    ServiceExe
)
/*++

Routine Description:

    create a on-demanded service

Arguments:

    SchSCManager    - Handle of service control manager
    ServiceName     - Service Name to create
    ServiceExe      - Service Excute file path

Return Value:

    TRUE            - Service create and installed succesfully
    FALSE           - Failed to create service

--*/
{
    SC_HANDLE   schService;
    DWORD       err;

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    //
    // Create a new a service object.
    //

    schService = CreateService(SchSCManager,           // handle of service control manager database
                               DRIVER_NAME,            // address of name of service to start
                               DRIVER_NAME,            // address of display name
                               SERVICE_ALL_ACCESS,     // type of access to service
        SERVICE_WIN32_OWN_PROCESS,  // type of service
                               SERVICE_DEMAND_START,   // when to start service
                               SERVICE_ERROR_NORMAL,   // severity if service fails to start
                               ServiceExe,             // address of name of binary file
                               NULL,                   // service does not belong to a group
                               NULL,                   // no tag requested
                               NULL,                   // no dependency names
                               NULL,                   // use LocalSystem account
                               NULL                    // no password for service account
    );

    if (schService == NULL) {

        err = GetLastError();

        if (err == ERROR_DUPLICATE_SERVICE_NAME || err == ERROR_SERVICE_EXISTS) {
            return TRUE;
        } else {
            return  FALSE;
        }
    }

    //
    // Close the service object.
    //
    if (schService) {

        CloseServiceHandle(schService);
    }

    //
    // Indicate success.
    //
    return TRUE;
}

BOOLEAN
RemoveService(
    __in SC_HANDLE SchSCManager
)
/*++

Routine Description:

    Remove service

Arguments:

    SchSCManager    - Handle of service control manager
    ServiceName     - Service Name to remove

Return Value:

    TRUE            - Remove Service Successfully
    FALSE           - Failed to Remove Service

--*/
{
    SC_HANDLE   schService;
    BOOLEAN     rCode;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(
        SchSCManager,
        DRIVER_NAME,
        SERVICE_ALL_ACCESS
    );

    if (schService == NULL) {
        return FALSE;
    }

    //
    // Mark the service for deletion from the service control manager database.
    //
    if (DeleteService(schService)) {

        //
        // Indicate success.
        //
        rCode = TRUE;

    } else {
        rCode = FALSE;
    }

    //
    // Close the service object.
    //
    if (schService) {

        CloseServiceHandle(schService);
    }

    return rCode;

}

BOOLEAN
DemandService(
    __in SC_HANDLE SchSCManager,
    __in char* SrvName
)
/*++

Routine Description:

    Start service on demanded

Arguments:

    SchSCManager    - Handle of service control manager
    ServiceName     - Service Name to start

Return Value:

    TRUE            - Start Service Successfully
    FALSE           - Failed to Start Service

--*/
{
    SC_HANDLE   schService;
    BOOLEAN ret = FALSE;

    //
    // Open the handle to the existing service.
    //

    if (!(schService = OpenServiceA(SchSCManager,
        SrvName,
        SERVICE_ALL_ACCESS
    )))
        return FALSE;

    //
    // Start the execution of the service (i.e. start the driver).
    //

    if (!(ret = (BOOLEAN)StartService(schService,     // service identifier
                      0,              // number of arguments
                      NULL            // pointer to arguments
    ))) {
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {

            //
            // Ignore this error.
            //

            ret = TRUE;
        }

    }

    //
    // Close the service object.
    //
    CloseServiceHandle(schService);
    return ret;

}

BOOLEAN
StopService(
    __in SC_HANDLE SchSCManager,
    __in char* SrvName
)
/*++

Routine Description:

    Start service on demanded

Arguments:

    SchSCManager    - Handle of service control manager
    ServiceName     - Service Name to Stop

Return Value:

    TRUE            - Start Service Successfully
    FALSE           - Failed to Start Service

--*/
{
    BOOLEAN         rCode = TRUE;
    SC_HANDLE       schService;
    SERVICE_STATUS  serviceStatus;

    //
    // Open the handle to the existing service.
    //

    if (!(schService = OpenServiceA(SchSCManager,
        SrvName,
        SERVICE_ALL_ACCESS
    )))
        return FALSE;

    //
    // Request that the service stop.
    //

    rCode = (BOOLEAN)ControlService(schService,
        SERVICE_CONTROL_STOP,
        &serviceStatus
    );

    //
    // Close the service object.
    //
    CloseServiceHandle(schService);

    return rCode;

}

BOOL
GetAcpiDevice (
    PCTSTR Name,
    LPBYTE PropertyBuffer,
    DWORD Idx
)
/*++

Routine Description:

Get the current full path service name for install/stop/remove service

Arguments:

DriverLocation  - Buffer to receive location of service
BufferLength    - Buffer size

Return Value:

TRUE            - Get service full path successfully
FALSE           - Failed to get service full path

--*/
{
    //HDEVINFO    hdev;
    SP_DEVINFO_DATA devdata;
    DWORD       PropertyRegDataType;
    DWORD       RequiedSize;

    BOOL res = FALSE;

    HDEVINFO hdev = SetupDiGetClassDevsEx (NULL, Name, NULL, DIGCF_ALLCLASSES, NULL, NULL, NULL);

    if (hdev != INVALID_HANDLE_VALUE) {
        ZeroMemory (&devdata, sizeof (devdata));
        devdata.cbSize = sizeof (devdata);
        if (SetupDiEnumDeviceInfo(hdev, Idx, &devdata)) {
            if (SetupDiGetDeviceInstanceId(hdev, &devdata, &DevInstanceId[0], 200, NULL)) {
                CopyMemory(DevInstanceId1, DevInstanceId, 201);
                if (SetupDiGetDeviceRegistryProperty(hdev, &devdata, 0xE,
                                                     &PropertyRegDataType, &PropertyBuffer[0], 0x400, &RequiedSize)) {
                    res = TRUE;
                }
            }
        }
        SetupDiDestroyDeviceInfoList (hdev);
    }
    return res;
}

BOOLEAN
APIENTRY
EvalAcpiMethod(
    __in HANDLE hDriver,
    __in const char* puNameSeg,
    __in PVOID *outputBuffer,
    __in_opt PVOID pArgs
) {
    BOOL                        IoctlResult;
    DWORD                       ReturnedLength, EvalMethod;

    PACPI_EVAL_INPUT_BUFFER_EX      inbuf;
    PACPI_EVAL_OUTPUT_BUFFER       outbuf;
    PACPI_EVAL_OUTPUT_BUFFER       ActualData;

    EvalMethod = pArgs ? IOCTL_GPD_EVAL_ACPI_WITH_DIRECT : IOCTL_GPD_EVAL_ACPI_WITHOUT_DIRECT;

    if (pArgs)
        inbuf = (PACPI_EVAL_INPUT_BUFFER_EX)pArgs;
    else
        inbuf = malloc(sizeof(ACPI_EVAL_INPUT_BUFFER_EX));
    strcpy_s(inbuf->MethodName, strlen(puNameSeg) + 1, puNameSeg);

    outbuf = (PACPI_EVAL_OUTPUT_BUFFER) malloc(sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 256); // for arguments...

    IoctlResult = DeviceIoControl(
        hDriver,           // Handle to device
        EvalMethod,    // IO Control code for Read
        inbuf,        // Buffer to driver.
        pArgs ? ((PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)pArgs)->Size : sizeof(ACPI_EVAL_INPUT_BUFFER_EX), // Length of buffer in bytes.
        outbuf,     // Buffer from driver.
        sizeof(ACPI_EVAL_OUTPUT_BUFFER),
        &ReturnedLength,    // Bytes placed in DataBuffer.
        NULL                // NULL means wait till op. completes.
    );
    if (!IoctlResult) {
        if (GetLastError() == ERROR_MORE_DATA) {
            ReturnedLength = outbuf->Length;
            free(outbuf);
            outbuf = (PACPI_EVAL_OUTPUT_BUFFER)malloc(ReturnedLength);
            IoctlResult = DeviceIoControl(
                hDriver,           // Handle to device
                EvalMethod,    // IO Control code for Read
                inbuf,        // Buffer to driver.
                pArgs ? ((PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)pArgs)->Size : sizeof(ACPI_EVAL_INPUT_BUFFER_EX), // Length of buffer in bytes.
                outbuf,     // Buffer from driver.
                ReturnedLength,
                &ReturnedLength,    // Bytes placed in DataBuffer.
                NULL                // NULL means wait till op. completes.
            );
        }
    }
    if (!IoctlResult || outbuf->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {
        (*outputBuffer) = NULL;
        free(outbuf);
    }
    else {
        (*outputBuffer) = (PVOID)outbuf;
    }
    free(inbuf);
    return (BOOLEAN) IoctlResult;
}

//BOOLEAN
//APIENTRY
//EvalAcpiMethodArgs(
//    __in HANDLE hDriver,
//    __in const char* puNameSeg,
//    __in PVOID pArgs,
//    __in PVOID *outputBuffer
//) {
//    BOOL                        IoctlResult;
//    DWORD                       ReturnedLength, LastError;
//
//    PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX     pMethodEx;
//    PACPI_EVAL_OUTPUT_BUFFER               outbuf;
//    PACPI_EVAL_OUTPUT_BUFFER               ActualData;
//
//    pMethodEx = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) pArgs;
//
//    //pMethodEx->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;
//    strcpy_s(pMethodEx->MethodName, strlen(puNameSeg) + 1, puNameSeg);
//    //pMethodEx->Size = sizeof(pArgs);
//    //memcpy_s(pMethodEx->Argument, sizeof(pArgs), pArgs, sizeof(pArgs));
//    outbuf = (PACPI_EVAL_OUTPUT_BUFFER) malloc(sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 256);
//
//    IoctlResult = DeviceIoControl(
//        hDriver,           // Handle to device
//        IOCTL_GPD_EVAL_ACPI_WITH_DIRECT,    // IO Control code for Read
//        pMethodEx,        // Buffer to driver.
//        pMethodEx->Size, // Length of buffer in bytes.
//        outbuf,     // Buffer from driver.
//        sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 256,
//        &ReturnedLength,    // Bytes placed in DataBuffer.
//        NULL                // NULL means wait till op. completes.
//    );
//    if (!IoctlResult) {
//        LastError = GetLastError();
//        if (LastError == ERROR_MORE_DATA) {
//            ActualData = (PACPI_EVAL_OUTPUT_BUFFER) malloc(outbuf->Length);
//            if (ActualData == NULL) {
//                return FALSE;
//            }
//            IoctlResult = DeviceIoControl(
//                hDriver,           // Handle to device
//                IOCTL_GPD_EVAL_ACPI_WITH_DIRECT,    // IO Control code for Read
//                pMethodEx,        // Buffer to driver.
//                pMethodEx->Size, // Length of buffer in bytes.
//                ActualData,     // Buffer from driver.
//                outbuf->Length,
//                &ReturnedLength,    // Bytes placed in DataBuffer.
//                NULL                // NULL means wait till op. completes.
//            );
//            free(outbuf);
//            if (IoctlResult) {
//                if (outputBuffer != NULL) {
//                    (*outputBuffer) = (PVOID) ActualData;
//                    if (ActualData->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {
//                        (*outputBuffer) = NULL;
//                    }
//                }
//            }
//        }
//    } else {
//        (*outputBuffer) = (PVOID) outbuf;
//    }
//    free(pMethodEx);
//    return (BOOLEAN) IoctlResult;
//}

void
APIENTRY
CloseAcpiDevice(
    __in HANDLE hDriver
) {
    if (hDriver && hDriver != INVALID_HANDLE_VALUE)
        CloseHandle(hDriver);
    hDriver = NULL;
}

HANDLE
APIENTRY
OpenAcpiDevice (
)
/*++

Routine Description:

Open Acpi Device

Arguments:

hDriver     - Handle of service

Return Value:

TRUE        - Open ACPI driver acpi.sys ready
FALSE       - Failed to open acpi driver

--*/

//#include <fltuser.h>

{
    HANDLE      hDriver = NULL;
    BYTE        PropertyBuffer[0x401];
    //CHAR        AcpiName[0x401];
    BOOL        IoctlResult = FALSE;
    ACPI_NAME   acpi;
    ULONG       ReturnedLength = 0;
    //OSVERSIONINFO osinfo;
    //osinfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    //if (!GetVersionEx (&osinfo)) {
    //    //  printf ("OS Major Version is %d, Minor Version is %d", osinfo.dwMajorVersion, osinfo.dwMinorVersion);
    //    //printf("GetVersion failed\n");
    //    return NULL;
    //}

    //acpi.dwMajorVersion = osinfo.dwMajorVersion;
    //acpi.dwMinorVersion = osinfo.dwMinorVersion;
    //acpi.dwBuildNumber  = osinfo.dwBuildNumber;
    //acpi.dwPlatformId   = osinfo.dwPlatformId;
    //acpi.pAcpiDeviceName = PropertyBuffer;

    if ((hDriver = CreateFile(
        _T("\\\\?\\GLOBALROOT\\Device\\HWACC0"), // _T("\\\\.\\HwAcc"), //
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL)) == INVALID_HANDLE_VALUE) {
        //printf("Driver access failed\n");
        return hDriver;
    }

    PVOID hdev = SetupDiGetClassDevsEx(NULL, _T("ACPI_HAL"), NULL, DIGCF_ALLCLASSES, NULL, NULL, NULL);
    if (hdev != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA devdata;
        DWORD       PropertyRegDataType;
        DWORD       RequiedSize;

        for (int Idx = 0; ; Idx++) {
            ZeroMemory(&devdata, sizeof(devdata));
            devdata.cbSize = sizeof(devdata);
            if (SetupDiEnumDeviceInfo(hdev, Idx, &devdata)) {
                if (SetupDiGetDeviceInstanceId(hdev, &devdata, &DevInstanceId[0], 200, NULL)) {
                    //CopyMemory(DevInstanceId1, DevInstanceId, 201);
                    if (SetupDiGetDeviceRegistryProperty(hdev, &devdata, 0xE,
                        &PropertyRegDataType, &PropertyBuffer[0], 0x400, &RequiedSize)) {
                        //wcstombs_s(NULL, AcpiName, 200, (wchar_t*)PropertyBuffer, 400);
                        //WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, (TCHAR*)PropertyBuffer, 400, AcpiName, 400, NULL, NULL);
                        acpi.pAcpiDeviceName = PropertyBuffer;
                        acpi.uAcpiDeviceNameLength = (ULONG)strlen(acpi.pAcpiDeviceName);
                        // calling driver....
                        if (IoctlResult = DeviceIoControl(hDriver, (DWORD)IOCTL_GPD_OPEN_ACPI, &acpi, sizeof(ACPI_NAME), NULL, 0, &ReturnedLength, NULL)) {
                            //printf("IOControl OK for %d\n", Idx);
                            break;
                        }
                    }
                }
            }
            else break;
        }
        SetupDiDestroyDeviceInfoList(hdev);
    }

    //for (UINT Idx = 0; GetAcpiDevice(_T("ACPI_HAL"), PropertyBuffer, Idx); Idx++) {

    //    WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, (TCHAR*)PropertyBuffer, 400, AcpiName, 400, NULL, NULL);
    //    acpi.pAcpiDeviceName = AcpiName;

    //    acpi.uAcpiDeviceNameLength = (ULONG)strlen (acpi.pAcpiDeviceName);

    //    if (IoctlResult = DeviceIoControl(hDriver, (DWORD) IOCTL_GPD_OPEN_ACPI, &acpi, sizeof(ACPI_NAME), NULL, 0, &ReturnedLength, NULL)) {
    //        //printf("IOControl OK for %d\n", Idx);
    //        break;
    //    }
    //}

    if (!IoctlResult) {
        CloseHandle(hDriver);
        hDriver = INVALID_HANDLE_VALUE;
    }

    return hDriver;
}

PVOID
PutIntArg(
    PVOID   pArgs,
    UINT64  value
)
/*++

Routine Description:

    Put the Input Args

Arguments:

    pArgs           -- Complexity Argument ptr
    value           -- UInt64 Value

Return Value:

    PVOID           -- Complexity Argument ptr

--*/
{
    ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX* pInputArgs = pArgs;
    ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX* pLocalArgs = pArgs;
    ACPI_METHOD_ARGUMENT* pArg;

    if (pInputArgs == NULL) {
        // allocate the memory buffer for new args
        pInputArgs = malloc(sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) + sizeof(UINT64));
        if (pInputArgs == NULL) {
            return NULL;
        }
        pInputArgs->ArgumentCount = 1;
        pInputArgs->Size = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) + sizeof(UINT64);
        pInputArgs->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;
        pInputArgs->Argument[0].Type = ACPI_METHOD_ARGUMENT_INTEGER;
        pInputArgs->Argument[0].DataLength = sizeof(UINT64);
        memcpy(pInputArgs->Argument[0].Data, &value, sizeof(UINT64));
        pArg = pInputArgs->Argument;
        pArg = (ACPI_METHOD_ARGUMENT*)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        pInputArgs->Size = (ULONG)((UINT64)pArg - (UINT64)pInputArgs);
    }
    else {
        pInputArgs = malloc(pInputArgs->Size +sizeof(ACPI_METHOD_ARGUMENT) + sizeof(UINT64));
        if (pInputArgs == NULL) {
            return NULL;
        }
        memcpy(pInputArgs, pArgs, pLocalArgs->Size);
        pInputArgs->Size = pInputArgs->Size + sizeof(ACPI_METHOD_ARGUMENT) + sizeof(UINT64);
        pArg = pInputArgs->Argument;
        for (ULONG uIndex = 0; uIndex < pInputArgs->ArgumentCount; uIndex++) {
            pArg = (ACPI_METHOD_ARGUMENT *)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        }
        pArg->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        pInputArgs->ArgumentCount ++;
        pArg->DataLength = sizeof(UINT64);
        memcpy(pArg->Data, &value, sizeof(UINT64));
        pArg = (ACPI_METHOD_ARGUMENT*)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        pInputArgs->Size = (ULONG)((UINT64)pArg - (UINT64)pInputArgs);
        free(pArgs);
    }
    return pInputArgs;
}

PVOID
PutStringArg(
    PVOID   pArgs,
    UINT    Length,
    TCHAR   *pString
)
/*++

Routine Description:

    Put the Input Args

Arguments:

    pArgs           -- Complexity Argument ptr
    pString         -- Strings

Return Value:

    PVOID           -- Complexity Argument ptr

--*/
{
    ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX* pInputArgs = pArgs;
    ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX* pLocalArgs = pArgs;
    ACPI_METHOD_ARGUMENT* pArg;
    UINT uIndex;

    if (pInputArgs == NULL) {
        // allocate the memory buffer for new args
        pInputArgs = malloc(sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) + Length + 1);
        if (pInputArgs == NULL) {
            return NULL;
        }
        pInputArgs->ArgumentCount = 1;
        pInputArgs->Size = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) + Length + 1;
        pInputArgs->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;
        pInputArgs->Argument[0].Type = ACPI_METHOD_ARGUMENT_STRING;
        pInputArgs->Argument[0].DataLength = Length + 1;
        for (uIndex = 0; uIndex < Length; uIndex++) {
            pInputArgs->Argument[0].Data[uIndex] = (UCHAR)pString[uIndex];
        }
        pInputArgs->Argument[0].Data[uIndex] = 0;
        pArg = pInputArgs->Argument;
        pArg = (ACPI_METHOD_ARGUMENT*)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        pInputArgs->Size = (ULONG)((UINT64)pArg - (UINT64)pInputArgs);
    }
    else {
        pInputArgs = malloc(pInputArgs->Size + sizeof(ACPI_METHOD_ARGUMENT) + Length + 1);
        if (pInputArgs == NULL) {
            free(pArgs);
            return NULL;
        }
        memcpy(pInputArgs, pArgs, pLocalArgs->Size);
        pInputArgs->Size = pInputArgs->Size + sizeof(ACPI_METHOD_ARGUMENT) + Length + 1;
        pArg = pInputArgs->Argument;
        for (ULONG uIndex = 0; uIndex < pInputArgs->ArgumentCount; uIndex++) {
            pArg = (ACPI_METHOD_ARGUMENT*)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        }
        pInputArgs->ArgumentCount++;
        pArg->Type = ACPI_METHOD_ARGUMENT_STRING;
        pArg->DataLength = Length + 1;
        for (uIndex = 0; uIndex < Length; uIndex++) {
            pArg->Data[uIndex] = (UCHAR)pString[uIndex];
        }
        pArg->Data[uIndex] = 0;
        pArg = (ACPI_METHOD_ARGUMENT*)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        pInputArgs->Size = (ULONG)((UINT64)pArg - (UINT64)pInputArgs);
        free(pArgs);
    }
    return pInputArgs;
}

PVOID
PutBuffArg(
    PVOID   pArgs,
    UINT    Length,
    UCHAR*  pBuf
)
/*++

Routine Description:

    Put the Input Args

Arguments:

    pArgs           -- Complexity Argument ptr
    pBuf            -- Buffer ptr

Return Value:

    PVOID           -- Complexity Argument ptr

--*/
{
    ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX* pInputArgs = pArgs;
    ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX* pLocalArgs = pArgs;
    ACPI_METHOD_ARGUMENT* pArg;

    if (pInputArgs == NULL) {
        // allocate the memory buffer for new args
        pInputArgs = malloc(sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) + Length + 1);
        if (pInputArgs == NULL) {
            return NULL;
        }
        pInputArgs->ArgumentCount = 1;
        pInputArgs->Size = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) + Length;
        pInputArgs->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;
        pInputArgs->Argument[0].Type = ACPI_METHOD_ARGUMENT_BUFFER;
        pInputArgs->Argument[0].DataLength =(USHORT) Length;
        memcpy(pInputArgs->Argument[0].Data, pBuf, Length);
        pArg = pInputArgs->Argument;
        pArg = (ACPI_METHOD_ARGUMENT*)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        pInputArgs->Size = (ULONG)((UINT64)pArg - (UINT64)pInputArgs);
    }
    else {
        pInputArgs = malloc(pInputArgs->Size + sizeof(ACPI_METHOD_ARGUMENT) + Length + 1);
        if (pInputArgs == NULL) {
            free(pArgs);
            return NULL;
        }
        memcpy(pInputArgs, pArgs, pLocalArgs->Size);
        pInputArgs->Size = pInputArgs->Size + sizeof(ACPI_METHOD_ARGUMENT) + Length + 1;
        pArg = pInputArgs->Argument;
        for (ULONG uIndex = 0; uIndex < pInputArgs->ArgumentCount; uIndex++) {
            pArg = (ACPI_METHOD_ARGUMENT*)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        }
        pInputArgs->ArgumentCount++;
        pArg->DataLength = (USHORT)Length;
        pArg->Type = ACPI_METHOD_ARGUMENT_BUFFER;
        memcpy(pArg->Data, pBuf, Length);
        free(pArgs);
        pArg = (ACPI_METHOD_ARGUMENT*)ACPI_METHOD_NEXT_ARGUMENT(pArg);
        pInputArgs->Size = (ULONG)((UINT64)pArg - (UINT64)pInputArgs);
    }
    return pInputArgs;
}