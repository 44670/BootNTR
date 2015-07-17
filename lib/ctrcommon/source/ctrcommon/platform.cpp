#include "ctrcommon/platform.hpp"

#include "service.hpp"
#include "../libkhax/khax.h"

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

#include <3ds.h>

#include <ctrcommon/gpu.hpp>

static bool hasError = false;
static Error currentError = {0};

extern bool gpuInit();
extern void gpuCleanup();

extern void uiInit();
extern void uiCleanup();

bool platformInit() {
    if(gpuInit()) {
        uiInit();
        return true;
    }

    return false;
}

void platformCleanup() {
    uiCleanup();
    gpuCleanup();
    serviceCleanup();
}

bool platformIsRunning() {
    return aptMainLoop();
}

bool platformIsNinjhax() {
    Result result = hbInit();
    if(result == 0) {
        hbExit();
    }

    return result == 0;
}

bool platformExecuteKernel(s32 (*func)()) {
    if(!serviceRequire("kernel")) {
        return false;
    }

    svcBackdoor(func);
    return true;
}

u32 platformGetDeviceId() {
    if(!serviceRequire("am")) {
        return 0;
    }

    u32 deviceId;
    Result result = AM_GetDeviceId(&deviceId);
    if(result != 0) {
        platformSetError(serviceParseError((u32) result));
        return 0;
    }
    
    return deviceId;
}

bool platformIsWifiConnected() {
    if(!serviceRequire("ac")) {
        return false;
    }

    u32 status;
    Result result = ACU_GetWifiStatus(NULL, &status);
    if(result != 0) {
        platformSetError(serviceParseError((u32) result));
        return false;
    }

    return status != 0;
}

bool platformWaitForInternet() {
    if(!serviceRequire("ac")) {
        return false;
    }

    Result result = ACU_WaitInternetConnection();
    if(result != 0) {
        platformSetError(serviceParseError((u32) result));
    }

    return result == 0;
}

u8 platformGetWifiLevel() {
    return platformIsWifiConnected() ? osGetWifiStrength() : (u8) 0;
}

bool platformIsBatteryCharging() {
    if(!serviceRequire("ptm")) {
        return false;
    }

    u8 charging;
    Result result = PTMU_GetBatteryChargeState(NULL, &charging);
    if(result != 0) {
        platformSetError(serviceParseError((u32) result));
        return false;
    }

    return charging != 0;
}

u8 platformGetBatteryLevel() {
    if(!serviceRequire("ptm")) {
        return 0;
    }

    u8 batteryLevel;
    Result result = PTMU_GetBatteryLevel(NULL, &batteryLevel);
    if(result != 0) {
        platformSetError(serviceParseError((u32) result));
        return 0;
    }

    return batteryLevel;
}

u64 platformGetTime() {
    return osGetTime();
}

void platformDelay(int ms) {
    svcSleepThread(ms * 1000000);
}

