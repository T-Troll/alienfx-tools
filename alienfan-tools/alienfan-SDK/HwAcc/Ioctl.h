/*++

2006-2020  NickelS

Module Name:

    ioctl.h

Abstract:

    Windows ACPI Obj related definition, IOCTL interface

Environment:

    Kernel mode only.

--*/
#pragma once
#ifndef __HWACC_IOCTL_H__
#define __HWACC_IOCTL_H__
//
// Device type           -- in the "User Defined" range."
//

#pragma warning(disable:4201)
#pragma warning(disable:4214)

#include <acpiioct.h>
#ifndef UINTN
#ifdef AMD64
#define UINTN UINT64
#else
#define UINTN UINT32
#endif
#endif
#define ACPI_TYPE_SCOPE             0
#define ACPI_TYPE_INTEGER           1
#define ACPI_TYPE_STRING            2
#define ACPI_TYPE_BUFFER            3
#define ACPI_TYPE_PACKAGE           4
#define ACPI_TYPE_FIELDUNIT         5
#define ACPI_TYPE_DEVICE            6
#define ACPI_TYPE_SYNC_OBJECT       7
#define ACPI_TYPE_METHOD            8
#define ACPI_TYPE_MUTEX             9
#define ACPI_TYPE_OPERATION_REG     0xA
#define ACPI_TYPE_POWER_SOURCE      0xB
#define ACPI_TYPE_PROCESSOR         0xC
#define ACPI_TYPE_TMERMAL_ZONE      0xD
#define ACPI_TYPE_BUFFUNIT          0xE
#define ACPI_TYPE_DDBHANDLE         0xF
#define ACPI_TYPE_DEBUG             0x10
#define ACPI_TYPE_ALIAS             0x80
#define ACPI_TYPE_DATAALIAS         0x81
#define ACPI_TYPE_BANKFIELD         0x82
#define ACPI_TYPE_FIELD             0x83
#define ACPI_TYPE_INDEXFIELD        0x84
#define ACPI_TYPE_DATA              0x85
#define ACPI_TYPE_DATAFIELD         0x86
#define ACPI_TYPE_DATAOBJ           0x87
#define ACPI_TYPE_REV               0x88
#define ACPI_TYPE_CFIELD            0x89
#define ACPI_TYPE_EXTERN            0x8A
#define ACPI_TYPE_INVALID           0xFFFF
#define ACPI_SIGNATURE(a,b,c,d) (UINT32) ((UINT32) (a) | (((UINT32) (b)) << 8) | (((UINT32) (c)) << 16)| (((UINT32) (d)) << 24))
#define ACPI_PREDEFINED__GL_        ACPI_SIGNATURE('_', 'G', 'L','_')
#define ACPI_PREDEFINED__OSI        ACPI_SIGNATURE('_', 'O', 'S','I')
#define ACPI_PREDEFINED__OS_        ACPI_SIGNATURE('_', 'O', 'S','_')
#define ACPI_PREDEFINED__REV        ACPI_SIGNATURE('_', 'R', 'E','V')

#define ACPI_GPE_LXX        ACPI_SIGNATURE('_', 'L', 0,0)
#define ACPI_EC_QXX     ACPI_SIGNATURE('_', 'Q', 0,0)

#define ACPI_DEVICE__ADR        ACPI_SIGNATURE('_', 'A', 'D','R')
#define ACPI_DEVICE__CID        ACPI_SIGNATURE('_', 'C', 'I','D')
#define ACPI_DEVICE__DDN        ACPI_SIGNATURE('_', 'D', 'D','N')
#define ACPI_DEVICE__HID        ACPI_SIGNATURE('_', 'H', 'I','D')
#define ACPI_DEVICE__MLS        ACPI_SIGNATURE('_', 'M', 'L','S')
#define ACPI_DEVICE__PLD        ACPI_SIGNATURE('_', 'P', 'L','D')
#define ACPI_DEVICE__STR        ACPI_SIGNATURE('_', 'S', 'T','R')
#define ACPI_DEVICE__SUN        ACPI_SIGNATURE('_', 'S', 'U','N')
#define ACPI_DEVICE__UID        ACPI_SIGNATURE('_', 'U', 'I','D')

