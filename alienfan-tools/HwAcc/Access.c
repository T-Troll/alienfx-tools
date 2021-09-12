/*++

2006-2020  NickelS

Module Name:

	Access.c

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
#include <string.h>
#include <Acpiioct.h>   
#include "def.h"
#include "access.h"

#define MY_TAG 'gTyM'   	// Pool tag for memory allocation

NTSTATUS
SendDownStreamIrp(
	IN PDEVICE_OBJECT   Pdo,
	IN ULONG            Ioctl,
	IN PVOID            InputBuffer,
	IN ULONG            InputSize,
	IN PVOID            OutputBuffer,
	IN ULONG            OutputSize
)

/*
Routine Description:

	General-purpose function called to send a request to the Pdo.
	The Ioctl argument accepts the control method being passed down
	by the calling function.

Arguments:
	Pdo             - the request is sent to this device object
	Ioctl           - the request - specified by the calling function
	InputBuffer     - incoming request
	InputSize       - size of the incoming request
	OutputBuffer    - the answer
	OutputSize      - size of the answer buffer

Return Value:
	NT Status of the operation
*/
{
	IO_STATUS_BLOCK     ioBlock;
	KEVENT              myIoctlEvent;
	NTSTATUS            status;
	PIRP                irp;

	// Initialize an event to wait on
	KeInitializeEvent(&myIoctlEvent, SynchronizationEvent, FALSE);

	// Build the request
	irp = IoBuildDeviceIoControlRequest(
		Ioctl,
		Pdo,
		InputBuffer,
		InputSize,
		OutputBuffer,
		OutputSize,
		FALSE,
		&myIoctlEvent,
		&ioBlock);

	if (!irp) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Pass request to Pdo, always wait for completion routine

	status = IoCallDriver(Pdo, irp);

	//DebugPrint(("SendDownStreamIrp Result %08X\n", status));
	if (status == STATUS_PENDING) {
		// Wait for the IRP to be completed, then grab the real status code
		KeWaitForSingleObject(
			&myIoctlEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);

		status = ioBlock.Status;
	}

	return status;
}

NTSTATUS
OpenAcpiDevice(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp
	)
/*++

Routine Description:
	This routine processes the IOCTLs which read from the ports.

Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location
	IoctlCode   - The ioctl code from the IRP

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_INVALID_PARAMETER -- The buffer sent to the driver
								was too small to contain the
								port, or the buffer which
								would be sent back to the driver
								was not a multiple of the data size.

	STATUS_ACCESS_VIOLATION  -- An illegal port number was given.

--*/

{
	ANSI_STRING     pDevicePath;
	UNICODE_STRING  ulDevicePath;
	NTSTATUS        ntStatus;
	ACPI_NAME       *pInput;
	UINT            AcpiDeviceExtension;
	
	PAGED_CODE();

	DebugPrint(("HWACC: Open ACPI device..."));

	// Size of buffer containing data from application

	pInput = (ACPI_NAME *)pIrp->AssociatedIrp.SystemBuffer;

	RtlInitAnsiString(&pDevicePath, pInput->pAcpiDeviceName);

	ntStatus = RtlAnsiStringToUnicodeString(&ulDevicePath, &pDevicePath, TRUE);

	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}

	ntStatus = IoGetDeviceObjectPointer(
		&ulDevicePath,
		FILE_ALL_ACCESS,
		&pLDI->LowFileObject,
		&pLDI->LowDeviceObject
	);

	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}
	pLDI->dwMajorVersion = pInput->dwMajorVersion;
	pLDI->dwMinorVersion = pInput->dwMinorVersion;
	pLDI->dwBuildNumber = pInput->dwBuildNumber;
	pLDI->dwPlatformId = pInput->dwPlatformId;
	// point the Acpi_Hal Device Extension for data retrieve
	AcpiDeviceExtension = (UINT)(pLDI->LowDeviceObject)->DeviceExtension;

	// seaching for the ACPI data objects, insert the notification code....
	// Not test on Win7 may failed on old win10
	// Check the device extension
	// KdBreakPoint();
	if (pInput->dwMajorVersion >= 6) {
		//pLDI->AcpiObject = (PVOID)(AcpiDeviceExtension + pLDI->uAcpiOffset);
		// new way to get acpi acpi namespace is only verifed on windows 10 after 2016
#ifndef _TINY_DRIVER_
		if (!(GetAcpiObjectBase(pLDI, AcpiDeviceExtension) == STATUS_SUCCESS))
		{
			return STATUS_NOT_SUPPORTED;
		}
#endif
	}
	else {
		return STATUS_NOT_SUPPORTED;
	}

	DebugPrint(("HWACC: Open ACPI device done!"));

	return ntStatus;
}

