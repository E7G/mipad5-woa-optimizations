#include "mock.h"
#include "../src/power.c"
const GUID GUID_CONSOLE_DISPLAY_STATE={1,0,0,{0}};
static FT5X_CONTROLLER_CONTEXT chip;
static DEVICE_EXTENSION dev;
static int locks[3], depth, fw, page, writecmd, failfw, failpage, failwrite, released, delays;
static ULONG value;
void WdfWaitLockAcquire(WDFWAITLOCK h,void* t){int n=(int)(intptr_t)h;(void)t;assert(n==1||n==2);assert(!locks[n]);if(n==2)assert(locks[1]);locks[n]=1;++depth;}
void WdfWaitLockRelease(WDFWAITLOCK h){int n=(int)(intptr_t)h;assert(locks[n]);if(n==1)assert(!locks[2]);locks[n]=0;--depth;}
void ReportStopContinuous(REPORT_CONTEXT* r){(void)r;assert(!locks[2]);}
void ReportReleaseContacts(REPORT_CONTEXT* r){assert(r->ReportsSuppressed&&locks[2]);++released;}
NTSTATUS NVTLoadFirmwareFile(WDFDEVICE d,SPB_CONTEXT* s){assert(d==&dev&&s==&dev.I2CContext&&locks[2]);++fw;return failfw?TEST_FAILURE:0;}
int nt36xxx_set_page(SPB_CONTEXT* s,unsigned int p){assert(s==&dev.I2CContext&&p==(0x2fe00|0x50)&&locks[2]);++page;return failpage?TEST_FAILURE:0;}
NTSTATUS SpbWriteDataSynchronously(SPB_CONTEXT* s,UCHAR a,PUCHAR b,USHORT n){assert(s==&dev.I2CContext&&a==0xd0&&n==1&&b[0]==0x11&&locks[2]);++writecmd;return failwrite?TEST_FAILURE:0;}
NTSTATUS KeDelayExecutionThread(int mode,int alert,LARGE_INTEGER* d){assert(mode==KernelMode&&!alert&&d->QuadPart==-500000);++delays;return 0;}
static NTSTATUS display(ULONG v){NTSTATUS s;value=v;s=TchPowerSettingCallback(&GUID_CONSOLE_DISPLAY_STATE,&value,sizeof(value),&dev);assert(depth==0);return s;}
static void reset(void){memset(&dev,0,sizeof(dev));memset(&chip,0,sizeof(chip));memset(locks,0,sizeof(locks));depth=fw=page=writecmd=failfw=failpage=failwrite=released=delays=0;dev.FxDevice=&dev;dev.TouchContext=&chip;dev.PowerTransitionLock=(void*)1;chip.ControllerLock=(void*)2;dev.DisplayOn=1;dev.ReportContext.ReportsSuppressed=1;}
int main(void){
 reset();assert(!display(0));assert(!page&&!writecmd&&!fw);assert(!display(1));
 assert(!NabuEnterD0(&dev));assert(fw==1&&dev.RuntimeD0&&!dev.ReportContext.ReportsSuppressed);
 assert(!display(2));assert(fw==1&&!dev.ReportContext.ReportsSuppressed);
 assert(!display(0));assert(page==1&&writecmd==1&&delays==1&&chip.ChipAsleep&&dev.ReportContext.ReportsSuppressed);
 assert(!display(0));assert(page==1&&writecmd==1);assert(!display(1));assert(fw==2&&!chip.ChipAsleep);
 for(int i=0;i<10000;++i){assert(!display(0));assert(!display(1));}
 assert(!NabuExitD0(&dev));assert(!dev.RuntimeD0&&dev.ReportContext.ReportsSuppressed);
 int p=page;assert(!display(1));assert(page==p);assert(!NabuEnterD0(&dev));assert(!dev.ReportContext.ReportsSuppressed);
 assert(display(3)==STATUS_INVALID_PARAMETER);
 assert(TchPowerSettingCallback(NULL,&value,4,&dev)==STATUS_INVALID_PARAMETER);
 assert(TchPowerSettingCallback(&GUID_CONSOLE_DISPLAY_STATE,&value,3,&dev)==STATUS_INVALID_PARAMETER);
 reset();assert(!NabuEnterD0(&dev));failpage=1;assert(display(0)==TEST_FAILURE);assert(!chip.ChipAsleep&&chip.FirmwareReady&&!dev.ReportContext.ReportsSuppressed&&!writecmd);
 failpage=0;failwrite=1;assert(display(0)==TEST_FAILURE);assert(!chip.ChipAsleep&&!dev.ReportContext.ReportsSuppressed&&!delays);
 failwrite=0;assert(!display(0));failfw=1;assert(display(1)==TEST_FAILURE);assert(dev.ReportContext.ReportsSuppressed&&!chip.FirmwareReady);
 failfw=0;assert(!display(1));assert(!dev.ReportContext.ReportsSuppressed);
 reset();failfw=1;assert(NabuEnterD0(&dev)==TEST_FAILURE);assert(!dev.RuntimeD0&&dev.ReportContext.ReportsSuppressed&&depth==0);
 reset();assert(!display(0));assert(!NabuEnterD0(&dev));assert(fw==1&&chip.ChipAsleep&&dev.ReportContext.ReportsSuppressed);assert(!NabuExitD0(&dev));assert(writecmd==1);
 reset();dev.TouchContext=NULL;assert(!display(0));assert(NabuEnterD0(&dev)==STATUS_INVALID_DEVICE_STATE);assert(!NabuExitD0(&dev));
 puts("PASS: production power.c; 10000 display cycles, D0 sequencing, IO/firmware failures, lock order, timer drain ordering. Mocks do not validate real WDF concurrency or hardware.");return 0;
}