#define ACPI_DEVICE__CRS        ACPI_SIGNATURE('_', 'C', 'R','S')
#define ACPI_DEVICE__DIS        ACPI_SIGNATURE('_', 'D', 'I','S')
#define ACPI_DEVICE__DMA        ACPI_SIGNATURE('_', 'D', 'M','A')
#define ACPI_DEVICE__FIX        ACPI_SIGNATURE('_', 'F', 'I','X')
#define ACPI_DEVICE__GSB        ACPI_SIGNATURE('_', 'G', 'S','B')
#define ACPI_DEVICE__HPP        ACPI_SIGNATURE('_', 'H', 'P','P')
#define ACPI_DEVICE__HPX        ACPI_SIGNATURE('_', 'H', 'P','X')
#define ACPI_DEVICE__MAT        ACPI_SIGNATURE('_', 'M', 'A','T')
#define ACPI_DEVICE__OSC        ACPI_SIGNATURE('_', 'O', 'S','C')
#define ACPI_DEVICE__PRS        ACPI_SIGNATURE('_', 'P', 'R','S')
#define ACPI_DEVICE__PRT        ACPI_SIGNATURE('_', 'P', 'R','T')
#define ACPI_DEVICE__PXM        ACPI_SIGNATURE('_', 'P', 'X','M')
#define ACPI_DEVICE__SLI        ACPI_SIGNATURE('_', 'S', 'L','I')
#define ACPI_DEVICE__SRS        ACPI_SIGNATURE('_', 'S', 'R','S')


#define ACPI_DEVICE__EDL        ACPI_SIGNATURE('_', 'E', 'D','L')
#define ACPI_DEVICE__EJD        ACPI_SIGNATURE('_', 'E', 'J','D')
#define ACPI_DEVICE__EJ0        ACPI_SIGNATURE('_', 'E', 'J','0')
#define ACPI_DEVICE__EJ1        ACPI_SIGNATURE('_', 'E', 'J','1')
#define ACPI_DEVICE__EJ2        ACPI_SIGNATURE('_', 'E', 'J','2')
#define ACPI_DEVICE__EJ3        ACPI_SIGNATURE('_', 'E', 'J','3')
#define ACPI_DEVICE__EJ4        ACPI_SIGNATURE('_', 'E', 'J','4')
#define ACPI_DEVICE__EJ5        ACPI_SIGNATURE('_', 'E', 'J','5')
#define ACPI_DEVICE__LCK        ACPI_SIGNATURE('_', 'L', 'C','K')
#define ACPI_DEVICE__OST        ACPI_SIGNATURE('_', 'O', 'S','T')
#define ACPI_DEVICE__RMV        ACPI_SIGNATURE('_', 'R', 'M','V')
#define ACPI_DEVICE__STA        ACPI_SIGNATURE('_', 'S', 'T','A')

#define ACPI_DEVICE__INI        ACPI_SIGNATURE('_', 'I', 'N','I')
#define ACPI_DEVICE__DCK        ACPI_SIGNATURE('_', 'D', 'C','K')
#define ACPI_DEVICE__BDN        ACPI_SIGNATURE('_', 'B', 'D','N')
#define ACPI_DEVICE__REG        ACPI_SIGNATURE('_', 'R', 'E','G')
#define ACPI_DEVICE__BBN        ACPI_SIGNATURE('_', 'B', 'B','N')
#define ACPI_DEVICE__SEG        ACPI_SIGNATURE('_', 'S', 'E','G')
#define ACPI_DEVICE__GLK        ACPI_SIGNATURE('_', 'G', 'L','K')


