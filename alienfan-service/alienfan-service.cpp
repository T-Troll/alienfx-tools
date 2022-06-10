#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <SetupAPI.h>
#include <acpiioct.h>
//#include "../alienfan-tools/alienfan-SDK/alienfan-low/Ioctl.h"

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib,"setupapi.lib")

#define SVCNAME TEXT("AlienFan")

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

VOID SvcInstall(void);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR*);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR*);
VOID SvcReportEvent(LPTSTR);

typedef struct {
    PVOID   pAcpiDeviceName;
    ULONG   uAcpiDeviceNameLength;
    ULONG   dwMajorVersion;
    ULONG   dwMinorVersion;
    ULONG   dwBuildNumber;
    ULONG   dwPlatformId;
} ACPI_NAME;

BOOLEAN
APIENTRY
EvalAcpiMethod(
    __in HANDLE hDriver,
    __in const char* puNameSeg,
    __in PVOID* outputBuffer,
    __in PVOID pArgs
) {
    BOOL                        IoctlResult;
    DWORD                       ReturnedLength, EvalMethod;

    PACPI_EVAL_INPUT_BUFFER_EX      inbuf;
    PACPI_EVAL_OUTPUT_BUFFER       outbuf;
    //PACPI_EVAL_OUTPUT_BUFFER       ActualData;

    EvalMethod = IOCTL_ACPI_EVAL_METHOD_EX;// pArgs ? IOCTL_GPD_EVAL_ACPI_WITH_DIRECT : IOCTL_GPD_EVAL_ACPI_WITHOUT_DIRECT;

    if (pArgs)
        inbuf = (PACPI_EVAL_INPUT_BUFFER_EX)pArgs;
    else
        inbuf = (PACPI_EVAL_INPUT_BUFFER_EX)malloc(sizeof(ACPI_EVAL_INPUT_BUFFER_EX));
    strcpy_s(inbuf->MethodName, strlen(puNameSeg) + 1, puNameSeg);

    outbuf = (PACPI_EVAL_OUTPUT_BUFFER)malloc(sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 256); // for arguments...
    inbuf->Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE_EX;

    OutputDebugStringA("Calling");
    OutputDebugStringA(inbuf->MethodName);

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
        char buf[256];
        sprintf_s(buf, 255, "Failed with code %d", GetLastError());
        OutputDebugStringA(buf);
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
        OutputDebugStringA("Success!\n");
        (*outputBuffer) = (PVOID)outbuf;
    }
    free(inbuf);
    return (BOOLEAN)IoctlResult;
}

BOOL
GetAcpiDevice(
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
    HDEVINFO    hdev;
    SP_DEVINFO_DATA devdata;
    DWORD       PropertyRegDataType;
    DWORD       RequiedSize;

    TCHAR           DevInstanceId[201];
    TCHAR           DevInstanceId1[201];

    BOOL res = FALSE;

    hdev = SetupDiGetClassDevsEx(NULL, Name, NULL, DIGCF_ALLCLASSES, NULL, NULL, NULL);

    if (hdev != INVALID_HANDLE_VALUE) {
        ZeroMemory(&devdata, sizeof(devdata));
        devdata.cbSize = sizeof(devdata);
        if (SetupDiEnumDeviceInfo(hdev, Idx, &devdata)) {
            if (SetupDiGetDeviceInstanceId(hdev, &devdata, &DevInstanceId[0], 200, NULL)) {
                CopyMemory(DevInstanceId1, DevInstanceId, 201);
                if (SetupDiGetDeviceRegistryProperty(hdev, &devdata, 0xE,
                    &PropertyRegDataType, &PropertyBuffer[0], 0x400, &RequiedSize)) {
                    res = TRUE;
                }
            }
        }
        SetupDiDestroyDeviceInfoList(hdev);
    }
    return res;
}