#ifndef _TINY_DRIVER_
NTSTATUS
GetAcpiObjectBase(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in UINTN address
	)
/*++

Routine Description:
	Find the Acpi Object Name space

Arguments:

	pLDI        - our local device data
	address		- addres of receive number of total objs

Return Value:

	STATUS_SUCCESS	-- Found base
	others			-- Reach the depth, report for new os version support

--*/
{
	UNREFERENCED_PARAMETER(address);
	UINTN* pAddress = (UINTN*)address;
	unsigned char *  pTest;	
	unsigned char test;
	UINTN* pBackTrace;
	OS_DATA * pOsData;
	ACPI_OBJ* pAcpiObj;
	int Depth;
	for (Depth = 0; Depth < SEARCH_DEPTH; Depth++) {
		__try {
			pTest = (unsigned char*) pAddress[Depth];
			// do a valid memory test here, and to avoid to be optimized
			RtlCopyMemory(&test, pTest, 1);
			// no memory access issue, then get the valid data
			pOsData = CORD_FROM(pTest, obj, OS_DATA);
			
			if (VALID_OS_DATA(pOsData)) {	// or, it's a back trace for addressing???
				
				pAcpiObj = (ACPI_OBJ *)pTest;
				// Goback to the root.
				while (pAcpiObj->pnsParent != NULL) {
					pAcpiObj = pAcpiObj->pnsParent;
				}
				if (pAcpiObj->dwNameSeg != '___\\') {
					// it's not a valid ACPI Root					
					continue;
				}
				pLDI->pAcpiScope = (PVOID)(pAddress + Depth);
				pLDI->AcpiObject = (PVOID)pAcpiObj;
				return STATUS_SUCCESS;
			}
			// check if it's HNSO object of acpi...
			pBackTrace = (UINTN*)(*(UINTN*)pAddress[Depth]);
			RtlCopyMemory(&test, (pBackTrace), 1);
			pOsData = CORD_FROM(pBackTrace, obj, OS_DATA);   			
			if (VALID_OS_DATA(pOsData)) {	
				//it's a valid data object of OS ACPI Object
				pAcpiObj = (ACPI_OBJ*)pBackTrace;
				
				while (pAcpiObj->pnsParent != NULL) {
					pAcpiObj = pAcpiObj->pnsParent;
				}

				if (pAcpiObj->dwNameSeg != '___\\') {
					// it's not a valid ACPI Root					
					continue;
				}
				
				pLDI->pAcpiScope = (PVOID)(*(UINTN *)(pAddress+Depth));
				pLDI->AcpiObject = (PVOID)pAcpiObj;
				
				return STATUS_SUCCESS;
				//break;
			}
		} 
		__except (EXCEPTION_EXECUTE_HANDLER) {
			// avoid invalid memory access cause system crash
			// KdBreakPoint();
			continue;
		}
	}

	if (Depth >= SEARCH_DEPTH) {
		// not found
		return STATUS_INVALID_PARAMETER;
	}
	return STATUS_SUCCESS;
}

void
GetChildNamespace(
	__in ACPI_OBJ *pParent,
	__inout ULONG *puCount,
	__inout ULONG *puSize
	)
