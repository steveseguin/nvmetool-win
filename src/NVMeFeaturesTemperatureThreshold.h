#pragma once
#include <windows.h>
#include <stdint.h>  // This defines uint32_t

int iNVMeSetFeaturesTemperatureThreshold(HANDLE _hDevice);
int iNVMeGetFeature32(HANDLE hDevice, DWORD dwFId, int iType, DWORD dwCDW11, uint32_t* pulData);