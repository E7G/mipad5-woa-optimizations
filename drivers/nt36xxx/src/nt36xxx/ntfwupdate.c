#include <nt36xxx/ntfwupdate.h>
#include <nt36xxx/fwparse.h>
#include <ntfwupdate.tmh>

//NT36523 memory map
unsigned int EVENT_BUF_ADDR = 0x2FE00;
/* Phase 2 Host Download */
unsigned int BOOT_RDY_ADDR = 0x3F10D;
unsigned int TX_AUTO_COPY_EN = 0x3F7E8;
/* BLD CRC */
unsigned int ILM_LENGTH_ADDR = 0x3F118;
unsigned int DLM_LENGTH_ADDR = 0x3F130;
unsigned int ILM_DES_ADDR = 0x3F128;
unsigned int DLM_DES_ADDR = 0x3F12C;
unsigned int G_ILM_CHECKSUM_ADDR = 0x3F100;
unsigned int G_DLM_CHECKSUM_ADDR = 0x3F104;
unsigned int BLD_CRC_EN_ADDR = 0x3F30E;

int nt36xxx_set_page(IN SPB_CONTEXT* SpbContext, IN unsigned int pageaddr)
{
    unsigned char addr = 0xFF; //set index/page/addr command
    unsigned char buf[4] = { 0 };

    buf[0] = (pageaddr >> 15) & 0xFF;
    buf[1] = (pageaddr >> 7) & 0xFF;

    return SpbWriteDataSynchronously(SpbContext, SPI_WRITE_MASK(addr), buf, 2);
}

int nt36xxx_write_addr(IN SPB_CONTEXT* SpbContext, unsigned int addr, unsigned char data)
{
    int ret = 0;

    unsigned char buf[1];

    buf[0] = data;

    //---set xdata index---
    ret = nt36xxx_set_page(SpbContext, addr);
    if (ret) {
        return ret;
    }

    //---write data to index---
    ret = SpbWriteDataSynchronously(SpbContext, SPI_WRITE_MASK(addr & 0x7F), buf, 1);
    if (ret) {
        return ret;
    }

    return ret;
}

int nt36xxx_eng_reset(IN SPB_CONTEXT* SpbContext)
{
    LARGE_INTEGER Interval;
    int ret;

    ret = nt36xxx_write_addr(SpbContext, ENG_RST_ADDR, 0x5A);
    if (ret)
        return ret;

    Interval.QuadPart = RELATIVE(MILLISECONDS(10));
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);

    return ret;
}

int nt36xxx_bootloader_reset(IN SPB_CONTEXT* SpbContext)
{
    LARGE_INTEGER Interval;
    int ret;

    ret = nt36xxx_write_addr(SpbContext, SWRST_N8_ADDR, NT36XXX_CMD_BOOTLOADER_RESET);
    if (ret)
        return ret;

    /* MCU has to reboot from bootloader: this is the typical boot time */
    Interval.QuadPart = RELATIVE(MILLISECONDS(5));
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);

    ret = nt36xxx_write_addr(SpbContext, SPI_RD_FAST_ADDR, 0x00);
    if (ret)
        return ret;

    return ret;
}


/* Every operation propagates failure. Keep upstream wire framing/CRC byte 16. */
#define CHECK_IO(expr) do { status = (expr); if (!NT_SUCCESS(status)) goto Cleanup; } while (0)

static NTSTATUS NabuCrcBank(SPB_CONTEXT* spb, unsigned int dest,
    unsigned int lenaddr, unsigned int crcaddr, const NABU_FW_PART* p)
{
    NTSTATUS status;
    UCHAR b[4];
    CHECK_IO(nt36xxx_set_page(spb, dest));
    b[0] = (UCHAR)p->sram; b[1] = (UCHAR)(p->sram >> 8); b[2] = (UCHAR)(p->sram >> 16);
    CHECK_IO(SpbWriteDataSynchronously(spb, SPI_WRITE_MASK(dest & 0x7f), b, 3));
    b[0] = (UCHAR)p->size; b[1] = (UCHAR)(p->size >> 8); b[2] = (UCHAR)((p->size >> 16) & 1);
    CHECK_IO(SpbWriteDataSynchronously(spb, SPI_WRITE_MASK(lenaddr & 0x7f), b, 3));
    b[0] = (UCHAR)p->crc; b[1] = (UCHAR)(p->crc >> 8);
    b[2] = (UCHAR)(p->crc >> 16); b[3] = (UCHAR)(p->crc >> 24);
    CHECK_IO(SpbWriteDataSynchronously(spb, SPI_WRITE_MASK(crcaddr & 0x7f), b, 4));
Cleanup:
    return status;
}