/*++

Routine Description:
	Build the USER ACPI Objects for UI

Arguments:

	pParent     - kernel acpi object parent
	puCount     - addres of receive number of total objs
	puSize		- addres of receive total size of internal data

Return Value:

	N/A

--*/
{
	BOOLEAN	  bOneSubOnly = FALSE;
	ACPI_OBJ* pLastChild;
	ACPI_OBJ* pFirstChild;
	__try {
		if (pParent->pnsFirstChild == NULL) {
			return;
		}		
		pFirstChild = pParent->pnsFirstChild;
		pLastChild = pParent->pnsLastChild;
		if (pLastChild == pFirstChild) {
			// only one child, break
			bOneSubOnly = TRUE;
		}		
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		
		return;
	}
	
	while(TRUE) {
		__try {
			if (pFirstChild->pnsFirstChild != NULL &&
				pFirstChild->pnsFirstChild != (ACPI_OBJ*)&pFirstChild->pnsFirstChild)
			{
				GetChildNamespace(pFirstChild, puCount, puSize);
			}

			// add the count of this child
			if (puCount != NULL) {
				(*puCount)++;
			}

			// get the size of data
			if (puSize != NULL) {
				(*puSize) += pFirstChild->ObjData.dwDataLen;
			}
			if (bOneSubOnly) {
				break;
			}
			
			if (pFirstChild == pLastChild) {
				// Finish handle the last one
				break;
			}

			pFirstChild = (ACPI_OBJ*)pFirstChild->list.plistNext;			
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			
			break;
		}
	} 
	return;
}

