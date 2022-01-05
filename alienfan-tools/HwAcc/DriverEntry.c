/*++

2006-2020  NickelS

Module Name:

	HwAcc.c

Abstract:

	Purpose of this driver is to demonstrate how the four different types
	of IOCTLs can be used, and how the I/O manager handles the user I/O
	buffers in each case. This sample also helps to understand the usage of
	some of the memory manager functions.

Environment:

	Kernel mode only.

--*/

// This is a kernel-mode driver
#include <ntddk.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <stdio.h>
#include "def.h"
#include "access.h"

#define NT_DEVICE_NAME      L"\\Device\\HWACC0"
#define DOS_DEVICE_NAME     L"\\DosDevices\\HwAcc"

#ifdef KERNEL_HACK
DRIVER_INITIALIZE DriverInit;
#endif
DRIVER_INITIALIZE DriverEntry;
DRIVER_DISPATCH CreateClose;
DRIVER_DISPATCH DeviceControl;
#ifndef KERNEL_HACK
DRIVER_UNLOAD UnloadDriver;
#endif

#ifdef ALLOC_PRAGMA
#ifdef KERNEL_HACK
#pragma alloc_text( INIT, DriverInit)
#else
#pragma alloc_text( INIT, DriverEntry)
#pragma alloc_text( PAGE, UnloadDriver)
#endif
#pragma alloc_text( PAGE, CreateClose)
#pragma alloc_text( PAGE, DeviceControl)
#pragma alloc_text( PAGE, OpenAcpiDevice)
#endif // ALLOC_PRAGMA


#ifdef KERNEL_HACK
NTKERNELAPI
NTSTATUS
IoCreateDriver(
	_In_ PUNICODE_STRING DriverName, OPTIONAL
	_In_ PDRIVER_INITIALIZE InitializationFunction
);

NTSTATUS 
DriverInit(
	_In_  struct _DRIVER_OBJECT* DriverObject,
	_In_  PUNICODE_STRING RegistryPath
) {
	//NTSTATUS        status;
	UNICODE_STRING  drvName;

	/* This parameters are invalid due to nonstandard way of loading and should not be used. */
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DebugPrint(("HWACC: Init, Calling entry point..."));

	RtlInitUnicodeString(&drvName, NT_DEVICE_NAME);
	//status = IoCreateDriver(&drvName, &DriverEntry);

	return IoCreateDriver(&drvName, &DriverEntry); //status;
}
#endif

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT   DriverObject,
	_In_ PUNICODE_STRING  RegistryPath
)
/*++

Routine Description:
	This routine is called by the Operating System to initialize the driver.

	It creates the device object, fills in the dispatch entry points and
	completes the initialization.

Arguments:
	DriverObject - a pointer to the object that represents this device
	driver.

	RegistryPath - a pointer to our Services key in the registry.

Return Value:
	STATUS_SUCCESS if initialized; an error otherwise.

--*/

{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS        ntStatus;
	UNICODE_STRING  ntUnicodeString;    // NT Device Name "\Device\HwAcc"
	//UNICODE_STRING  ntWin32NameString;    // Win32 Name "\DosDevices\HwAcc"
	PDEVICE_OBJECT  deviceObject = NULL;    // ptr to device object


	RtlInitUnicodeString(&ntUnicodeString, NT_DEVICE_NAME);
	//RtlInitUnicodeString(&ntWin32NameString, DOS_DEVICE_NAME);

	DebugPrint(("HWACC: Entry Point"));

	ntStatus = IoCreateDevice(
		DriverObject,                   // Our Driver Object
		sizeof(LOCAL_DEVICE_INFO),     // We use a device extension
		&ntUnicodeString,               // Device name "\Device\HwAcc"
		FILE_DEVICE_UNKNOWN,            // Device type
		FILE_DEVICE_SECURE_OPEN,     // Device characteristics
		FALSE,                          // Not an exclusive device
		&deviceObject);                // Returned ptr to Device Object

	if (!NT_SUCCESS(ntStatus) || !deviceObject)
	{
		DebugPrint(("HWACC: Couldn't create the device object\n"));
		return ntStatus;
	}
	DebugPrint(("HWACC: Create the device object successfully\n"));

	//
	// Create a symbolic link between our device name  and the Win32 name
	//

	//ntStatus = IoCreateSymbolicLink(
	//	&ntWin32NameString, &ntUnicodeString);

	//if (!NT_SUCCESS(ntStatus))
	//{
	//	//
	//	// Delete everything that this routine has allocated.
	//	//
	//	DebugPrint(("HWACC: Couldn't create symbolic link\n"));
	//	IoDeleteDevice(deviceObject);
	//}

	//
	// Initialize the driver object with this driver's entry points.
	//

	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
#ifdef KERNEL_HACK
	DriverObject->DriverUnload = NULL;
	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
#else
	DriverObject->DriverUnload = UnloadDriver;
#endif

	DebugPrint(("HWACC: Init done\n"));

	return ntStatus;
}

NTSTATUS
CreateClose(
	_In_ PDEVICE_OBJECT DeviceObject, //__in PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp //__in PIRP Irp
)
/*++

Routine Description:

	This routine is called by the I/O system when the HwAcc is opened or
	closed.

	No action is performed other than completing the request successfully.

Arguments:

	DeviceObject - a pointer to the object that represents the device
	that I/O is to be done on.

	Irp - a pointer to the I/O Request Packet for this request.

Return Value:

	NT status code

--*/