#define ACPI_DEVICE__OFF        ACPI_SIGNATURE('_', 'O', 'F','F')
#define ACPI_DEVICE__ON_        ACPI_SIGNATURE('_', 'O', 'N','_')
#define ACPI_DEVICE__STA        ACPI_SIGNATURE('_', 'S', 'T','A')

#define ACPI_DEVICE__DSW        ACPI_SIGNATURE('_', 'D', 'S','W')
#define ACPI_DEVICE__PS0        ACPI_SIGNATURE('_', 'P', 'S','0')
#define ACPI_DEVICE__PS1        ACPI_SIGNATURE('_', 'P', 'S','1')
#define ACPI_DEVICE__PS2        ACPI_SIGNATURE('_', 'P', 'S','2')
#define ACPI_DEVICE__PS3        ACPI_SIGNATURE('_', 'P', 'S','3')

#define ACPI_DEVICE__PSC        ACPI_SIGNATURE('_', 'P', 'S','C')
#define ACPI_DEVICE__PR0        ACPI_SIGNATURE('_', 'P', 'R','0')
#define ACPI_DEVICE__PR1        ACPI_SIGNATURE('_', 'P', 'R','1')
#define ACPI_DEVICE__PR2        ACPI_SIGNATURE('_', 'P', 'R','2')

#define ACPI_DEVICE__PRW        ACPI_SIGNATURE('_', 'P', 'R','W')
#define ACPI_DEVICE__PSW        ACPI_SIGNATURE('_', 'P', 'S','W')

#define ACPI_DEVICE__IRC        ACPI_SIGNATURE('_', 'I', 'R','C')
#define ACPI_DEVICE__S1D        ACPI_SIGNATURE('_', 'S', '1','D')
#define ACPI_DEVICE__S2D        ACPI_SIGNATURE('_', 'S', '2','D')
#define ACPI_DEVICE__S3D        ACPI_SIGNATURE('_', 'S', '3','D')
#define ACPI_DEVICE__S4D        ACPI_SIGNATURE('_', 'S', '4','D')
#define ACPI_DEVICE__S0W        ACPI_SIGNATURE('_', 'S', '0','W')
#define ACPI_DEVICE__S1W        ACPI_SIGNATURE('_', 'S', '1','W')
#define ACPI_DEVICE__S2W        ACPI_SIGNATURE('_', 'S', '2','W')
#define ACPI_DEVICE__S3W        ACPI_SIGNATURE('_', 'S', '3','W')
#define ACPI_DEVICE__S4W        ACPI_SIGNATURE('_', 'S', '4','W')

#define ACPI_DEVICE__BFS        ACPI_SIGNATURE('_', 'B', 'F','S')
#define ACPI_DEVICE__PTS        ACPI_SIGNATURE('_', 'P', 'T','S')
#define ACPI_DEVICE__S0_        ACPI_SIGNATURE('_', 'S', '0','_')
#define ACPI_DEVICE__S1_        ACPI_SIGNATURE('_', 'S', '1','_')
#define ACPI_DEVICE__S2_        ACPI_SIGNATURE('_', 'S', '2','_')
#define ACPI_DEVICE__S3_        ACPI_SIGNATURE('_', 'S', '3','_')
#define ACPI_DEVICE__S4_        ACPI_SIGNATURE('_', 'S', '4','_')
#define ACPI_DEVICE__S5_        ACPI_SIGNATURE('_', 'S', '5','_')

#define ACPI_DEVICE__SWS        ACPI_SIGNATURE('_', 'S', 'W','S')
#define ACPI_DEVICE__TTS        ACPI_SIGNATURE('_', 'T', 'T','S')
#define ACPI_DEVICE__WAK        ACPI_SIGNATURE('_', 'W', 'A','K')

#define ACPI_DEVICE__PDC        ACPI_SIGNATURE('_', 'P', 'D','C')
#define ACPI_DEVICE__SST        ACPI_SIGNATURE('_', 'S', 'S','T')
#define ACPI_DEVICE__MSG        ACPI_SIGNATURE('_', 'M', 'S','G')
#define ACPI_DEVICE__BLT        ACPI_SIGNATURE('_', 'B', 'L','T')