NTSTATUS
BuildUserAcpiObjects(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

	Build the USER ACPI Objects for UI

Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_INVALID_PARAMETER -- Incorrect parameter

--*/
{
	ULONG		OutBufferSize = 0;
	ULONG		uObjs = 0;
	ULONG		uCount = 0;
	ULONG		uSize = 0;
	ACPI_OBJ*	pAcpiObject = (ACPI_OBJ *)pLDI->AcpiObject;

	GetChildNamespace(pAcpiObject, &uCount, &uSize);
	OutBufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	uCount += 1;	// for the root
	uObjs = uCount * sizeof(ACPI_NAMESPACE);
	if (OutBufferSize < uObjs) {
		
		if (OutBufferSize >= sizeof(UINT)) {
			*(UINT*)(pIrp->AssociatedIrp.SystemBuffer) = uObjs;
			pIrp->IoStatus.Information = sizeof(UINT);			
		}
		else {
			pIrp->IoStatus.Information = 0;			
			return STATUS_BUFFER_TOO_SMALL;
		}
	}
	else {
		ACPI_NAMESPACE* pBuffer;
		ACPI_NAMESPACE* pEnd;
		ACPI_NAMESPACE* pUserStart;
		ACPI_NAMESPACE** pNext;
		pBuffer = (ACPI_NAMESPACE*)pIrp->AssociatedIrp.SystemBuffer;
		
		RtlZeroMemory(pBuffer, OutBufferSize);
		// Initialize the parent
		//ASSIGN_ACPI_OBJ(pBuffer, pAcpiObject);
		// data field
		ASSIGN_ACPI_OBJ(pBuffer, pAcpiObject);
		// assign the next avaiable value
		pUserStart = pBuffer;
		
		pNext = &pUserStart;		
		pEnd = pBuffer + OutBufferSize /sizeof(ACPI_NAMESPACE) + 1;
		(*pNext)++;
		uSize = BuildUserAcpiNameObjs(pAcpiObject, pBuffer, pEnd, pNext);
		uSize += 1;// for the root
		pIrp->IoStatus.Information = sizeof(ACPI_NAMESPACE) * uSize;
		
	}
	return STATUS_SUCCESS;
}

ULONG
BuildUserAcpiNameObjs(
	__in ACPI_OBJ* pParent,
	__in ACPI_NAMESPACE* pUserParent,
	__in ACPI_NAMESPACE* pEnd,
	__in ACPI_NAMESPACE** pNextObj
)
/*++

Routine Description:

	Build the USER ACPI Objects for UI

Arguments:

	pParent     - kernel acpi object parent
	pUserParent - user acpi object parent
	pEnd		- end of memory space
	pNextObj	- next avaiable memory for user acpi objects

Return Value:

	ULONG		- size of user acpi objects

--*/
{	
	UNREFERENCED_PARAMETER(pUserParent);
	BOOLEAN	  bOneSubOnly = FALSE;
	ACPI_OBJ* pLastChild;
	ACPI_OBJ* pFirstChild;
	ACPI_NAMESPACE* pUser = *pNextObj;
	ULONG			nSize = 0;
	PAGED_CODE();
	
	__try {
		if (pParent->pnsFirstChild == NULL) {
			return 0;
		}
		pFirstChild = pParent->pnsFirstChild;
		pLastChild = pParent->pnsLastChild;
		if (pLastChild == pFirstChild) {
			// only one child, break
			bOneSubOnly = TRUE;
		}		
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		// Debug try
		
		return 0;
	}

	while (TRUE) {
		__try {
			// got the chid item and assign it in user space
			ASSIGN_ACPI_OBJ(pUser, pFirstChild);
			(*pNextObj)++;	// point to next userable
			if ((*pNextObj) >= pEnd) {
				
				return nSize;
			}
			nSize++;
			if (pFirstChild->pnsFirstChild != NULL &&
				pFirstChild->pnsFirstChild != (ACPI_OBJ*)&pFirstChild->pnsFirstChild)
			{			
				//	GetChildNamespace(pFirstChild, puCount, puSize);
				nSize += BuildUserAcpiNameObjs(pFirstChild, pUser, pEnd, pNextObj);
			}
			if (bOneSubOnly) {
				break;
			}
			if (pLastChild == pFirstChild) {
				// Last child done
				break;
			}
			pFirstChild = (ACPI_OBJ*)pFirstChild->list.plistNext;
			pUser = *pNextObj;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			KdBreakPoint();
			break;
		}
	} 
	return nSize;
}


NTSTATUS
EvalAcpiWithoutInputInternal(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in ACPI_OBJ			*pParent,
	__in UCHAR				*puNameSeg,
	__in PVOID				pBuffer,
	__in ULONG				*uBufSize 
)

/*++

Routine Description:
	Eval ACPI data or method without input paramter

Arguments:

	pLDI			- our local device data
	pParent			- Acpi Namespace scope
	puNameSeg		- Not used
	pBuffer			- Received buffer
	uBufSize		- Size of received buffer

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_INVALID_PARAMETER -- The buffer sent to the driver
								was too small to contain the
								port, or the buffer which
								would be sent back to the driver
								was not a multiple of the data size.

--*/
{
	PACPI_EVAL_INPUT_BUFFER_EX  pMethodWithoutInputEx;
	NTSTATUS                    status;
	ACPI_OBJ					*pAcpiRoot;
	
	pMethodWithoutInputEx =
		(ACPI_EVAL_INPUT_BUFFER_EX*)ExAllocatePoolWithTag(NonPagedPool, sizeof(ACPI_EVAL_INPUT_BUFFER_EX), MY_TAG);

	pMethodWithoutInputEx->Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE_EX;
	strcpy_s(pMethodWithoutInputEx->MethodName, 255, (char *) puNameSeg);

	GET_ACPI_OBJS_ROOT(pAcpiRoot);
	if (pAcpiRoot != NULL) {
		SET_ACPI_OBJS_ROOT(pParent);
	}

	DebugPrint(("HWACC: Eval without input method:"));
	DebugPrint((pMethodWithoutInputEx->MethodName));

	status = SendDownStreamIrp(
		pLDI->LowDeviceObject,
		IOCTL_ACPI_EVAL_METHOD_EX,
		pMethodWithoutInputEx,
		sizeof(ACPI_EVAL_INPUT_BUFFER_EX),
		pBuffer,
		*uBufSize
	);
	SET_ACPI_OBJS_ROOT(pAcpiRoot);
	return status;
}

NTSTATUS
EvalAcpiWithoutInput(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

	Eval ACPI data or method without input paramter

Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_INVALID_PARAMETER -- The buffer sent to the driver
								was too small to contain the
								port, or the buffer which
								would be sent back to the driver
								was not a multiple of the data size.

--*/
{
	

	NTSTATUS                    status;
	PACPI_NAMESPACE             pAcpiNameSpace;
	//ACPI_EVAL_OUTPUT_BUFFER		acpi_eval_out;
	ULONG						nSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER);// sizeof(acpi_eval_out);
	UCHAR						uName[5];

	PAGED_CODE();
	
	if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PACPI_NAMESPACE)) {
		return STATUS_INVALID_PARAMETER_2;
	}

	pAcpiNameSpace = (PACPI_NAMESPACE)pIrp->AssociatedIrp.SystemBuffer;
	nSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	if (pAcpiNameSpace == NULL) {
		return STATUS_INVALID_PARAMETER_1;
	}	
	uName[0] = pAcpiNameSpace->MethodName[0];
	uName[1] = pAcpiNameSpace->MethodName[1];
	uName[2] = pAcpiNameSpace->MethodName[2];
	uName[3] = pAcpiNameSpace->MethodName[3];	
	uName[4] = 0;
	//if (pAcpiNameSpace->pKernelAddr == pLDI->pSetupAml) {
	//	//Debug for Aml Notification
	//	//KdBreakPoint();		
	//}
	status = EvalAcpiWithoutInputInternal(pLDI, (ACPI_OBJ*)pAcpiNameSpace->pParentNameSpace, uName, pAcpiNameSpace, &nSize);	
	pIrp->IoStatus.Information = nSize;
	return status;

}