static NTSTATUS NabuFinishTransfer(SPB_CONTEXT* spb)
{
    NTSTATUS status;
    UCHAR b[20] = { 0 };
    LARGE_INTEGER delay;
    CHECK_IO(nt36xxx_set_page(spb, BLD_CRC_EN_ADDR));
    b[0] = 0xff;
    CHECK_IO(SpbReadDataSynchronously(spb, SPI_READ_MASK(BLD_CRC_EN_ADDR & 0x7f), b, 20, FALSE));
    b[0] = b[16] | 0x80; /* Preserve upstream controller-specific SPI framing. */
    CHECK_IO(SpbWriteDataSynchronously(spb, SPI_WRITE_MASK(BLD_CRC_EN_ADDR & 0x7f), b, 1));
    CHECK_IO(nt36xxx_set_page(spb, EVENT_BUF_ADDR));
    b[0] = 0;
    CHECK_IO(SpbWriteDataSynchronously(spb, SPI_WRITE_MASK(NT36XXX_EVT_RESET_COMPLETE), b, 1));
    b[0] = 0xae;
    CHECK_IO(SpbWriteDataSynchronously(spb, SPI_WRITE_MASK(NT36XXX_EVT_HOST_CMD), b, 1));
    CHECK_IO(nt36xxx_write_addr(spb, BOOT_RDY_ADDR, 1));
    delay.QuadPart = RELATIVE(MILLISECONDS(5));
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
    delay.QuadPart = RELATIVE(MILLISECONDS(35));
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
    CHECK_IO(nt36xxx_set_page(spb, EVENT_BUF_ADDR | NT36XXX_EVT_HOST_CMD));
    b[0] = 0x7b; b[1] = FALSE;
    CHECK_IO(SpbWriteDataSynchronously(spb, SPI_WRITE_MASK(NT36XXX_EVT_HOST_CMD), b, 2));
Cleanup:
    return status;
}