{
	UNREFERENCED_PARAMETER(DeviceObject);
	//UNREFERENCED_PARAMETER(Irp);
	PAGED_CODE();

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

#ifndef _TINY_DRIVER_
void
RemoveAcpiNS(
	__in PACPI_NAME_NODE	pAcpiNS
)
/*++

Routine Description:

	This routine removes the ACPI_OBJ node from the link

Arguments:

	pAcpiNS - a pointer to the acpi namespace object

Return Value:

	None
--*/
{
	PACPI_NAME_NODE pPrev = pAcpiNS->pPrev;
	PACPI_NAME_NODE pCurrent = pAcpiNS->pNext;

	pCurrent->pPrev = pPrev;
	pPrev->pNext = pCurrent;
}
#endif

#ifndef KERNEL_HACK
VOID
UnloadDriver(
	__in PDRIVER_OBJECT DriverObject
)
/*++

Routine Description:

	This routine is called by the I/O system to unload the driver.

	Any resources previously allocated must be freed.

Arguments:

	DriverObject - a pointer to the object that represents our driver.

Return Value:

	None
--*/

{


	PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
	//UNICODE_STRING uniWin32NameString;

	PAGED_CODE();
#ifndef _TINY_DRIVER_
	PLOCAL_DEVICE_INFO  pLDI;
	pLDI = (PLOCAL_DEVICE_INFO)deviceObject->DeviceExtension;    // Get local info struct

	if (pLDI->pSetupAml != NULL) {
		RemoveAcpiNS((PACPI_NAME_NODE)pLDI->pSetupAml);
		ExFreePoolWithTag(pLDI->pSetupAml, MYNT_TAG);
		pLDI->pSetupAml = NULL;
	}
#endif

	//
	// Create counted string version of our Win32 device name.
	//

	//RtlInitUnicodeString(&uniWin32NameString, DOS_DEVICE_NAME);


	//
	// Delete the link from our device name to a name in the Win32 namespace.
	//

	//IoDeleteSymbolicLink(&uniWin32NameString);

	if (deviceObject != NULL)
	{
		IoDeleteDevice(deviceObject);
	}
	DebugPrint(("HWACC: Device unloaded\n"));
}
#endif

NTSTATUS
DeviceControl(
	_In_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP pIrp
)

/*++

Routine Description:

	This routine is called by the I/O system to perform a device I/O
	control function.

Arguments:

	DeviceObject - a pointer to the object that represents the device
		that I/O is to be done on.

	Irp - a pointer to the I/O Request Packet for this request.

Return Value:

	NT status code

--*/

{

	//UNREFERENCED_PARAMETER(DeviceObject);

	PIO_STACK_LOCATION  pIrpStack;// Pointer to current stack location
	NTSTATUS            Status = STATUS_SUCCESS;// Assume success
	//ULONG               inBufLength; // Input buffer length
	//ULONG               outBufLength; // Output buffer length
	//PCHAR               inBuf, outBuf; // pointer to Input and output buffer
	//PCHAR               data = "This String is from Device Driver !!!";
	//ULONG               datalen = strlen(data) + 1;//Length of data including null
	//PMDL                mdl = NULL;
	//PCHAR               buffer = NULL;
	PLOCAL_DEVICE_INFO  pLDI;
	//PDEVICE_OBJECT      pDevObj;

	PAGED_CODE();

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	if (!pIrpStack) {
		DebugPrint(("HWACC: Internal error - zero stack\n"));
		return STATUS_INTERNAL_ERROR;
	}

	pLDI = (PLOCAL_DEVICE_INFO)DeviceObject->DeviceExtension;    // Get local info struct
	//inBufLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	//outBufLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_GPD_OPEN_ACPI:
		//pDevObj = IoGetLowerDeviceObject (DeviceObject);
		Status = OpenAcpiDevice(pLDI, pIrp);
		break;
	case IOCTL_GPD_EVAL_ACPI_WITHOUT_DIRECT:
	    Status = EvalAcpiWithoutInputDirect(pLDI, pIrp, pIrpStack);
	    break;
	case IOCTL_GPD_EVAL_ACPI_WITH_DIRECT:
	    Status = EvalAcpiWithInputDirect(pLDI, pIrp, pIrpStack);
	    break;
#ifndef _TINY_DRIVER_
	case IOCTL_GPD_ENUM_ACPI:
		Status = BuildUserAcpiObjects(pLDI, pIrp, pIrpStack);
		break;
	case IOCTL_GPD_READ_ACPI_MEMORY:
		Status = ReadAcpiMemory(pLDI, pIrp, pIrpStack);
		break;
	case IOCTL_GPD_EVAL_ACPI_WITHOUT_PARAMETER:
		Status = EvalAcpiWithoutInput(pLDI, pIrp, pIrpStack);
		break;
	case IOCTL_GPD_EVAL_ACPI_WITH_PARAMETER:
		Status = EvalAcpiWithInput(pLDI, pIrp, pIrpStack);
		break;
	case IOCTL_GPD_LOAD_AML:
		Status = SetupAmlForNotify(pLDI, pIrp, pIrpStack);
		break;
	case IOCTL_GPD_NOTIFY_DEVICE:
		Status = SetupAmlForNotify(pLDI, pIrp, pIrpStack);
		break;
	case IOCTL_GPD_UNLOAD_AML:
		Status = RemoveAmlForNotify(pLDI, pIrp, pIrpStack);
		break;
#endif
	default:
		Status = STATUS_INVALID_PARAMETER;
	}

#ifdef _DEBUG
	if (!NT_SUCCESS(Status))
		DebugPrint(("HWACC: Control operation failed (more data?)\n"));
#endif

	pIrp->IoStatus.Status = Status;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return Status;
}