NTSTATUS
ReadAcpiMemory(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

	Read acpi memory to user space for asl parser, or data simulation

Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_ACCESS_VIOLATION  -- Target memory is not accessable

--*/
{
	UINTN*	pInput;
	PVOID   pOutput;
	ULONG   uLong;
	UNREFERENCED_PARAMETER(pIrp);
	UNREFERENCED_PARAMETER(pLDI);
	PAGED_CODE();
	pInput =  (UINTN*)pIrp->AssociatedIrp.SystemBuffer;
	pOutput = (UCHAR*)pIrp->AssociatedIrp.SystemBuffer;
	uLong = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	try 
	{		
		RtlCopyMemory(pOutput, (PVOID)(*pInput), uLong);
	} 
	except(EXCEPTION_EXECUTE_HANDLER) 
	{
		// Failed to access acpi memory
		DebugPrint(("Exception, When Copy Memory %d!\n", uLong));
		pIrp->IoStatus.Information = 0;
		return STATUS_ACCESS_VIOLATION;
	}

	pIrp->IoStatus.Information = uLong;

	return STATUS_SUCCESS;
}

NTSTATUS
EvalAcpiWithInput(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

	Eval ACPI data or method without input paramter

Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_INVALID_PARAMETER -- The buffer sent to the driver
								was too small to contain the
								port, or the buffer which
								would be sent back to the driver
								was not a multiple of the data size.
	others					 -- Result from Lower Driver

--*/
{
	NTSTATUS                    status;
	PACPI_NAMESPACE             pAcpiNameSpace;
	ACPI_EVAL_OUTPUT_BUFFER		acpi_eval_out;
	ACPI_METHOD_ARG_COMPLEX* pInput;
	ULONG						nSize = sizeof(acpi_eval_out);
	UCHAR						uName[4];

	PAGED_CODE();

	if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ACPI_METHOD_ARG_COMPLEX)) {
		return STATUS_INVALID_PARAMETER_2;
	}
	pInput = (ACPI_METHOD_ARG_COMPLEX *)pIrp->AssociatedIrp.SystemBuffer;
	// Get the name space
	pAcpiNameSpace = &pInput->NameSpace;
	nSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	if (pAcpiNameSpace == NULL) {
		return STATUS_INVALID_PARAMETER_1;
	}

	uName[0] = pAcpiNameSpace->MethodName[0];
	uName[1] = pAcpiNameSpace->MethodName[1];
	uName[2] = pAcpiNameSpace->MethodName[2];
	uName[3] = pAcpiNameSpace->MethodName[3];

	if (pInput->InputBufferComplex.Signature == ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX) {
		status = EvalAcpiWithInputInternalEx(
					pLDI, 
			        (ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX*) &pInput->InputBufferComplex,
					(ACPI_OBJ*)pAcpiNameSpace->pParentNameSpace, 
					uName, 
					pInput, 
					&nSize);
	}
	else {
		status = EvalAcpiWithInputInternal(
					pLDI, 
					&pInput->InputBufferComplex, 
					(ACPI_OBJ*)pAcpiNameSpace->pParentNameSpace, 
					uName, 
					pInput, 
					&nSize);
	}

	pIrp->IoStatus.Information = nSize;

	return status;

}

#define COMPLEX_SIZE_ADJ ((UINTN) &(((PACPI_EVAL_INPUT_BUFFER_COMPLEX) 0)->Size) - (UINTN) &(((PACPI_EVAL_INPUT_BUFFER_COMPLEX) 0)->Signature))

NTSTATUS
EvalAcpiWithInputInternal(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in ACPI_EVAL_INPUT_BUFFER_COMPLEX* pInputBuffer,
	__in ACPI_OBJ* pParent,
	__in UCHAR* puNameSeg,
	__in PVOID	pBuffer,
	__in ULONG* uBufSize
)

