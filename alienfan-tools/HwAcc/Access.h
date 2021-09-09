/*++

2006-2020  NickelS

Module Name:

	Access.h

Abstract:

	Purpose of this driver is to demonstrate how the four different types
	of IOCTLs can be used, and how the I/O manager handles the user I/O
	buffers in each case. This sample also helps to understand the usage of
	some of the memory manager functions.

Environment:

	Kernel mode only.

--*/

#pragma once
#ifndef __ACCESS_H__
#define __ACCESS_H__

#pragma warning(disable:4616 4201)

#include "ioctl.h"
//
// Define the IOCTL codes we will use.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.
//

#ifndef UINTN
#ifdef AMD64
#define UINTN UINT64
#else
#define UINTN UINT32
#endif
#endif

/*** Type and Structure definitions
 */


#define ASSIGN_ACPI_OBJ(a,b) { \
	a->pUserContain = NULL; \
	a->Contain = b->ObjData.pbDataBuff;\
	a->InegerData = b->ObjData.qwDataValue;\
	a->Length = b->ObjData.dwDataLen;\
	a->Flags = b->ObjData.dwfData;\
	a->Type = b->ObjData.dwDataType;\
	a->MethodNameAsUlong = b->dwNameSeg;\
	a->pKernelAddr = (ACPI_NAMESPACE*)b;\
	a->pParent = (ACPI_NAMESPACE*)b->pnsParent;\
	a->pParentNameSpace = (ACPI_NAMESPACE*)b->pnsParent;\
	a->pAddress = (ACPI_NAMESPACE*)b;}

#define GET_ACPI_OBJS_ROOT(a)	a=(ACPI_OBJ *) (*((ACPI_OBJ **)(pLDI->pAcpiScope)));
#define SET_ACPI_OBJS_ROOT(a)   *((ACPI_OBJ **)(pLDI->pAcpiScope)) = a;

#define ASSIGN_ACPI_OBJ1(a,b) {memcpy(&(a->obj), b, sizeof(ACPI_OBJ)); \
								a->kernelAddr = b; \
								a->list = b->list; \
								a->pnsFirstChild = b->pnsFirstChild; \
								a->pnsLastChild = b->pnsLastChild; \
								a->pnsParent = b->pnsParent; }


typedef struct _OSData						// ACPI using following data to store the ACPI information
{
	ULONG	lSignature; // HNSO
	ULONG	Length;		// 0x000000b0, dump from os
	PVOID	pHeap;		// Heap address	' memory start with "HEAP" signature
	struct _Acpi_Obj obj;
} OS_DATA, *POS_DATA;

typedef struct {
	ACPI_ENUM_CHILDREN_INPUT_BUFFER input;
	CHAR        Padding[256];
} ACPI_ENUM_CHILDREN_INPUT_BUFFER_EX;


#define CORD_OF(field, name)	(UINT64) (&((name *)NULL)->field)
#define CORD_FROM(fieldaddr, field, name)  (name *)((PVOID)((UINT) (fieldaddr) - CORD_OF (field, name)))
#define VALID_OS_DATA(a)	((POS_DATA)(a))->lSignature == 0x4F534E48	// ('HNSO' "OSNH")
#define SEARCH_DEPTH 256

// Interface definition

NTSTATUS
OpenAcpiDevice(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp
	);

NTSTATUS
GetAcpiObjectBase(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in UINTN address
	);

NTSTATUS
DumpAcpiObj(
	__in PLOCAL_DEVICE_INFO pLDI
	);

NTSTATUS
BuildUserAcpiObjects(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
	);

void
GetChildNamespace(
	__in ACPI_OBJ* pParent,
	__inout ULONG* puCount,
	__inout ULONG* puSize
	);

ULONG
BuildUserAcpiNameObjs(
	__in ACPI_OBJ*			pParent,
	__in ACPI_NAMESPACE*	pUserParent,
	__in ACPI_NAMESPACE*	pEnd,
	__in ACPI_NAMESPACE**	pNextObj
);

NTSTATUS
EvalAcpiWithoutInputInternal(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in ACPI_OBJ* pParent,
	__in UCHAR* puNameSeg,
	__in PVOID				pBuffer,
	__in ULONG* uBufSize
);

NTSTATUS
EvalAcpiWithInputInternal(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in ACPI_EVAL_INPUT_BUFFER_COMPLEX* pInputBuffer,
	__in ACPI_OBJ* pParent,
	__in UCHAR* puNameSeg,
	__in PVOID				pBuffer,
	__in ULONG* uBufSize
);


NTSTATUS
EvalAcpiWithoutInput(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
);

NTSTATUS
ReadAcpiMemory(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
);

NTSTATUS
EvalAcpiWithInput(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
);

NTSTATUS
SetupAmlForNotify(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
); 

NTSTATUS
RemoveAmlForNotify(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
);

void
RemoveAcpiNS(
	__in PACPI_NAME_NODE	pAcpiNS
);

PVOID
SetupAmlForNotifyInternal(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PAML_SETUP			pAmlSetup
);

NTSTATUS
EvalAcpiWithInputInternalEx(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX* pInputBuffer,
	__in ACPI_OBJ* pParent,
	__in UCHAR* puNameSeg,
	__in PVOID	pBuffer,
	__in ULONG* uBufSize
);

NTSTATUS
EvalAcpiWithoutInputDirect(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
);

NTSTATUS
EvalAcpiWithInputDirect(
	__in PLOCAL_DEVICE_INFO pLDI,
	__in PIRP pIrp,
	__in PIO_STACK_LOCATION IrpStack
);

#endif