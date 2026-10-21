#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wchar.h>
typedef int32_t NTSTATUS;
typedef uint8_t UCHAR,BOOLEAN,*PUCHAR;
typedef uint16_t USHORT;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef void *PVOID,*HANDLE,*WDFDEVICE,*WDFKEY,*WDFSTRING,*WDFWAITLOCK;
typedef struct { int64_t QuadPart; } LARGE_INTEGER;
typedef struct { uint32_t a; uint16_t b,c; unsigned char d[8]; } GUID;
typedef const GUID* LPCGUID;
typedef struct { USHORT Length,MaximumLength; wchar_t* Buffer; } UNICODE_STRING;
typedef struct { UNICODE_STRING* name; } OBJECT_ATTRIBUTES;
typedef struct { uintptr_t Information; } IO_STATUS_BLOCK;
typedef struct { LARGE_INTEGER EndOfFile; } FILE_STANDARD_INFORMATION;
typedef struct { int unused; } SPB_CONTEXT;
typedef struct { volatile LONG ReportsSuppressed; } REPORT_CONTEXT;
typedef struct { int DevicePowerState; BOOLEAN ChipAsleep,FirmwareReady; WDFWAITLOCK ControllerLock; } FT5X_CONTROLLER_CONTEXT;
typedef struct { WDFDEVICE FxDevice; FT5X_CONTROLLER_CONTEXT* TouchContext;
 WDFWAITLOCK PowerTransitionLock; volatile LONG RuntimeD0,DisplayOn;
 SPB_CONTEXT I2CContext; REPORT_CONTEXT ReportContext; } DEVICE_EXTENSION,*PDEVICE_EXTENSION;
#define IN
#define _Use_decl_annotations_
#define TRUE 1
#define FALSE 0
#define STATUS_SUCCESS ((NTSTATUS)0)
#define STATUS_INVALID_PARAMETER ((NTSTATUS)0xc000000d)
#define STATUS_INVALID_DEVICE_STATE ((NTSTATUS)0xc0000184)
#define STATUS_INSUFFICIENT_RESOURCES ((NTSTATUS)0xc000009a)
#define STATUS_OBJECT_PATH_INVALID ((NTSTATUS)0xc0000039)
#define STATUS_INVALID_IMAGE_FORMAT ((NTSTATUS)0xc000007b)
#define STATUS_DEVICE_DATA_ERROR ((NTSTATUS)0xc000009c)
#define TEST_FAILURE ((NTSTATUS)0xc0000185)
#define NT_SUCCESS(s) ((NTSTATUS)(s)>=0)
#define TOUCH_POOL_TAG 0
#define NonPagedPool 0
#define WDF_NO_OBJECT_ATTRIBUTES NULL
#define PLUGPLAY_REGKEY_DRIVER 0
#define GENERIC_READ 1
#define SYNCHRONIZE 2
#define OBJ_CASE_INSENSITIVE 1
#define OBJ_KERNEL_HANDLE 2
#define FILE_ATTRIBUTE_NORMAL 0
#define FILE_SHARE_READ 1
#define FILE_OPEN 1
#define FILE_SYNCHRONOUS_IO_NONALERT 1
#define FILE_NON_DIRECTORY_FILE 2
#define FileStandardInformation 0
#define KernelMode 0
#define PASSIVE_LEVEL 0
#define PowerDeviceD0 1
#define PowerDeviceD3 4
#define RELATIVE(n) (-(n))
#define MILLISECONDS(n) ((int64_t)(n)*10000)
#define SPI_WRITE_MASK(a) ((a)|0x80)
#define SPI_READ_MASK(a) ((a)&0x7f)
#define ENG_RST_ADDR 0x7fff80
#define SWRST_N8_ADDR 0x3f0fe
#define SPI_RD_FAST_ADDR 0x3f310
#define NT36XXX_CMD_BOOTLOADER_RESET 0x69
#define NT36XXX_CMD_ENTER_SLEEP 0x11
#define NT36XXX_EVT_RESET_COMPLETE 0x60
#define NT36XXX_EVT_HOST_CMD 0x50
#define NVT_TRANSFER_LEN (63u*1024u)
#define FWBUFFER_SIZE 0x30000u
#define RtlCopyMemory memcpy
#define Trace(...) ((void)0)
#define InitializeObjectAttributes(o,n,a,b,c) ((o)->name=(n))
extern const GUID GUID_CONSOLE_DISPLAY_STATE;
static inline int IsEqualGUID(const GUID* a,const GUID* b){return !memcmp(a,b,sizeof(*a));}
static inline LONG InterlockedExchange(volatile LONG* p,LONG x){LONG o=*p;*p=x;return o;}
static inline DEVICE_EXTENSION* GetDeviceContext(WDFDEVICE d){return (DEVICE_EXTENSION*)d;}
int KeGetCurrentIrql(void);
void* ExAllocatePoolWithTag(int,size_t,int);
void ExFreePoolWithTag(void*,int);
NTSTATUS KeDelayExecutionThread(int,int,LARGE_INTEGER*);
NTSTATUS WdfStringCreate(void*,void*,WDFSTRING*);
void WdfStringGetUnicodeString(WDFSTRING,UNICODE_STRING*);
NTSTATUS WdfDeviceOpenRegistryKey(WDFDEVICE,int,int,void*,WDFKEY*);
NTSTATUS WdfRegistryQueryString(WDFKEY,UNICODE_STRING*,WDFSTRING);
void WdfRegistryClose(WDFKEY);
void WdfObjectDelete(void*);
void RtlInitUnicodeString(UNICODE_STRING*,const wchar_t*);
NTSTATUS RtlAppendUnicodeToString(UNICODE_STRING*,const wchar_t*);
NTSTATUS RtlAppendUnicodeStringToString(UNICODE_STRING*,UNICODE_STRING*);
NTSTATUS ExGetFirmwareEnvironmentVariable(UNICODE_STRING*,GUID*,void*,ULONG*,void*);
NTSTATUS ZwCreateFile(HANDLE*,int,OBJECT_ATTRIBUTES*,IO_STATUS_BLOCK*,void*,int,int,int,int,void*,int);
NTSTATUS ZwQueryInformationFile(HANDLE,IO_STATUS_BLOCK*,FILE_STANDARD_INFORMATION*,size_t,int);
NTSTATUS ZwReadFile(HANDLE,void*,void*,void*,IO_STATUS_BLOCK*,void*,ULONG,LARGE_INTEGER*,void*);
void ZwClose(HANDLE);
NTSTATUS SpbWriteDataSynchronously(SPB_CONTEXT*,UCHAR,PUCHAR,USHORT);
NTSTATUS SpbReadDataSynchronously(SPB_CONTEXT*,UCHAR,PUCHAR,USHORT,BOOLEAN);
int nt36xxx_set_page(SPB_CONTEXT*,unsigned int);
NTSTATUS NVTLoadFirmwareFile(WDFDEVICE,SPB_CONTEXT*);
void WdfWaitLockAcquire(WDFWAITLOCK,void*);
void WdfWaitLockRelease(WDFWAITLOCK);
void ReportStopContinuous(REPORT_CONTEXT*);
void ReportReleaseContacts(REPORT_CONTEXT*);