/*++

Routine Description:
	Eval ACPI data or method without input paramter

Arguments:

	pLDI			- our local device data
	pInputBuffer    - Method Input
	pParent			- Acpi Namespace scope
	puNameSeg		- Not used
	pBuffer			- Received buffer
	uBufSize		- Size of received buffer

Return Value:
	STATUS_SUCCESS      -- OK

	others				-- Result from Lower Driver

--*/
{
	//	UNREFERENCED_PARAMETER(pParent);
	NTSTATUS                    status;
	ACPI_OBJ* pAcpiRoot;

	ULONG nSize = pInputBuffer->Size;
	pInputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
	pInputBuffer->MethodName[0] = puNameSeg[0];
	pInputBuffer->MethodName[1] = puNameSeg[1];
	pInputBuffer->MethodName[2] = puNameSeg[2];
	pInputBuffer->MethodName[3] = puNameSeg[3];
	pInputBuffer->Size = pInputBuffer->Size - (ULONG)COMPLEX_SIZE_ADJ;

	//UNREFERENCED_PARAMETER(pParent);
	GET_ACPI_OBJS_ROOT(pAcpiRoot);
	if (pAcpiRoot != NULL) {
		SET_ACPI_OBJS_ROOT(pParent);
	}

	DebugPrint(("HWACC: Eval with input method:"));
	DebugPrint((pMethodWithoutInputEx->MethodName));

	status = SendDownStreamIrp(
		pLDI->LowDeviceObject,
		IOCTL_ACPI_EVAL_METHOD,
		pInputBuffer,
		nSize,
		pBuffer,
		*uBufSize
	);
	SET_ACPI_OBJS_ROOT(pAcpiRoot);
	return status;
}

NTSTATUS
EvalAcpiWithInputInternalEx(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX* pInputBuffer,
	__in ACPI_OBJ* pParent,
	__in UCHAR* puNameSeg,
	__in PVOID	pBuffer,
	__in ULONG* uBufSize
)

/*++

Routine Description:
	Eval ACPI data or method without input paramter

Arguments:

	pLDI			- our local device data
	pInputBuffer    - Method Input
	pParent			- Acpi Namespace scope
	puNameSeg		- Not used
	pBuffer			- Received buffer
	uBufSize		- Size of received buffer

Return Value:
	STATUS_SUCCESS      -- OK

	others				-- Result from Lower Driver

--*/
{
	UNREFERENCED_PARAMETER(puNameSeg);
	NTSTATUS    status;
	ACPI_OBJ	*pAcpiRoot;
	
	ULONG nSize = pInputBuffer->Size;
	pInputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;

	GET_ACPI_OBJS_ROOT(pAcpiRoot);
	if (pAcpiRoot != NULL) {
		SET_ACPI_OBJS_ROOT(pParent);
	}

	DebugPrint(("HWACC: Eval with input EX method:"));
	DebugPrint((pInputBuffer->MethodName));

	status = SendDownStreamIrp(
		pLDI->LowDeviceObject,
		IOCTL_ACPI_EVAL_METHOD_EX,
		pInputBuffer,
		nSize,
		pBuffer,
		*uBufSize
	);
	SET_ACPI_OBJS_ROOT(pAcpiRoot);
	return status;
}

void
AddMyNotifyInAcpiNS(
	__in PLOCAL_DEVICE_INFO pLDI,
	ACPI_OBJ* pAcpiObj
)
/*++

Routine Description:
	Setup the ACPI Namespace link under \____ and just behind the first child

Arguments:

	pLDI        - our local device data
	pAcpiObj    - Acpi Namespace Object to insert

Return Value:

	NA

--*/
{
	//PAGED_CODE();
	ACPI_OBJ* pRoot = (ACPI_OBJ *)pLDI->AcpiObject;
	pAcpiObj->pnsParent = pRoot;
	pRoot = pRoot->pnsFirstChild;	// it's the first child, now insert
	pAcpiObj->list.plistNext = (LIST*)pRoot->list.plistNext;
	pAcpiObj->list.plistPrev = (LIST*)pRoot;
	pAcpiObj->pnsFirstChild = (ACPI_OBJ*)&pAcpiObj->pnsFirstChild;
	pAcpiObj->pnsLastChild = (ACPI_OBJ*)&pAcpiObj->pnsFirstChild;
	pRoot->list.plistNext->plistPrev = (LIST*)pAcpiObj;
	pRoot->list.plistNext = (LIST*)pAcpiObj;
}