#define ACPI_DEVICE__ALI        ACPI_SIGNATURE('_', 'A', 'L','I')
#define ACPI_DEVICE__ALT        ACPI_SIGNATURE('_', 'A', 'L','T')
#define ACPI_DEVICE__ALC        ACPI_SIGNATURE('_', 'A', 'L','C')
#define ACPI_DEVICE__ALR        ACPI_SIGNATURE('_', 'A', 'L','R')
#define ACPI_DEVICE__ALP        ACPI_SIGNATURE('_', 'A', 'L','P')

#define ACPI_DEVICE__LID        ACPI_SIGNATURE('_', 'L','I', 'D')
#define ACPI_DEVICE__FDE        ACPI_SIGNATURE('_', 'F', 'D','E')
#define ACPI_DEVICE__FDI        ACPI_SIGNATURE('_', 'F', 'D','I')
#define ACPI_DEVICE__FDM        ACPI_SIGNATURE('_', 'F', 'D','M')

#define ACPI_DEVICE__UPC        ACPI_SIGNATURE('_', 'U', 'P','C')
#define ACPI_DEVICE__PLD        ACPI_SIGNATURE('_', 'P', 'L','D')

#define ACPI_DEVICE__DSM        ACPI_SIGNATURE('_', 'D', 'S','M')

#define ACPI_DEVICE__UPD        ACPI_SIGNATURE('_', 'U', 'P','D')
#define ACPI_DEVICE__UPP        ACPI_SIGNATURE('_', 'U', 'P','P')

#define ACPI_DEVICE__PSR        ACPI_SIGNATURE('_', 'P', 'S','R')
#define ACPI_DEVICE__PCL        ACPI_SIGNATURE('_', 'P', 'C','L')


#define ACPI_THERMAL__AC0       ACPI_SIGNATURE('_', 'A', 'C','0')
#define ACPI_THERMAL__AC1       ACPI_SIGNATURE('_', 'A', 'C','1')
#define ACPI_THERMAL__AC2       ACPI_SIGNATURE('_', 'A', 'C','2')
#define ACPI_THERMAL__AC3       ACPI_SIGNATURE('_', 'A', 'C','3')
#define ACPI_THERMAL__AC4       ACPI_SIGNATURE('_', 'A', 'C','4')
#define ACPI_THERMAL__AC5       ACPI_SIGNATURE('_', 'A', 'C','5')
#define ACPI_THERMAL__AC6       ACPI_SIGNATURE('_', 'A', 'C','6')
#define ACPI_THERMAL__AC7       ACPI_SIGNATURE('_', 'A', 'C','7')
#define ACPI_THERMAL__AC8       ACPI_SIGNATURE('_', 'A', 'C','8')
#define ACPI_THERMAL__AC9       ACPI_SIGNATURE('_', 'A', 'C','9')
#define ACPI_THERMAL__AL0       ACPI_SIGNATURE('_', 'A', 'L','0')
#define ACPI_THERMAL__AL1       ACPI_SIGNATURE('_', 'A', 'L','1')
#define ACPI_THERMAL__AL2       ACPI_SIGNATURE('_', 'A', 'L','2')
#define ACPI_THERMAL__AL3       ACPI_SIGNATURE('_', 'A', 'L','3')
#define ACPI_THERMAL__AL4       ACPI_SIGNATURE('_', 'A', 'L','4')
#define ACPI_THERMAL__AL5       ACPI_SIGNATURE('_', 'A', 'L','5')
#define ACPI_THERMAL__AL6       ACPI_SIGNATURE('_', 'A', 'L','6')
#define ACPI_THERMAL__AL7       ACPI_SIGNATURE('_', 'A', 'L','7')
#define ACPI_THERMAL__AL8       ACPI_SIGNATURE('_', 'A', 'L','8')
#define ACPI_THERMAL__AL9       ACPI_SIGNATURE('_', 'A', 'L','9')
#define ACPI_THERMAL__CRT       ACPI_SIGNATURE('_', 'C', 'R','T')
#define ACPI_THERMAL__HOT       ACPI_SIGNATURE('_', 'H', 'O','T')
#define ACPI_THERMAL__PSL       ACPI_SIGNATURE('_', 'P', 'S','L')
#define ACPI_THERMAL__PSV       ACPI_SIGNATURE('_', 'P', 'S','V')
#define ACPI_THERMAL__SCP       ACPI_SIGNATURE('_', 'S', 'C','P')
#define ACPI_THERMAL__TC1       ACPI_SIGNATURE('_', 'T', 'C','1')
#define ACPI_THERMAL__TC2       ACPI_SIGNATURE('_', 'T', 'C','2')
#define ACPI_THERMAL__TMP       ACPI_SIGNATURE('_', 'T', 'M','P')
#define ACPI_THERMAL__TPT       ACPI_SIGNATURE('_', 'T', 'P','T')
#define ACPI_THERMAL__TRT       ACPI_SIGNATURE('_', 'T', 'R','T')
#define ACPI_THERMAL__TSP       ACPI_SIGNATURE('_', 'T', 'S','P')
#define ACPI_THERMAL__TST       ACPI_SIGNATURE('_', 'T', 'S','T')
#define ACPI_THERMAL__TZD       ACPI_SIGNATURE('_', 'T', 'Z','D')
#define ACPI_THERMAL__TZM       ACPI_SIGNATURE('_', 'T', 'Z','M')
#define ACPI_THERMAL__TZP       ACPI_SIGNATURE('_', 'T', 'Z','P')