//
// Purpose:
//   Entry point for the process
//
// Parameters:
//   None
//
// Return value:
//   None, defaults to 0 (zero)
//
int __cdecl _tmain(int argc, TCHAR* argv[])
{
    // If command-line parameter is "install", install the service.
    // Otherwise, the service is probably being started by the SCM.

    if (lstrcmpi(argv[1], TEXT("install")) == 0)
    {
        SvcInstall();
        return 0;
    }

    // TO_DO: Add any additional services for the process to this table.
    SERVICE_TABLE_ENTRY DispatchTable[] =
    {
        { (LPWSTR)SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
        { NULL, NULL }
    };

    // This call returns when the service has stopped.
    // The process should simply terminate when the call returns.

    if (!StartServiceCtrlDispatcher(DispatchTable))
    {
        SvcReportEvent((LPTSTR)TEXT("StartServiceCtrlDispatcher"));
    }
}

//
// Purpose:
//   Installs a service in the SCM database
//
// Parameters:
//   None
//
// Return value:
//   None
//
VOID SvcInstall()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    TCHAR szUnquotedPath[MAX_PATH];

    if (!GetModuleFileName(NULL, szUnquotedPath, MAX_PATH))
    {
        printf("Cannot install service (%d)\n", GetLastError());
        return;
    }

    // In case the path contains a space, it must be quoted so that
    // it is correctly interpreted. For example,
    // "d:\my share\myservice.exe" should be specified as
    // ""d:\my share\myservice.exe"".
    TCHAR szPath[MAX_PATH];
    StringCbPrintf(szPath, MAX_PATH, TEXT("\"%s\""), szUnquotedPath);

    // Get a handle to the SCM database.

    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database
        SC_MANAGER_ALL_ACCESS);  // full access rights

    if (NULL == schSCManager)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    // Create the service

    schService = CreateService(
        schSCManager,              // SCM database
        SVCNAME,                   // name of service
        SVCNAME,                   // service name to display
        SERVICE_ALL_ACCESS,        // desired access
        SERVICE_WIN32_OWN_PROCESS, // service type
        SERVICE_DEMAND_START,      // start type
        SERVICE_ERROR_NORMAL,      // error control type
        szPath,                    // path to service's binary
        NULL,                      // no load ordering group
        NULL,                      // no tag identifier
        NULL,                      // no dependencies
        NULL,                      // LocalSystem account
        NULL);                     // no password

    if (schService == NULL)
    {
        printf("CreateService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return;
    }
    else printf("Service installed successfully\n");

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

//
// Purpose:
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
//
// Return value:
//   None.
//
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    // Register the handler function for the service

    gSvcStatusHandle = RegisterServiceCtrlHandler(
        SVCNAME,
        SvcCtrlHandler);

    if (!gSvcStatusHandle)
    {
        SvcReportEvent((LPTSTR)TEXT("RegisterServiceCtrlHandler"));
        return;
    }

    // These SERVICE_STATUS members remain as set here

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gSvcStatus.dwServiceSpecificExitCode = 0;

    // Report initial status to the SCM

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    // Perform service-specific initialization and work.

    SvcInit(dwArgc, lpszArgv);
}

//
// Purpose:
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
//
// Return value:
//   None
//
VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
    HANDLE      hDriver = INVALID_HANDLE_VALUE;
    BYTE        PropertyBuffer[0x401];
    CHAR        AcpiName[0x401];
    BOOL        IoctlResult = FALSE;
    ACPI_NAME   acpi;

    // Create an event. The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.

    ghSvcStopEvent = CreateEvent(
        NULL,    // default security attributes
        TRUE,    // manual reset event
        FALSE,   // not signaled
        NULL);   // no name

    if (ghSvcStopEvent == NULL)
    {
        ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 0);

    for (UINT Idx = 0; GetAcpiDevice(_T("ACPI_HAL"), PropertyBuffer, Idx); Idx++) {

        CHAR mainPath[256] = "\\\\?\\GLOBALROOT";

        WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, (TCHAR*)PropertyBuffer, 400, AcpiName, 400, NULL, NULL);
        acpi.pAcpiDeviceName = AcpiName;

        strcat_s(mainPath, 255, AcpiName);

        acpi.uAcpiDeviceNameLength = (ULONG)strlen((char*)acpi.pAcpiDeviceName);

        OutputDebugString(L"Trying open ACPI: ");
        OutputDebugStringA(mainPath);
        OutputDebugString(L"\n");

        if ((hDriver = CreateFileA(
            mainPath, //_T("\\\\?\\GLOBALROOT\\Device\\HWACC0"), // _T("\\\\.\\HwAcc"), //
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL)) != INVALID_HANDLE_VALUE) {
            //printf("Driver access failed\n");
            OutputDebugString(L"Success!\n");
            break;
        }
    }

    if (hDriver == INVALID_HANDLE_VALUE)
    {
        OutputDebugString(L"Failed!\n");
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    // Report running status when initialization is complete.

    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    PACPI_EVAL_OUTPUT_BUFFER resName = NULL;

    EvalAcpiMethod(hDriver, "\\_SB.PCI0.LPCB.EC0.SEN2._STR", (PVOID*)&resName, NULL);

    // TO_DO: Perform work until service stops.

    while (1)
    {
        // Check whether to stop the service.

        WaitForSingleObject(ghSvcStopEvent, INFINITE);
        OutputDebugString(L"Stopping...\n");
        if (hDriver != INVALID_HANDLE_VALUE)
            CloseHandle(hDriver);
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }
}

//
// Purpose:
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation,
//     in milliseconds
//
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure.

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ((dwCurrentState == SERVICE_RUNNING) ||
        (dwCurrentState == SERVICE_STOPPED))
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//
// Purpose:
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
//
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
    // Handle the requested control code.

    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

        // Signal the service to stop.

        SetEvent(ghSvcStopEvent);
        ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

        return;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        break;
    }

}

//
// Purpose:
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
//
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPTSTR szFunction)
{
    HANDLE hEventSource;
    LPCTSTR lpszStrings[2];
    TCHAR Buffer[80];

    hEventSource = RegisterEventSource(NULL, SVCNAME);

    if (NULL != hEventSource)
    {
        StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

        lpszStrings[0] = SVCNAME;
        lpszStrings[1] = Buffer;

        ReportEvent(hEventSource,        // event log handle
            EVENTLOG_ERROR_TYPE, // event type
            0,                   // event category
            0, //SVC_ERROR,           // event identifier
            NULL,                // no security identifier
            2,                   // size of lpszStrings array
            0,                   // no binary data
            lpszStrings,         // array of strings
            NULL);               // no binary data

        DeregisterEventSource(hEventSource);
    }
}