PVOID
SetupAmlForNotifyInternal(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PAML_SETUP			pAmlSetup
)
/*++

Routine Description:

	Allocate the inserted AML code memory resource and setup the ACPI Namespace link.
	This will be dangrous if os changes the data format and will easily cause blue screen.

Arguments:

	pLDI        - our local device data
	pAmlSetup   - AML memory to setup

Return Value:

	PVOID       -- The buffer allocated for aml

--*/
{
	//PAGED_CODE();
	ACPI_OBJ* pAcpiObj;	

	if (pLDI->pSetupAml != NULL) {
		// memory already allocated then just put the code in...... 
		return pLDI->pSetupAml;
	}
	__try {
		pAcpiObj = ExAllocatePoolWithTag(NonPagedPool, (SIZE_T)pAmlSetup->dwSize + MYNT_OBJ_LEN, MYNT_TAG);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		//EnableInterrupt();		
		pAcpiObj = NULL;
		//return STATUS_ILLEGAL_INSTRUCTION;
	}
	if (pAcpiObj != NULL) {
		MYNT_ACPI_OBJ* pMyNtObj;
		RtlZeroMemory(pAcpiObj, pAmlSetup->dwSize + MYNT_OBJ_LEN);
		pAcpiObj->ObjData.pbDataBuff = (PVOID)((UINT)(pAcpiObj) +MYNT_OBJ_LEN);
		pAcpiObj->ObjData.pbDataBuff[pAmlSetup->dwOffset] = 0x0;
		memset(&pAcpiObj->ObjData.pbDataBuff[pAmlSetup->dwOffset + 1], 0xA3, pAmlSetup->dwSize - pAmlSetup->dwOffset);
		pAcpiObj->ObjData.dwDataLen = pAmlSetup->dwSize;
		pAcpiObj->ObjData.dwDataType = ACPI_TYPE_METHOD;
		pAcpiObj->dwRefCount = 1;
		pMyNtObj = (MYNT_ACPI_OBJ*)pAcpiObj;
		pMyNtObj->pOnwer = pAcpiObj;
		//pAcpiObj->ObjData.
		pAcpiObj->ucName[0] = 'M';
		pAcpiObj->ucName[1] = 'Y';
		pAcpiObj->ucName[2] = 'N';
		pAcpiObj->ucName[3] = 'T';
		//// Insert the parent and heads
		////pAcpiName->SubNameSpace;;	// The header of first sub; get the header and insert of tail		
		AddMyNotifyInAcpiNS(pLDI,pAcpiObj);
	}

	return pAcpiObj;
}