NTSTATUS NVTLoadFirmwareFile(WDFDEVICE device, SPB_CONTEXT* spb)
{
    NTSTATUS status;
    HANDLE file = NULL;
    WDFKEY key = NULL;
    WDFSTRING pathString = NULL;
    PUCHAR buffer = NULL, transfer = NULL;
    NABU_FW_PART* parts = NULL;
    unsigned int count = 0, i;
    UNICODE_STRING path, fullPath = { 0 }, keyName, efiName;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK iosb = { 0 };
    FILE_STANDARD_INFORMATION fileInfo;
    LARGE_INTEGER offset;
    GUID efiGuid = { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };
    ULONG display[32] = { 0 }, displayLength = sizeof(display);
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) return STATUS_INVALID_DEVICE_STATE;
    RtlInitUnicodeString(&efiName, L"UEFIDisplayInfo");
    status = ExGetFirmwareEnvironmentVariable(&efiName, &efiGuid, display, &displayLength, NULL);
    if (NT_SUCCESS(status) && displayLength >= 11u * sizeof(ULONG) && display[10] != 0x25)
        RtlInitUnicodeString(&keyName, L"NVTFWImageTianmaPath");
    else
        RtlInitUnicodeString(&keyName, L"NVTFWImageHuaxingPath");
    CHECK_IO(WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &pathString));
    CHECK_IO(WdfDeviceOpenRegistryKey(device, PLUGPLAY_REGKEY_DRIVER, GENERIC_READ,
        WDF_NO_OBJECT_ATTRIBUTES, &key));
    CHECK_IO(WdfRegistryQueryString(key, &keyName, pathString));
    WdfRegistryClose(key); key = NULL;
    WdfStringGetUnicodeString(pathString, &path);
    /* Prefix, trailing NUL, and USHORT overflow checked before allocation. */
    if (!path.Length || (path.Length & 1) || path.Length > 0xffe0u) {
        status = STATUS_OBJECT_PATH_INVALID; goto Cleanup;
    }
    fullPath.MaximumLength = (USHORT)(path.Length + sizeof(L"\\DosDevices\\"));
    fullPath.Buffer = ExAllocatePoolWithTag(NonPagedPool, fullPath.MaximumLength, TOUCH_POOL_TAG);
    if (!fullPath.Buffer) { status = STATUS_INSUFFICIENT_RESOURCES; goto Cleanup; }
    CHECK_IO(RtlAppendUnicodeToString(&fullPath, L"\\DosDevices\\"));
    CHECK_IO(RtlAppendUnicodeStringToString(&fullPath, &path));
    InitializeObjectAttributes(&attributes, &fullPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    CHECK_IO(ZwCreateFile(&file, GENERIC_READ | SYNCHRONIZE, &attributes, &iosb, NULL,
        FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, NULL, 0));
    CHECK_IO(ZwQueryInformationFile(file, &iosb, &fileInfo, sizeof(fileInfo), FileStandardInformation));
    if (fileInfo.EndOfFile.QuadPart != FWBUFFER_SIZE) { status = STATUS_INVALID_IMAGE_FORMAT; goto Cleanup; }
    buffer = ExAllocatePoolWithTag(NonPagedPool, FWBUFFER_SIZE, TOUCH_POOL_TAG);
    parts = ExAllocatePoolWithTag(NonPagedPool, NABU_MAX_PARTITIONS * sizeof(*parts), TOUCH_POOL_TAG);
    if (!buffer || !parts) { status = STATUS_INSUFFICIENT_RESOURCES; goto Cleanup; }
    offset.QuadPart = 0;
    CHECK_IO(ZwReadFile(file, NULL, NULL, NULL, &iosb, buffer, FWBUFFER_SIZE, &offset, NULL));
    if (iosb.Information != FWBUFFER_SIZE) { status = STATUS_DEVICE_DATA_ERROR; goto Cleanup; }
    ZwClose(file); file = NULL;
    if (!NabuParseFirmware(buffer, FWBUFFER_SIZE, parts, NABU_MAX_PARTITIONS, &count)) {
        status = STATUS_INVALID_IMAGE_FORMAT; goto Cleanup;
    }
    transfer = ExAllocatePoolWithTag(NonPagedPool, NVT_TRANSFER_LEN + 2, TOUCH_POOL_TAG);
    if (!transfer) { status = STATUS_INSUFFICIENT_RESOURCES; goto Cleanup; }
    CHECK_IO(nt36xxx_bootloader_reset(spb));
    CHECK_IO(NabuCrcBank(spb, ILM_DES_ADDR, ILM_LENGTH_ADDR, G_ILM_CHECKSUM_ADDR, &parts[0]));
    CHECK_IO(NabuCrcBank(spb, DLM_DES_ADDR, DLM_LENGTH_ADDR, G_DLM_CHECKSUM_ADDR, &parts[1]));
    CHECK_IO(nt36xxx_write_addr(spb, TX_AUTO_COPY_EN, 0x69));
    for (i = 0; i < count; ++i) {
        unsigned int remaining, bin, sram;
        if (!parts[i].size) continue;
        remaining = parts[i].size + 1;
        bin = parts[i].bin; sram = parts[i].sram;
        while (remaining) {
            USHORT chunk = (USHORT)(remaining < NVT_TRANSFER_LEN ? remaining : NVT_TRANSFER_LEN);
            CHECK_IO(nt36xxx_set_page(spb, sram));
            RtlCopyMemory(transfer, buffer + bin, chunk);
            CHECK_IO(SpbWriteDataSynchronously(spb, SPI_WRITE_MASK(sram & 0x7f), transfer, chunk));
            remaining -= chunk; bin += chunk; sram += chunk;
        }
    }
    CHECK_IO(NabuFinishTransfer(spb));
Cleanup:
    if (file) ZwClose(file);
    if (key) WdfRegistryClose(key);
    if (pathString) WdfObjectDelete(pathString);
    if (fullPath.Buffer) ExFreePoolWithTag(fullPath.Buffer, TOUCH_POOL_TAG);
    if (buffer) ExFreePoolWithTag(buffer, TOUCH_POOL_TAG);
    if (parts) ExFreePoolWithTag(parts, TOUCH_POOL_TAG);
    if (transfer) ExFreePoolWithTag(transfer, TOUCH_POOL_TAG);
    return status;
}
