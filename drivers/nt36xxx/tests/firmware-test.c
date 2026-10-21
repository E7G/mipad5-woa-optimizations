#include "mock.h"
#include "../src/nt36xxx/ntfwupdate.c"
static unsigned char image[FWBUFFER_SIZE];
static int nth, calls, allocs, keys, files, shortread, imagewrong, displayshort, panel, irql;
static unsigned long long hash;
static NTSTATUS next(void){return ++calls==nth?TEST_FAILURE:0;}
int KeGetCurrentIrql(void){return irql;}
void* ExAllocatePoolWithTag(int p,size_t n,int tag){(void)p;(void)tag;if(next())return NULL;void* x=calloc(1,n);assert(x);++allocs;return x;}
void ExFreePoolWithTag(void* p,int tag){(void)tag;assert(p&&allocs>0);--allocs;free(p);}
NTSTATUS WdfStringCreate(void* a,void* b,WDFSTRING* s){(void)a;(void)b;if(next())return TEST_FAILURE;*s=calloc(1,sizeof(UNICODE_STRING));assert(*s);++allocs;return 0;}
void WdfObjectDelete(void* o){ExFreePoolWithTag(o,0);}
void RtlInitUnicodeString(UNICODE_STRING* s,const wchar_t* b){s->Buffer=(wchar_t*)b;s->Length=(USHORT)(wcslen(b)*sizeof(wchar_t));s->MaximumLength=s->Length+sizeof(wchar_t);}
NTSTATUS WdfDeviceOpenRegistryKey(WDFDEVICE d,int a,int b,void* c,WDFKEY* k){(void)d;(void)a;(void)b;(void)c;if(next())return TEST_FAILURE;*k=(void*)1;++keys;return 0;}
NTSTATUS WdfRegistryQueryString(WDFKEY k,UNICODE_STRING* key,WDFSTRING s){assert(k);if(next())return TEST_FAILURE;
 assert(!wcscmp(key->Buffer,displayshort||panel==0x25?L"NVTFWImageHuaxingPath":L"NVTFWImageTianmaPath"));
 RtlInitUnicodeString(s,L"C:\\Firmware\\test.bin");return 0;}