NTSTATUS
RemoveAmlForNotify(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

	Remove inserted Aml Code 

Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location

Return Value:
	STATUS_SUCCESS           -- OK

--*/
{
	UNREFERENCED_PARAMETER(pLDI);
	UNREFERENCED_PARAMETER(pIrp);
	UNREFERENCED_PARAMETER(IrpStack);
	if (pLDI->pSetupAml != NULL) {
		RemoveAcpiNS((PACPI_NAME_NODE)pLDI->pSetupAml);
		ExFreePoolWithTag(pLDI->pSetupAml, MYNT_TAG);
		pLDI->pSetupAml = NULL;
	}
	return STATUS_SUCCESS;
}

NTSTATUS
SetupAmlForNotify(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

	Setup the AML code for notify, wait for code setup in runtime
	Method (MYNT, 0x0, NotSerialized) {
             Noop ();
         }
	May need to adjust on different OS version.
	
Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_INVALID_PARAMETER -- The buffer sent to the driver
								was too small to contain the
								port, or the buffer which
								would be sent back to the driver
								was not a multiple of the data size.

	STATUS_BUFFER_OVERFLOW  --  Failed to allocate memory for Notify Aml Code

--*/
{
	UNREFERENCED_PARAMETER(pLDI);
	UNREFERENCED_PARAMETER(pIrp);
	UNREFERENCED_PARAMETER(IrpStack);
	PAML_SETUP pAmlCode;

	if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(AML_SETUP)) {
		return STATUS_INVALID_PARAMETER;
	}
	pAmlCode = (PAML_SETUP)pIrp->AssociatedIrp.SystemBuffer;
	if (pLDI->pSetupAml != NULL) {
		ACPI_OBJ* pAcpiObj = (ACPI_OBJ *)pLDI->pSetupAml;
		memset(&pAcpiObj->ObjData.pbDataBuff[pAmlCode->dwOffset + 1], 0xA3, pAmlCode->dwSize - pAmlCode->dwOffset);
		// a notify code #define NOTIFY_OP			0x86
		pAcpiObj->ObjData.pbDataBuff[pAmlCode->dwOffset + 1] = 0x86;	
		// setup the namestring for target device, processor or thermal
		RtlCopyMemory(&pAcpiObj->ObjData.pbDataBuff[pAmlCode->dwOffset + 2], pAmlCode->Name, pAmlCode->uNameSize);
		pAcpiObj->ObjData.pbDataBuff[pAmlCode->dwOffset + 2 + pAmlCode->uNameSize] = 0x0C;	// dword integer for notify code
		RtlCopyMemory(&pAcpiObj->ObjData.pbDataBuff[pAmlCode->dwOffset + 2  +pAmlCode->uNameSize + 1], &pAmlCode->ulCode, sizeof(ULONG));
		return STATUS_SUCCESS;
	}
	pLDI->pSetupAml = SetupAmlForNotifyInternal(pLDI,pAmlCode);
	if (pLDI->pSetupAml == NULL) {
		return STATUS_BUFFER_OVERFLOW;
	}
	return STATUS_SUCCESS;	
}
#endif // #ifndef _TINY_DRIVER_

NTSTATUS
EvalAcpiWithoutInputDirect(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

	Eval ACPI data or method without input paramter

Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_INVALID_PARAMETER -- The buffer sent to the driver
								was too small to contain the
								port, or the buffer which
								would be sent back to the driver
								was not a multiple of the data size.

--*/
{


	NTSTATUS                    status;
	PACPI_EVAL_INPUT_BUFFER_EX  pInput;
	ULONG						nSize = 0;// sizeof(acpi_eval_out);

	PAGED_CODE();

	DebugPrint(("HWACC: Eval without input direct..."));

	if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PACPI_EVAL_INPUT_BUFFER_EX)) {
		return STATUS_INVALID_PARAMETER_2;
	}

	pInput = (PACPI_EVAL_INPUT_BUFFER_EX) pIrp->AssociatedIrp.SystemBuffer;
	nSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	if (pInput == NULL) {
		return STATUS_INVALID_PARAMETER_1;
	}

	pInput->Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE_EX;

	status = SendDownStreamIrp(
		pLDI->LowDeviceObject,
		IOCTL_ACPI_EVAL_METHOD_EX,
		pInput,
		sizeof(ACPI_EVAL_INPUT_BUFFER_EX),
		pInput,
		nSize
	);

	pIrp->IoStatus.Information = nSize;

	DebugPrint(("HWACC: Eval without input direct done!"));

	return status;

}

NTSTATUS
EvalAcpiWithInputDirect(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

	Eval ACPI data or method without input paramter

Arguments:

	pLDI        - our local device data
	pIrp        - IO request packet
	IrpStack    - The current stack location

Return Value:
	STATUS_SUCCESS           -- OK

	STATUS_INVALID_PARAMETER -- The buffer sent to the driver
								was too small to contain the
								port, or the buffer which
								would be sent back to the driver
								was not a multiple of the data size.
	others					 -- Result from Lower Driver

--*/
{
	NTSTATUS                    status;
	ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX *pInput;
	ULONG						nSize = 0;// sizeof(acpi_eval_out);


	PAGED_CODE();

	DebugPrint(("HWACC: Eval with input direct..."));

	if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)) {
		return STATUS_INVALID_PARAMETER_2;
	}
	pInput = (ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX *) pIrp->AssociatedIrp.SystemBuffer;

	nSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	//ULONG nSize = pInput->Size;
	pInput->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;

	status = SendDownStreamIrp(
		pLDI->LowDeviceObject,
		IOCTL_ACPI_EVAL_METHOD_EX,
		pInput,
		pInput->Size,
		pInput,
		nSize
	);

	pIrp->IoStatus.Information = nSize;

	DebugPrint(("HWACC: Eval with input direct done"));

	return status;

}
