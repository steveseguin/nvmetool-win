#include <stdio.h>
#include <windows.h>
#include <winioctl.h>  // Add this to ensure we have the latest definitions
#include "NVMeIdentifyController.h"
#include "NVMeUtils.h"
#include "WinFunc.h"
#include "NVMeGetFeatures.h"

#ifndef IOCTL_STORAGE_SET_TEMPERATURE_THRESHOLD
#define IOCTL_STORAGE_SET_TEMPERATURE_THRESHOLD CTL_CODE(IOCTL_STORAGE_BASE, 0x0450, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#ifndef STORAGE_TEMPERATURE_THRESHOLD_FLAG_ADAPTER_REQUEST
#define STORAGE_TEMPERATURE_THRESHOLD_FLAG_ADAPTER_REQUEST 0x1
#endif

typedef union {
    struct {
        ULONG TMPTH : 16;  // Temperature Threshold (TMPTH)
        ULONG TMPSEL : 4;  // Threshold Temperature Select
        ULONG THSEL : 2;   // Threshold Type Select
        ULONG Reserved : 10;
    } DUMMYSTRUCTNAME;
    ULONG AsUlong;
} NVME_CDW11_FEATURE_TEMP_THRESHOLD, *PNVME_CDW11_FEATURE_TEMP_THRESHOLD;


static int siNVMeSetFeaturesTemperatureThreshold(HANDLE _hDevice, DWORD _cdw10, DWORD _cdw11) {
    int iResult = -1;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG returnedLength = 0;

    PSTORAGE_PROPERTY_SET query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR_EXT protocolDataDescr = NULL;

    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROPERTY_SET, AdditionalParameters) +
                   sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA_EXT);
    buffer = malloc(bufferLength);

    if (buffer == NULL) {
        vPrintSystemError(GetLastError(), "malloc");
        return iResult;
    }

    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_SET)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR_EXT)buffer;
    protocolData =
        (PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT)query->AdditionalParameters;

    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->SetType = PropertyStandardSet;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeFeature;
    protocolData->ProtocolDataValue = _cdw10;
    protocolData->ProtocolDataSubValue = _cdw11;
    protocolData->ProtocolDataOffset = 0;
    protocolData->ProtocolDataLength = 0;

    // Send request down.
    iResult = iIssueDeviceIoControl(_hDevice, IOCTL_STORAGE_SET_PROPERTY,
                                    buffer, bufferLength, buffer, bufferLength,
                                    &returnedLength, NULL);

    if (iResult == 0) {
        printf("\n[I] Command Set Features (Temperature Threshold) succeeded.\n");
    }

    if (buffer != NULL) {
        free(buffer);
    }

    return iResult;
}

int iNVMeSetFeaturesTemperatureThreshold(HANDLE _hDevice) {
    int iResult = -1;
    int iTMPTH;
    ULONG returnedLength = 0;

    printf("\n[I] Setting Temperature Threshold");
    printf("\n[I] Current Over Temperature Threshold is %d (K)", g_stController.WCTEMP);

    // Get user input
    {
        char cCmd;
        char strCmd[256];
        char strPrompt[1024];
        iTMPTH = iGetConsoleInputDec(
            "\n# Input new Over Temperature Threshold in Kelvin\n",
            strCmd);
        sprintf_s(strPrompt, 1024,
                  "\n# You specified new threshold = %d (K), press 'y' to continue\n",
                  iTMPTH);
        cCmd = cGetConsoleInput(strPrompt, strCmd);
        if ((cCmd != 'y') && (cCmd != 'Y')) {
            printf("\n[I] Process aborted\n");
            return 0;
        }
    }

    // Set temperature using Windows IOCTL
    {
        STORAGE_TEMPERATURE_THRESHOLD setThreshold = {0};
        setThreshold.Version = sizeof(STORAGE_TEMPERATURE_THRESHOLD);
        setThreshold.Size = sizeof(STORAGE_TEMPERATURE_THRESHOLD);
        setThreshold.Flags = STORAGE_TEMPERATURE_THRESHOLD_FLAG_ADAPTER_REQUEST;
        setThreshold.Index = 0;
        setThreshold.Threshold = iTMPTH - 273;
        setThreshold.OverThreshold = TRUE;
        iResult = DeviceIoControl(_hDevice,
                                  IOCTL_STORAGE_SET_TEMPERATURE_THRESHOLD,
                                  &setThreshold,
                                  sizeof(STORAGE_TEMPERATURE_THRESHOLD),
                                  NULL,
                                  0,
                                  &returnedLength,
                                  NULL);
        if (iResult == 0) {
            DWORD error = GetLastError();
            vPrintSystemError(error, "DeviceIoControl(IOCTL_STORAGE_SET_TEMPERATURE_THRESHOLD)");
            return 0;
        } else {
            printf("\n[I] Successfully set current Over Temperature Threshold to %d (K)\n", iTMPTH);
            printf("\n[W] This change is temporary and will reset after a system reboot.\n");
        }
    }

    // Verify the current value
    uint32_t currentValue = 0;
    DWORD featureId = NVME_FEATURE_TEMPERATURE_THRESHOLD;
    int selectType = NVME_FEATURE_VALUE_CURRENT;
    DWORD cdw11 = 0;
    if (iNVMeGetFeature32(_hDevice, featureId, selectType, cdw11, &currentValue) == 0) {
        printf("\n[I] Verified current Over Temperature Threshold: %d (K)\n", (currentValue & 0xFFFF));
        if ((currentValue & 0xFFFF) != iTMPTH) {
            printf("\n[W] The verified threshold does not match the requested threshold. The change may not have been applied correctly.\n");
        }
    } else {
        printf("\n[W] Failed to verify current Over Temperature Threshold\n");
    }

    return 1;
}