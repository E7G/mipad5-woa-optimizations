// Copyright (c) original driver authors. Local nabu sleep candidate.
// Lock order: PowerTransitionLock -> ControllerLock -> SpbLock.
// Replay timer never takes these locks; drain before taking ControllerLock.
#include <internal.h>
#include <nt36xxx/ntinternal.h>
#include <nt36xxx/ntfwupdate.h>
#include <power.tmh>
#define NABU_EVENT_BUFFER 0x2FE00u

static NTSTATUS NabuAwakeLocked(PDEVICE_EXTENSION dev, BOOLEAN force)
{
    FT5X_CONTROLLER_CONTEXT* c = dev->TouchContext;
    NTSTATUS status;
    if (!force && c->FirmwareReady && !c->ChipAsleep) return STATUS_SUCCESS;
    c->FirmwareReady = FALSE;
    status = NVTLoadFirmwareFile(dev->FxDevice, &dev->I2CContext);
    if (!NT_SUCCESS(status)) return status;
    c->ChipAsleep = FALSE;
    c->FirmwareReady = TRUE;
    Trace(TRACE_LEVEL_INFORMATION, TRACE_POWER, "Nabu firmware transfer complete");
    return STATUS_SUCCESS;
}

static NTSTATUS NabuSleepLocked(PDEVICE_EXTENSION dev)
{
    FT5X_CONTROLLER_CONTEXT* c = dev->TouchContext;
    UCHAR command = NT36XXX_CMD_ENTER_SLEEP;
    LARGE_INTEGER delay;
    NTSTATUS status;
    if (c->ChipAsleep) return STATUS_SUCCESS;
    if (!c->FirmwareReady) return STATUS_INVALID_DEVICE_STATE;
    status = nt36xxx_set_page(&dev->I2CContext, NABU_EVENT_BUFFER | NT36XXX_EVT_HOST_CMD);
    if (!NT_SUCCESS(status)) return status;
    status = SpbWriteDataSynchronously(&dev->I2CContext,
        SPI_WRITE_MASK(NT36XXX_EVT_HOST_CMD), &command, sizeof(command));
    if (!NT_SUCCESS(status)) return status;
    delay.QuadPart = RELATIVE(MILLISECONDS(50));
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
    c->ChipAsleep = TRUE;
    c->FirmwareReady = FALSE;
    Trace(TRACE_LEVEL_INFORMATION, TRACE_POWER, "Nabu deep sleep command accepted");
    return STATUS_SUCCESS;
}

// Caller owns PowerTransitionLock. WDF owns interrupt connect/disconnect.
// Never call WdfInterruptDisable/Enable from an arbitrary Po callback thread.
static NTSTATUS NabuDisplayLocked(PDEVICE_EXTENSION dev)
{
    FT5X_CONTROLLER_CONTEXT* c = dev->TouchContext;
    NTSTATUS status;
    if (!dev->RuntimeD0) return STATUS_SUCCESS;
    InterlockedExchange(&dev->ReportContext.ReportsSuppressed, 1);
    ReportStopContinuous(&dev->ReportContext);
    WdfWaitLockAcquire(c->ControllerLock, NULL);
    if (dev->DisplayOn) {
        status = NabuAwakeLocked(dev, FALSE);
        if (NT_SUCCESS(status)) InterlockedExchange(&dev->ReportContext.ReportsSuppressed, 0);
    } else {
        ReportReleaseContacts(&dev->ReportContext);
        status = NabuSleepLocked(dev);
        if (!NT_SUCCESS(status) && c->FirmwareReady && !c->ChipAsleep)
            InterlockedExchange(&dev->ReportContext.ReportsSuppressed, 0);
    }
    WdfWaitLockRelease(c->ControllerLock);
    if (!dev->DisplayOn) ReportStopContinuous(&dev->ReportContext);
    Trace(TRACE_LEVEL_INFORMATION, TRACE_POWER,
        "Nabu display request %ld completed 0x%08lX", dev->DisplayOn, status);
    return status;
}

_Use_decl_annotations_
NTSTATUS TchPowerSettingCallback(LPCGUID setting, PVOID value, ULONG length, PVOID context)
{
    PDEVICE_EXTENSION dev = context;
    ULONG state;
    NTSTATUS status = STATUS_SUCCESS;
    if (!dev || !setting || !value || length != sizeof(ULONG)) return STATUS_INVALID_PARAMETER;
    RtlCopyMemory(&state, value, sizeof(state));
    if (!IsEqualGUID(&GUID_CONSOLE_DISPLAY_STATE, setting)) return STATUS_SUCCESS;
    if (state > 2) return STATUS_INVALID_PARAMETER;
    if (state == 2) return STATUS_SUCCESS;
    // PrepareHardware can receive the initial callback before entering D0.
    WdfWaitLockAcquire(dev->PowerTransitionLock, NULL);
    InterlockedExchange(&dev->DisplayOn, state == 1);
    if (dev->TouchContext) status = NabuDisplayLocked(dev);
    WdfWaitLockRelease(dev->PowerTransitionLock);
    return status;
}

NTSTATUS NabuEnterD0(WDFDEVICE device)
{
    PDEVICE_EXTENSION dev = GetDeviceContext(device);
    FT5X_CONTROLLER_CONTEXT* c = dev->TouchContext;
    NTSTATUS status;
    if (!c) return STATUS_INVALID_DEVICE_STATE;
    WdfWaitLockAcquire(dev->PowerTransitionLock, NULL);
    InterlockedExchange(&dev->ReportContext.ReportsSuppressed, 1);
    ReportStopContinuous(&dev->ReportContext);
    WdfWaitLockAcquire(c->ControllerLock, NULL);
    status = NabuAwakeLocked(dev, TRUE); // PEP may have removed power in D3.
    if (NT_SUCCESS(status)) {
        c->DevicePowerState = PowerDeviceD0;
        InterlockedExchange(&dev->RuntimeD0, 1);
        if (dev->DisplayOn) InterlockedExchange(&dev->ReportContext.ReportsSuppressed, 0);
        else {
            ReportReleaseContacts(&dev->ReportContext);
            status = NabuSleepLocked(dev);
        }
    }
    if (!NT_SUCCESS(status)) InterlockedExchange(&dev->RuntimeD0, 0);
    WdfWaitLockRelease(c->ControllerLock);
    WdfWaitLockRelease(dev->PowerTransitionLock);
    return status;
}

NTSTATUS NabuExitD0(WDFDEVICE device)
{
    PDEVICE_EXTENSION dev = GetDeviceContext(device);
    FT5X_CONTROLLER_CONTEXT* c = dev->TouchContext;
    NTSTATUS status = STATUS_SUCCESS;
    if (!c) return STATUS_SUCCESS;
    WdfWaitLockAcquire(dev->PowerTransitionLock, NULL);
    InterlockedExchange(&dev->ReportContext.ReportsSuppressed, 1);
    ReportStopContinuous(&dev->ReportContext);
    WdfWaitLockAcquire(c->ControllerLock, NULL);
    if (dev->RuntimeD0) {
        ReportReleaseContacts(&dev->ReportContext);
        status = NabuSleepLocked(dev);
    }
    InterlockedExchange(&dev->RuntimeD0, 0);
    c->FirmwareReady = FALSE;
    c->DevicePowerState = PowerDeviceD3;
    WdfWaitLockRelease(c->ControllerLock);
    WdfWaitLockRelease(dev->PowerTransitionLock);
    return status;
}