void WdfRegistryClose(WDFKEY k){assert(k&&keys==1);--keys;}
void WdfStringGetUnicodeString(WDFSTRING s,UNICODE_STRING* p){*p=*(UNICODE_STRING*)s;}
NTSTATUS RtlAppendUnicodeStringToString(UNICODE_STRING* a,UNICODE_STRING* b){if(next())return TEST_FAILURE;assert(a->Length+b->Length+sizeof(wchar_t)<=a->MaximumLength);memcpy((char*)a->Buffer+a->Length,b->Buffer,b->Length);a->Length+=b->Length;a->Buffer[a->Length/sizeof(wchar_t)]=0;return 0;}
NTSTATUS RtlAppendUnicodeToString(UNICODE_STRING* a,const wchar_t* b){UNICODE_STRING s;RtlInitUnicodeString(&s,b);return RtlAppendUnicodeStringToString(a,&s);}
NTSTATUS ExGetFirmwareEnvironmentVariable(UNICODE_STRING* a,GUID* b,void* out,ULONG* len,void* c){(void)a;(void)b;(void)c;assert(*len==128);((ULONG*)out)[10]=panel;if(displayshort)*len=4;return 0;}
NTSTATUS ZwCreateFile(HANDLE* f,int a,OBJECT_ATTRIBUTES* o,IO_STATUS_BLOCK* i,void* b,int c,int d,int e,int g,void* h,int j){(void)a;(void)i;(void)b;(void)c;(void)d;(void)e;(void)g;(void)h;(void)j;if(next())return TEST_FAILURE;assert(!wcscmp(o->name->Buffer,L"\\DosDevices\\C:\\Firmware\\test.bin"));*f=(void*)1;++files;return 0;}
NTSTATUS ZwQueryInformationFile(HANDLE f,IO_STATUS_BLOCK* i,FILE_STANDARD_INFORMATION* b,size_t n,int c){(void)i;(void)n;(void)c;assert(f);if(next())return TEST_FAILURE;b->EndOfFile.QuadPart=FWBUFFER_SIZE+imagewrong;return 0;}
NTSTATUS ZwReadFile(HANDLE f,void* a,void* b,void* c,IO_STATUS_BLOCK* i,void* out,ULONG n,LARGE_INTEGER* off,void* d){(void)a;(void)b;(void)c;(void)d;assert(f&&n==FWBUFFER_SIZE&&!off->QuadPart);if(next())return TEST_FAILURE;memcpy(out,image,n);i->Information=n-shortread;return 0;}
void ZwClose(HANDLE f){assert(f&&files==1);--files;}
NTSTATUS SpbWriteDataSynchronously(SPB_CONTEXT* s,UCHAR a,PUCHAR b,USHORT n){(void)s;if(next())return TEST_FAILURE;hash=(hash^a)*1099511628211ULL;for(unsigned int i=0;i<n;++i)hash=(hash^b[i])*1099511628211ULL;return 0;}
NTSTATUS SpbReadDataSynchronously(SPB_CONTEXT* s,UCHAR a,PUCHAR b,USHORT n,BOOLEAN hack){(void)s;(void)a;(void)hack;if(next())return TEST_FAILURE;memset(b,0,n);if(n>16)b[16]=0x33;return 0;}
NTSTATUS KeDelayExecutionThread(int m,int a,LARGE_INTEGER* d){assert(m==0&&!a&&d->QuadPart<0);return 0;}
static NTSTATUS run(int fail){NTSTATUS s;assert(!allocs&&!files&&!keys);nth=fail;calls=0;hash=1469598103934665603ULL;s=NVTLoadFirmwareFile(NULL,NULL);assert(!allocs&&!files&&!keys);return s;}
static uint32_t rng=0x36523;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(int argc,char** argv){
 assert(argc==3);
 for(int j=1;j<=2;++j){FILE* f=fopen(argv[j],"rb");assert(f);assert(fread(image,1,sizeof(image),f)==sizeof(image));fclose(f);
  NABU_FW_PART parts[NABU_MAX_PARTITIONS];unsigned int count;
  assert(NabuParseFirmware(image,sizeof(image),parts,NABU_MAX_PARTITIONS,&count)&&count>=2);
  assert(!run(0));int total=calls;unsigned long long cleanhash=hash;
  printf("WIRE_HASH %d %llu\n",j,cleanhash);
  for(int failure=1;failure<=total;++failure){assert(!NT_SUCCESS(run(failure)));assert(calls>=failure);}
  shortread=1;assert(run(0)==STATUS_DEVICE_DATA_ERROR);shortread=0;
  imagewrong=1;assert(run(0)==STATUS_INVALID_IMAGE_FORMAT);imagewrong=0;
  panel=0x25;assert(!run(0)&&hash==cleanhash);panel=0;displayshort=1;assert(!run(0));displayshort=0;
  irql=2;assert(run(0)==STATUS_INVALID_DEVICE_STATE);irql=0;
  for(unsigned int n=0;n<0x30;++n)assert(!NabuParseFirmware(image,n,parts,NABU_MAX_PARTITIONS,&count));
  assert(!NabuParseFirmware(image,sizeof(image),parts,1,&count));
  printf("PASS: image %d; %d individually injected API/allocation/SPI failures, cleanup leak checks; short read, wrong size, panel selection, IRQL.\n",j,total);
 }
 unsigned char saved[FWBUFFER_SIZE];memcpy(saved,image,sizeof(image));
 for(int i=0;i<50000;++i){NABU_FW_PART parts[NABU_MAX_PARTITIONS];unsigned int count=999;
  memcpy(image,saved,sizeof(image));unsigned int pos=random32()%0x300;image[pos]=(unsigned char)random32();
  unsigned int n=random32()%(FWBUFFER_SIZE+1);
  if(NabuParseFirmware(image,n,parts,NABU_MAX_PARTITIONS,&count)){
   assert(count<=NABU_MAX_PARTITIONS);
   for(unsigned int j=0;j<count;++j)if(parts[j].size)assert(parts[j].bin<n&&parts[j].size<n-parts[j].bin);
  }else assert(count==0);
 }
 puts("PASS: production bounded parser; 50000 deterministic malformed/truncated cases. No real SPI/controller behavior tested.");return 0;
}