void platformPrintf(const char* format, ...) {
    char buffer[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

    svcOutputDebugString(buffer, strlen(buffer));
}

bool platformHasError() {
    return hasError;
}

Error platformGetError() {
    Error error = currentError;
    if(hasError) {
        hasError = false;
        currentError = {0};
    }

    return error;
}

void platformSetError(Error error) {
    hasError = true;
    currentError = error;
}

std::string platformGetErrorString(Error error) {
    std::stringstream result;

    result << "Raw Error: 0x" << std::hex << error.raw << "\n";

    result << "Module: ";
    switch(error.module) {
        case MODULE_COMMON:
            result << "MODULE_COMMON";
            break;
        case MODULE_NN_KERNEL:
            result << "MODULE_NN_KERNEL";
            break;
        case MODULE_NN_UTIL:
            result << "MODULE_NN_UTIL";
            break;
        case MODULE_NN_FILE_SERVER:
            result << "MODULE_NN_FILE_SERVER";
            break;
        case MODULE_NN_LOADER_SERVER:
            result << "MODULE_NN_LOADER_SERVER";
            break;
        case MODULE_NN_TCB:
            result << "MODULE_NN_TCB";
            break;
        case MODULE_NN_OS:
            result << "MODULE_NN_OS";
            break;
        case MODULE_NN_DBG:
            result << "MODULE_NN_DBG";
            break;
        case MODULE_NN_DMNT:
            result << "MODULE_NN_DMNT";
            break;
        case MODULE_NN_PDN:
            result << "MODULE_NN_PDN";
            break;
        case MODULE_NN_GX:
            result << "MODULE_NN_GX";
            break;
        case MODULE_NN_I2C:
            result << "MODULE_NN_I2C";
            break;
        case MODULE_NN_GPIO:
            result << "MODULE_NN_GPIO";
            break;
        case MODULE_NN_DD:
            result << "MODULE_NN_DD";
            break;
        case MODULE_NN_CODEC:
            result << "MODULE_NN_CODEC";
            break;
        case MODULE_NN_SPI:
            result << "MODULE_NN_SPI";
            break;
        case MODULE_NN_PXI:
            result << "MODULE_NN_PXI";
            break;
        case MODULE_NN_FS:
            result << "MODULE_NN_FS";
            break;
        case MODULE_NN_DI:
            result << "MODULE_NN_DI";
            break;
        case MODULE_NN_HID:
            result << "MODULE_NN_HID";
            break;
        case MODULE_NN_CAMERA:
            result << "MODULE_NN_CAMERA";
            break;
        case MODULE_NN_PI:
            result << "MODULE_NN_PI";
            break;
        case MODULE_NN_PM:
            result << "MODULE_NN_PM";
            break;
        case MODULE_NN_PMLOW:
            result << "MODULE_NN_PMLOW";
            break;
        case MODULE_NN_FSI:
            result << "MODULE_NN_FSI";
            break;
        case MODULE_NN_SRV:
            result << "MODULE_NN_SRV";
            break;
        case MODULE_NN_NDM:
            result << "MODULE_NN_NDM";
            break;
        case MODULE_NN_NWM:
            result << "MODULE_NN_NWM";
            break;
        case MODULE_NN_SOCKET:
            result << "MODULE_NN_SOCKET";
            break;
        case MODULE_NN_LDR:
            result << "MODULE_NN_LDR";
            break;
        case MODULE_NN_ACC:
            result << "MODULE_NN_ACC";
            break;
        case MODULE_NN_ROMFS:
            result << "MODULE_NN_ROMFS";
            break;
        case MODULE_NN_AM:
            result << "MODULE_NN_AM";
            break;
        case MODULE_NN_HIO:
            result << "MODULE_NN_HIO";
            break;
        case MODULE_NN_UPDATER:
            result << "MODULE_NN_UPDATER";
            break;
        case MODULE_NN_MIC:
            result << "MODULE_NN_MIC";
            break;
        case MODULE_NN_FND:
            result << "MODULE_NN_FND";
            break;
        case MODULE_NN_MP:
            result << "MODULE_NN_MP";
            break;
        case MODULE_NN_MPWL:
            result << "MODULE_NN_MPWL";
            break;
        case MODULE_NN_AC:
            result << "MODULE_NN_AC";
            break;
        case MODULE_NN_HTTP:
            result << "MODULE_NN_HTTP";
            break;
        case MODULE_NN_DSP:
            result << "MODULE_NN_DSP";
            break;
        case MODULE_NN_SND:
            result << "MODULE_NN_SND";
            break;
        case MODULE_NN_DLP:
            result << "MODULE_NN_DLP";
            break;
        case MODULE_NN_HIOLOW:
            result << "MODULE_NN_HIOLOW";
            break;
        case MODULE_NN_CSND:
            result << "MODULE_NN_CSND";
            break;
        case MODULE_NN_SSL:
            result << "MODULE_NN_SSL";
            break;
        case MODULE_NN_AMLOW:
            result << "MODULE_NN_AMLOW";
            break;
        case MODULE_NN_NEX:
            result << "MODULE_NN_NEX";
            break;
        case MODULE_NN_FRIENDS:
            result << "MODULE_NN_FRIENDS";
            break;
        case MODULE_NN_RDT:
            result << "MODULE_NN_RDT";
            break;
        case MODULE_NN_APPLET:
            result << "MODULE_NN_APPLET";
            break;
        case MODULE_NN_NIM:
            result << "MODULE_NN_NIM";
            break;
        case MODULE_NN_PTM:
            result << "MODULE_NN_PTM";
            break;
        case MODULE_NN_MIDI:
            result << "MODULE_NN_MIDI";
            break;
        case MODULE_NN_MC:
            result << "MODULE_NN_MC";
            break;
        case MODULE_NN_SWC:
            result << "MODULE_NN_SWC";
            break;
        case MODULE_NN_FATFS:
            result << "MODULE_NN_FATFS";
            break;
        case MODULE_NN_NGC:
            result << "MODULE_NN_NGC";
            break;
        case MODULE_NN_CARD:
            result << "MODULE_NN_CARD";
            break;
        case MODULE_NN_CARDNOR:
            result << "MODULE_NN_CARDNOR";
            break;
        case MODULE_NN_SDMC:
            result << "MODULE_NN_SDMC";
            break;
        case MODULE_NN_BOSS:
            result << "MODULE_NN_BOSS";
            break;
        case MODULE_NN_DBM:
            result << "MODULE_NN_DBM";
            break;
        case MODULE_NN_CONFIG:
            result << "MODULE_NN_CONFIG";
            break;
        case MODULE_NN_PS:
            result << "MODULE_NN_PS";
            break;
        case MODULE_NN_CEC:
            result << "MODULE_NN_CEC";
            break;
        case MODULE_NN_IR:
            result << "MODULE_NN_IR";
            break;
        case MODULE_NN_UDS:
            result << "MODULE_NN_UDS";
            break;
        case MODULE_NN_PL:
            result << "MODULE_NN_PL";
            break;
        case MODULE_NN_CUP:
            result << "MODULE_NN_CUP";
            break;
        case MODULE_NN_GYROSCOPE:
            result << "MODULE_NN_GYROSCOPE";
            break;
        case MODULE_NN_MCU:
            result << "MODULE_NN_MCU";
            break;
        case MODULE_NN_NS:
            result << "MODULE_NN_NS";
            break;
        case MODULE_NN_NEWS:
            result << "MODULE_NN_NEWS";
            break;
        case MODULE_NN_RO:
            result << "MODULE_NN_RO";
            break;
        case MODULE_NN_GD:
            result << "MODULE_NN_GD";
            break;
        case MODULE_NN_CARDSPI:
            result << "MODULE_NN_CARDSPI";
            break;
        case MODULE_NN_EC:
            result << "MODULE_NN_EC";
            break;
        case MODULE_NN_WEBBRS:
            result << "MODULE_NN_WEBBRS";
            break;
        case MODULE_NN_TEST:
            result << "MODULE_NN_TEST";
            break;
        case MODULE_NN_ENC:
            result << "MODULE_NN_ENC";
            break;
        case MODULE_NN_PIA:
            result << "MODULE_NN_PIA";
            break;
        case MODULE_NN_MVD:
            result << "MODULE_NN_MVD";
            break;
        case MODULE_NN_QTM:
            result << "MODULE_NN_QTM";
            break;
        case MODULE_APPLICATION:
            result << "MODULE_APPLICATION";
            break;
        case MODULE_INVALID_RESULT_VALUE:
            result << "MODULE_INVALID_RESULT_VALUE";
            break;
        default:
            result << "<unknown>";
            break;
    }

    result << " (0x" << std::hex << error.module << ")" << "\n";

    result << "Level: ";
    switch(error.level) {
        case LEVEL_SUCCESS:
            result << "LEVEL_SUCCESS";
            break;
        case LEVEL_INFO:
            result << "LEVEL_INFO";
            break;
        case LEVEL_STATUS:
            result << "LEVEL_STATUS";
            break;
        case LEVEL_TEMPORARY:
            result << "LEVEL_TEMPORARY";
            break;
        case LEVEL_PERMANENT:
            result << "LEVEL_PERMANENT";
            break;
        case LEVEL_USAGE:
            result << "LEVEL_USAGE";
            break;
        case LEVEL_REINIT:
            result << "LEVEL_REINIT";
            break;
        case LEVEL_RESET:
            result << "LEVEL_RESET";
            break;
        case LEVEL_FATAL:
            result << "LEVEL_FATAL";
            break;
        default:
            result << "<unknown>";
            break;
    }

    result << " (0x" << std::hex << error.level << ")" << "\n";

    result << "Summary: ";
    switch(error.summary) {
        case SUMMARY_SUCCESS:
            result << "SUMMARY_SUCCESS";
            break;
        case SUMMARY_NOTHING_HAPPENED:
            result << "SUMMARY_NOTHING_HAPPENED";
            break;
        case SUMMARY_WOULD_BLOCK:
            result << "SUMMARY_WOULD_BLOCK";
            break;
        case SUMMARY_OUT_OF_RESOURCE:
            result << "SUMMARY_OUT_OF_RESOURCE";
            break;
        case SUMMARY_NOT_FOUND:
            result << "SUMMARY_NOT_FOUND";
            break;
        case SUMMARY_INVALID_STATE:
            result << "SUMMARY_INVALID_STATE";
            break;
        case SUMMARY_NOT_SUPPORTED:
            result << "SUMMARY_NOT_SUPPORTED";
            break;
        case SUMMARY_INVALID_ARGUMENT:
            result << "SUMMARY_INVALID_ARGUMENT";
            break;
        case SUMMARY_WRONG_ARGUMENT:
            result << "SUMMARY_WRONG_ARGUMENT";
            break;
        case SUMMARY_CANCELED:
            result << "SUMMARY_CANCELED";
            break;
        case SUMMARY_STATUS_CHANGED:
            result << "SUMMARY_STATUS_CHANGED";
            break;
        case SUMMARY_INTERNAL:
            result << "SUMMARY_INTERNAL";
            break;
        case SUMMARY_INVALID_RESULT_VALUE:
            result << "SUMMARY_INVALID_RESULT_VALUE";
            break;
        default:
            result << "<unknown>";
            break;
    }

    result << " (0x" << std::hex << error.summary << ")" << "\n";

    result << "Description: ";
    switch(error.description) {
        case DESCRIPTION_SUCCESS:
            result << "DESCRIPTION_SUCCESS";
            break;
        case DESCRIPTION_INVALID_MEMORY_PERMISSIONS:
            result << "DESCRIPTION_INVALID_MEMORY_PERMISSIONS";
            break;
        case DESCRIPTION_INVALID_TICKET_VERSION:
            result << "DESCRIPTION_INVALID_TICKET_VERSION";
            break;
        case DESCRIPTION_STRING_TOO_BIG:
            result << "DESCRIPTION_STRING_TOO_BIG";
            break;
        case DESCRIPTION_ACCESS_DENIED:
            result << "DESCRIPTION_ACCESS_DENIED";
            break;
        case DESCRIPTION_STRING_TOO_SMALL:
            result << "DESCRIPTION_STRING_TOO_SMALL";
            break;
        case DESCRIPTION_CAMERA_BUSY:
            result << "DESCRIPTION_CAMERA_BUSY";
            break;
        case DESCRIPTION_NOT_ENOUGH_MEMORY:
            result << "DESCRIPTION_NOT_ENOUGH_MEMORY";
            break;
        case DESCRIPTION_SESSION_CLOSED_BY_REMOTE:
            result << "DESCRIPTION_SESSION_CLOSED_BY_REMOTE";
            break;
        case DESCRIPTION_INVALID_NCCH:
            result << "DESCRIPTION_INVALID_NCCH";
            break;
        case DESCRIPTION_INVALID_TITLE_VERSION:
            result << "DESCRIPTION_INVALID_TITLE_VERSION";
            break;
        case DESCRIPTION_DATABASE_DOES_NOT_EXIST:
            result << "DESCRIPTION_DATABASE_DOES_NOT_EXIST";
            break;
        case DESCRIPTION_TRIED_TO_UNINSTALL_SYSTEM_APP:
            result << "DESCRIPTION_TRIED_TO_UNINSTALL_SYSTEM_APP";
            break;
        case DESCRIPTION_ARCHIVE_NOT_MOUNTED:
            result << "DESCRIPTION_ARCHIVE_NOT_MOUNTED";
            break;
        case DESCRIPTION_REQUEST_TIMED_OUT:
            result << "DESCRIPTION_REQUEST_TIMED_OUT";
            break;
        case DESCRIPTION_INVALID_SIGNATURE:
            result << "DESCRIPTION_INVALID_SIGNATURE";
            break;
        case DESCRIPTION_TITLE_NOT_FOUND:
            result << "DESCRIPTION_TITLE_NOT_FOUND";
            break;
        case DESCRIPTION_GAMECARD_NOT_INSERTED:
            result << "DESCRIPTION_GAMECARD_NOT_INSERTED";
            break;
        case DESCRIPTION_INVALID_FILE_OPEN_FLAGS:
            result << "DESCRIPTION_INVALID_FILE_OPEN_FLAGS";
            break;
        case DESCRIPTION_INVALID_CONFIGURATION:
            result << "DESCRIPTION_INVALID_CONFIGURATION";
            break;
        case DESCRIPTION_NCCH_HASH_CHECK_FAILED:
            result << "DESCRIPTION_NCCH_HASH_CHECK_FAILED";
            break;
        case DESCRIPTION_AES_VERIFICATION_FAILED:
            result << "DESCRIPTION_AES_VERIFICATION_FAILED";
            break;
        case DESCRIPTION_INVALID_DATABASE:
            result << "DESCRIPTION_INVALID_DATABASE";
            break;
        case DESCRIPTION_SAVE_HASH_CHECK_FAILED:
            result << "DESCRIPTION_SAVE_HASH_CHECK_FAILED";
            break;
        case DESCRIPTION_COMMAND_PERMISSION_DENIED:
            result << "DESCRIPTION_COMMAND_PERMISSION_DENIED";
            break;
        case DESCRIPTION_INVALID_PATH:
            result << "DESCRIPTION_INVALID_PATH";
            break;
        case DESCRIPTION_INCORRECT_READ_SIZE:
            result << "DESCRIPTION_INCORRECT_READ_SIZE";
            break;
        case DESCRIPTION_INVALID_SELECTION:
            result << "DESCRIPTION_INVALID_SELECTION";
            break;
        case DESCRIPTION_TOO_LARGE:
            result << "DESCRIPTION_TOO_LARGE";
            break;
        case DESCRIPTION_NOT_AUTHORIZED:
            result << "DESCRIPTION_NOT_AUTHORIZED";
            break;
        case DESCRIPTION_ALREADY_DONE:
            result << "DESCRIPTION_ALREADY_DONE";
            break;
        case DESCRIPTION_INVALID_SIZE:
            result << "DESCRIPTION_INVALID_SIZE";
            break;
        case DESCRIPTION_INVALID_ENUM_VALUE:
            result << "DESCRIPTION_INVALID_ENUM_VALUE";
            break;
        case DESCRIPTION_INVALID_COMBINATION:
            result << "DESCRIPTION_INVALID_COMBINATION";
            break;
        case DESCRIPTION_NO_DATA:
            result << "DESCRIPTION_NO_DATA";
            break;
        case DESCRIPTION_BUSY:
            result << "DESCRIPTION_BUSY";
            break;
        case DESCRIPTION_MISALIGNED_ADDRESS:
            result << "DESCRIPTION_MISALIGNED_ADDRESS";
            break;
        case DESCRIPTION_MISALIGNED_SIZE:
            result << "DESCRIPTION_MISALIGNED_SIZE";
            break;
        case DESCRIPTION_OUT_OF_MEMORY:
            result << "DESCRIPTION_OUT_OF_MEMORY";
            break;
        case DESCRIPTION_NOT_IMPLEMENTED:
            result << "DESCRIPTION_NOT_IMPLEMENTED";
            break;
        case DESCRIPTION_INVALID_ADDRESS:
            result << "DESCRIPTION_INVALID_ADDRESS";
            break;
        case DESCRIPTION_INVALID_POINTER:
            result << "DESCRIPTION_INVALID_POINTER";
            break;
        case DESCRIPTION_INVALID_HANDLE:
            result << "DESCRIPTION_INVALID_HANDLE";
            break;
        case DESCRIPTION_NOT_INITIALIZED:
            result << "DESCRIPTION_NOT_INITIALIZED";
            break;
        case DESCRIPTION_ALREADY_INITIALIZED:
            result << "DESCRIPTION_ALREADY_INITIALIZED";
            break;
        case DESCRIPTION_NOT_FOUND:
            result << "DESCRIPTION_NOT_FOUND";
            break;
        case DESCRIPTION_CANCEL_REQUESTED:
            result << "DESCRIPTION_CANCEL_REQUESTED";
            break;
        case DESCRIPTION_ALREADY_EXISTS:
            result << "DESCRIPTION_ALREADY_EXISTS";
            break;
        case DESCRIPTION_OUT_OF_RANGE:
            result << "DESCRIPTION_OUT_OF_RANGE";
            break;
        case DESCRIPTION_TIMEOUT:
            result << "DESCRIPTION_TIMEOUT";
            break;
        case DESCRIPTION_INVALID_RESULT_VALUE:
            result << "DESCRIPTION_INVALID_RESULT_VALUE";
            break;
        default:
            result << "<unknown>";
            break;
    }

    result << " (0x" << std::hex << error.description << ")" << "\n";

    return result.str();
}