#define ACPI_PROCESSOR__PSS  ACPI_SIGNATURE('_', 'P', 'S','S')
#define ACPI_PROCESSOR__PCT  ACPI_SIGNATURE('_', 'P', 'C','T')
#define ACPI_PROCESSOR__PDC  ACPI_SIGNATURE('_', 'P', 'D','C')
#define ACPI_PROCESSOR__PSD  ACPI_SIGNATURE('_', 'P', 'S','D')
#define ACPI_PROCESSOR__PPC  ACPI_SIGNATURE('_', 'P', 'P','C')
#define ACPI_PROCESSOR__CST  ACPI_SIGNATURE('_', 'C', 'S','T')
#define ACPI_PROCESSOR__CSD  ACPI_SIGNATURE('_', 'C', 'S','D')
#define ACPI_PROCESSOR__PTC  ACPI_SIGNATURE('_', 'P', 'T','C')
#define ACPI_PROCESSOR__TSS  ACPI_SIGNATURE('_', 'T', 'S','S')
#define ACPI_PROCESSOR__TPC  ACPI_SIGNATURE('_', 'T', 'P','C')
#define ACPI_PROCESSOR__TSD  ACPI_SIGNATURE('_', 'T', 'S','D')
#define ACPI_PROCESSOR__GTF  ACPI_SIGNATURE('_', 'G', 'T','F')
#define ACPI_PROCESSOR__GTM  ACPI_SIGNATURE('_', 'G', 'T','M')
#define ACPI_PROCESSOR__STM  ACPI_SIGNATURE('_', 'S', 'T','M')
#define ACPI_PROCESSOR__SDD  ACPI_SIGNATURE('_', 'S', 'D','D')
#define ACPI_PROCESSOR_MYNT  ACPI_SIGNATURE('M', 'Y', 'N','T')

#define HWACC_TYPE 40000

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.

