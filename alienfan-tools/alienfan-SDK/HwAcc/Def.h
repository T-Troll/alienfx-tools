/*++

2006-2020  NickelS

Module Name:

	Def.h

Abstract:

	Include driver extension data structure and legacy definition related to old WinDDK and OS

Environment:

	Kernel mode only.

--*/

#pragma once

#if AMD64
#define DisableInterrupt() _disable ()
#else
#define DisableInterrupt() __asm cli
#endif

#if AMD64
#define EnableInterrupt() _enable ()
#else
#define EnableInterrupt() __asm sti
#endif

#ifdef _DEBUG
#define DebugPrint(_x_) \
               DbgPrint _x_;

#else
#define DebugPrint(_x_)
#endif

#define MYNT_TAG 'gTyN'
#define MYNT_OBJ_LEN 0x200
#ifdef AMD64
#define UINT UINT64
#else
#define UINT UINT32
#endif

typedef UINT * PUINT;

#pragma pack(1)
// driver local data structure specific to each device object
typedef struct _LOCAL_DEVICE_INFO {
	PFILE_OBJECT  	LowFileObject;
	PDEVICE_OBJECT 	LowDeviceObject;
	PVOID			PDevicePath;
	PVOID			pSetupAml;
	PVOID			AcpiObject;
	PVOID			pAcpiScope;
	UINT            uAcpiOffset;
	ULONG           dwMajorVersion;
	ULONG           dwMinorVersion;
	ULONG           dwBuildNumber;
	ULONG           dwPlatformId;
} LOCAL_DEVICE_INFO, *PLOCAL_DEVICE_INFO;

#pragma pack()