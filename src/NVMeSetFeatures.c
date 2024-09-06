#include <stdio.h>
#include <windows.h>
#include "NVMeFeaturesHCTM.h"
#include "NVMeFeaturesTemperatureThreshold.h"
#include "NVMeFeaturesVWC.h"
#include "NVMeUtils.h"

#define NVME_FEATURE_TEMPERATURE_THRESHOLD 0x04

int iNVMeSetFeatures(HANDLE _hDevice) {
    int iResult = -1;
    int iFId = 0;
    char cCmd;
    char strCmd[256];
    char strPrompt[1024];
    sprintf_s(strPrompt, 1024,
              "\n# Input Feature Identifier (in hex):"
              "\n#    Supported Features are:"
              "\n#     %02Xh = Temperature Threshold"
              "\n#     %02Xh = Volatile Write Cache"
              "\n#     %02Xh = Host Controlled Thermal Management"
              "\n",
              NVME_FEATURE_TEMPERATURE_THRESHOLD,
              NVME_FEATURE_VOLATILE_WRITE_CACHE,
              NVME_FEATURE_HOST_CONTROLLED_THERMAL_MANAGEMENT);
    iFId = iGetConsoleInputHex((const char*)strPrompt, strCmd);
    switch (iFId) {
        case NVME_FEATURE_TEMPERATURE_THRESHOLD:
            cCmd = cGetConsoleInput(
                "\n# Set Feature - Temperature Threshold (Feature Identifier = "
                "04h), Press 'y' to continue\n",
                strCmd);
            if (cCmd == 'y') {
                iResult = iNVMeSetFeaturesTemperatureThreshold(_hDevice);
            }
            break;
        case NVME_FEATURE_VOLATILE_WRITE_CACHE:
            cCmd = cGetConsoleInput(
                "\n# Set Feature - Volatile Write Cache (Feature Identifier = "
                "06h), Press 'y' to continue\n",
                strCmd);
            if (cCmd == 'y') {
                iResult = iNVMeSetFeaturesVWC(_hDevice);
            }
            break;
        case NVME_FEATURE_HOST_CONTROLLED_THERMAL_MANAGEMENT:
            cCmd = cGetConsoleInput(
                "\n# Set Feature - Host Controlled Thermal Management (Feature "
                "Identifier = 10h), Press 'y' to continue\n",
                strCmd);
            if (cCmd == 'y') {
                iResult = iNVMeSetFeaturesHCTM(_hDevice);
            }
            break;
        default:
            fprintf(stderr, "\n[E] Feature is not implemented yet.\n");
            break;
    }
    printf("\n");
    return iResult;
}