#define IOCTL_GPD_READ_PORT_UCHAR \
    CTL_CODE( HWACC_TYPE, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_READ_PORT_USHORT \
    CTL_CODE( HWACC_TYPE, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_READ_PORT_ULONG \
    CTL_CODE( HWACC_TYPE, 0x902, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_WRITE_PORT_UCHAR \
    CTL_CODE(HWACC_TYPE,  0x910, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_GPD_WRITE_PORT_USHORT \
    CTL_CODE(HWACC_TYPE,  0x911, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_GPD_WRITE_PORT_ULONG \
    CTL_CODE(HWACC_TYPE,  0x912, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_GPD_READ_PCI \
    CTL_CODE(HWACC_TYPE,  0x913, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_GPD_WRITE_PCI \
    CTL_CODE(HWACC_TYPE,  0x914, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#define IOCTL_GPD_READ_MEM \
    CTL_CODE( HWACC_TYPE, 0x906, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_READ_MSR \
    CTL_CODE( HWACC_TYPE, 0x907, METHOD_BUFFERED, FILE_READ_ACCESS )


#define IOCTL_GPD_WRITE_MEM \
    CTL_CODE(HWACC_TYPE,  0x916, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_GPD_WRITE_MSR \
    CTL_CODE(HWACC_TYPE,  0x917, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_GPD_MAP_PHYSICAL \
    CTL_CODE( HWACC_TYPE, 0x920, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_UMMAP_PHYSICAL \
    CTL_CODE( HWACC_TYPE, 0x921, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_OPEN_ACPI \
    CTL_CODE( HWACC_TYPE, 0x940, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_EVAL_ACPI \
    CTL_CODE( HWACC_TYPE, 0x950, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_EVAL_ACPI_NO_RETURN \
    CTL_CODE( HWACC_TYPE, 0x951, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_EVAL_ACPI_WITH_PARAMETER \
    CTL_CODE( HWACC_TYPE, 0x952, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_EVAL_ACPI_WITHOUT_PARAMETER \
    CTL_CODE( HWACC_TYPE, 0x953, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_LOAD_AML \
    CTL_CODE( HWACC_TYPE, 0x954, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_UNLOAD_AML \
    CTL_CODE( HWACC_TYPE, 0x955, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_NOTIFY_DEVICE \
    CTL_CODE(HWACC_TYPE, 0x956, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_GPD_EVAL_ACPI_WITHOUT_DIRECT \
    CTL_CODE( HWACC_TYPE, 0x957, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_EVAL_ACPI_WITH_DIRECT \
    CTL_CODE( HWACC_TYPE, 0x958, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_GET_NEXT_ACPI_NODE \
    CTL_CODE( HWACC_TYPE, 0x960, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_READ_ACPI_MEMORY \
    CTL_CODE( HWACC_TYPE, 0x961, METHOD_BUFFERED, FILE_READ_ACCESS )
#define IOCTL_GPD_WRITE_ACPI_MEMORY \
        CTL_CODE( HWACC_TYPE, 0x962, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_ENUM_ACPI \
    CTL_CODE( HWACC_TYPE, 0x970, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_BUILD_ACPI \
    CTL_CODE( HWACC_TYPE, 0x970, METHOD_BUFFERED, FILE_READ_ACCESS )




//Data struct definition

typedef struct {
    PVOID   pAcpiDeviceName;
    ULONG   uAcpiDeviceNameLength;
    ULONG   dwMajorVersion;
    ULONG   dwMinorVersion;
    ULONG   dwBuildNumber;
    ULONG   dwPlatformId;
} ACPI_NAME;

typedef struct _ACPI_NAME_NODE {
    struct _ACPI_NAME_NODE* pPrev;
    struct _ACPI_NAME_NODE* pNext;
    struct _ACPI_NAME_NODE* pHeader;
    struct _ACPI_NAME_NODE* SubNameSpace;
    union {
        UINT32                  Name;
        CHAR                    chName[4];
    };
    PVOID                   Para1;
    PVOID                   PrevNodeWithNameType;
    UINT32                  Para3;
    UINT32                  Type;
    UINT32                  TypeExt;
    UINT32                  IntegerData;
    UINT32                  IntegerData1;
    UINT32                  Length;
    UINT8* Contain;
    UINT32                  Para8;
    UINT32                  Para9;
} ACPI_NAME_NODE, * PACPI_NAME_NODE;

typedef struct _ACPI_NODE {
    PVOID               CurrentNode;
    ACPI_NAME_NODE      Node;
    ULONG               Length;
    UINT8               RawData[1];
} ACPI_NODE, * PACPI_NODE;

typedef struct _ACPI_NAMESPACE {
    struct _ACPI_NAMESPACE* pParentNameSpace;
    struct _ACPI_NAMESPACE* pAddress;
    struct _ACPI_NAMESPACE* pKernelAddr;
    struct _ACPI_NAMESPACE* pParent;
    struct _ACPI_NAMESPACE* pChild;
    struct _ACPI_NAMESPACE* pNext;
    struct _ACPI_NAMESPACE* pPrev;
    void* hTreeItem;
    void* Contain;
    void* pUserContain;
    union {
        ULONG  MethodNameAsUlong;
        UCHAR  MethodName[4];
    };
    union {
		union {
			USHORT Flags;
			UCHAR  uFlags;
			struct {
				UCHAR	ArgCount : 3;
				UCHAR	SerializeFlag : 1;
				UCHAR	SyncLevel : 4;
			};
		};
        USHORT TypeExt;
    };
    
    USHORT Type;
    ULONG  Length;
    union {
        ULONG64   InegerData;
        struct {
            ULONG IntegerLow;
            ULONG IntegerHigh;
        };
    };
} ACPI_NAMESPACE, * PACPI_NAMESPACE;

typedef struct {
    ACPI_NAMESPACE* pAcpiNS;
    UINT            uCount;
    UINT            uLength;
} ACPI_NS_DATA, * PACPI_NS_DATA;

typedef struct {
    ULONG_PTR   Addr;
    ULONG       Length;
    UCHAR       Type;
    UCHAR       Rsvd[3];
} ACPI_OPERATION_REGION, * PACPI_OPERATION_REGION;

typedef struct {
    UINT32      ByteOffset;
    UINT32      BitOffset;
    UINT32      Width;
    UINT32      Type;
    PVOID       Parent;
} ACPI_FIELD, * PACPI_FIELD;

typedef struct {
    UINT32      ByteOffset;
    UINT32      BitOffset;
    UINT32      Width;
    UINT32      Type;
    PVOID       Parent;
    UINT32      Rsvd;
} ACPI_FIELD_UINT, * PACPI_FIELD_UINT;

typedef struct {
    UINT64      Type;
    UINT32      Rsvd1;
    UINT32      Rsvd2;
    UINT32      Length;
    PVOID       Buffer;
} ACPI_NODE_POINT, * PACPI_NODE_POINT;

typedef struct {
    PVOID   *pField;
    ULONG   Offset;
    ULONG   BitOffset;
    ULONG   NumOfBits;
    ULONG   Type;
} ACPI_FIELD_UNIT, * PACPI_FIELD_UNIT;

typedef struct {
    ULONG   PblkAddr;
    ULONG   PblkLen;
    ULONG   ProcID;
} ACPI_PROCESSOR, * PACPI_PROCESSOR;

typedef struct {
    PVOID*  pOperationReg;
    PVOID*  pField;
    ULONG   Bank;
    ULONG   Rsvd;
} ACPI_BANKFIELD, * PACPI_BANKFIELD;

typedef struct {
    PVOID* pField;
    ULONG   Length;
    ULONG   Offset;
    ULONG   BitOffset;
    ULONG   NumOfBits;
} ACPI_BUF_FIELD, * PACPI_BUF_FIELD;

typedef struct {
    //
    // 
    //
    UINT16  Type;
    UINT16  Length;
    union {
        struct {
            UINT32  dwData0;
            UINT32  dwData1;
        };
        UINT8   bData[1];
    };
} ACPI_RETURN_PACKAGE_DATA, * PACPI_RETURN_PACKAGE_DATA;

typedef struct {
    UINT16      Rsvd;
    UINT16      Type;
    PVOID       pData;
    union {
        UINT64  Data64;
        struct {
            UINT32  Data1;
            UINT32  Data2;
        };
    };
    UINT32      Data3;
    UINT32      Data4;
} ACPI_DATA, * PACPI_DATA;

typedef struct {
    UINT32      Length;
    UINT32      Rsvd;
    ACPI_DATA   Data[1];
} ACPI_PACKAGE_NODE, * PACPI_PACKAGE_NODE;


typedef struct _ACPI_MEMORY {
    PVOID       	pAcpiMemoryAddr;
    UINT32			Length;
    unsigned char   pAcpiMemoryBuffer[1];
} ACPI_MEMORY, * PACPI_MEMORY;

typedef struct _Acpi_ObjData OBJDATA, * POBJDATA, ** PPOBJDATA;
typedef struct _Acpi_Obj NSOBJ, * PNSOBJ, ** PPNSOBJ;


typedef struct _Acpi_ObjData
{
    USHORT        dwfData;              
    USHORT        dwDataType;           
    union
    {
        ULONG     dwRefCount;           
        POBJDATA  pdataBase;            
    };
    union
    {
        ULONG     dwDataValue;          
        ULONG64   qwDataValue;          
        PNSOBJ    pnsAlias;             
        POBJDATA  pdataAlias;           
        PVOID     powner;               
    };
    ULONG         dwDataLen;            
    PUCHAR        pbDataBuff;
} ACPI_OBJDATA, * PACPI_OBJDATA;

typedef struct _List
{
    struct _List* plistNext;
    struct _List* plistPrev;
} LIST, * PLIST, ** PPLIST;

typedef struct _Acpi_Obj
{
    LIST    list;                       
    PNSOBJ  pnsParent;
    PNSOBJ  pnsFirstChild;
    PNSOBJ  pnsLastChild;
    union
    {
        ULONG   dwNameSeg;
        UCHAR	ucName[4];
    };
    HANDLE  hOwner;
    PNSOBJ  pnsOwnedNext;
    OBJDATA ObjData;
    PVOID   Context;
    ULONG   dwRefCount;
}ACPI_OBJ, * PACPI_OBJ;

typedef struct 
{
    LIST    list;                       
    PNSOBJ  pnsParent;
    PNSOBJ  pnsFirstChild;
    PNSOBJ  pnsLastChild;
    union
    {
        ULONG   dwNameSeg;
        UCHAR	ucName[4];
    };
    HANDLE  hOwner;
    PNSOBJ  pnsOwnedNext;
    OBJDATA ObjData;
    PVOID   Context;
    ULONG   dwRefCount;
    PVOID   pOnwer;
    ULONG   dwRefCount1;
} MYNT_ACPI_OBJ, * PMYNT_ACPI_OBJ;

typedef struct _User_Acpi_Obj {
    ACPI_OBJ* kernelAddr;
    ACPI_OBJ* pnsParent;
    ACPI_OBJ* pnsFirstChild;
    ACPI_OBJ* pnsLastChild;
    HANDLE		handle[2];		// reserved for certain handler usage in user space
    LIST		list;
    ACPI_OBJ	obj;
}USER_ACPI_OBJ, * PUSER_ACPI_OBJ;

typedef struct _PackageObj
{
    ULONG   dwcElements;
    OBJDATA adata[ANYSIZE_ARRAY];
} PACKAGEOBJ, * PPACKAGEOBJ;

typedef struct _ACPI_METHOD_ARG_COMPLEX {
	ACPI_NAMESPACE                      NameSpace;
	ACPI_EVAL_INPUT_BUFFER_COMPLEX      InputBufferComplex;
} ACPI_METHOD_ARG_COMPLEX;

typedef struct _AML_SETUP_ {
    UINT32 dwSize;
    UINT32 dwOffset;
    ULONG  ulCode;
    ULONG  uNameSize;
    CHAR   Name[256];    //NULL terminated name string
}AML_SETUP, *PAML_SETUP;

typedef  ACPI_METHOD_ARG_COMPLEX*    PACPI_METHOD_ARG_COMPLEX;

#endif
