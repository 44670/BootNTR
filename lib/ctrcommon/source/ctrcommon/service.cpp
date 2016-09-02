#include "service.hpp"

#include "../libkhax/khax.h"

#include <malloc.h>
#include <stdio.h>

#include <functional>
#include <map>

#include <3ds.h>

#define GET_BITS(v, s, e) (((v) >> (s)) & ((1 << ((e) - (s) + 1)) - 1))

static u32* socBuffer;

Result socInit() {
    socBuffer = (u32*) memalign(0x1000, 0x100000);
    if(socBuffer == NULL) {
        return -1;
    }

    Result socResult = SOC_Initialize(socBuffer, 0x100000);
    if(socResult != 0) {
        free(socBuffer);
        socBuffer = NULL;
        return socResult;
    }

    return 0;
}

void socCleanup() {
    SOC_Shutdown();
    if(socBuffer != NULL) {
        free(socBuffer);
        socBuffer = NULL;
    }
}

static u32* irBuffer;

Result irInit() {
    irBuffer = (u32*) memalign(0x1000, 0x1000);
    if(irBuffer == NULL) {
        return -1;
    }

    Result irResult = IRU_Initialize(irBuffer, 0x1000);
    if(irResult != 0) {
        free(irBuffer);
        irBuffer = NULL;
        return irResult;
    }

    return 0;
}

void irCleanup() {
    IRU_Shutdown();
    if(irBuffer != NULL) {
        free(irBuffer);
        irBuffer = NULL;
    }
}

static std::map<std::string, std::function<void()>> services;

void serviceCleanup() {
    for(std::map<std::string, std::function<void()>>::iterator it = services.begin(); it != services.end(); it++) {
        if((*it).second != NULL) {
            (*it).second();
        }
    }

    services.clear();
}

bool serviceCheckNew3DS() {
    if(osGetKernelVersion() >= SYSTEM_VERSION(2, 44, 6)) {
        u8 isNew3DS = 0;
        Result result = APT_CheckNew3DS(NULL, &isNew3DS);
        if(result == 0) {
            return isNew3DS != 0;
        }
    }

    return false;
}

bool serviceRequire(const std::string service) {
    if(services.find(service) != services.end()) {
        return true;
    }

    Result result = 0;
    std::function<void()> cleanup = NULL;
    if(service.compare("gfx") == 0) {
        result = (gfxInitDefault(), 0);
        cleanup = &gfxExit;
    } else if(service.compare("soc") == 0) {
        result = socInit();
        cleanup = &socCleanup;
    } else if(service.compare("ir") == 0) {
        result = irInit();
        cleanup = &irCleanup;
    } else if(service.compare("ac") == 0) {
        result = acInit();
        cleanup = &acExit;
    } else if(service.compare("ptm") == 0) {
        result = ptmInit();
        cleanup = &ptmExit;
    } else if(service.compare("kernel") == 0) {
        result = khaxInit();
        cleanup = &khaxExit;
    } else {
        if(!platformIsNinjhax() || (service.compare("csnd") == 0 && !serviceCheckNew3DS()) || serviceRequire("kernel")) {
            if(service.compare("am") == 0) {
                result = amInit();
                cleanup = &amExit;
            } else if(service.compare("csnd") == 0) {
                result = csndInit();
                cleanup = &csndExit;
            } else if(service.compare("nor") == 0) {
                result = CFGNOR_Initialize(1);
                cleanup = &CFGNOR_Shutdown;
            }
        } else {
            return false;
        }
    }

    if(result == 0) {
        services[service] = cleanup;
    } else {
        platformSetError(serviceParseError((u32) result));
    }

    return result == 0;
}

Error serviceParseError(u32 error) {
    Error err;

    err.raw = error;
    err.module = (ErrorModule) GET_BITS(error, 10, 17);
    err.level = (ErrorLevel) GET_BITS(error, 27, 31);
    err.summary = (ErrorSummary) GET_BITS(error, 21, 26);
    err.description = (ErrorDescription) GET_BITS(error, 0, 9);

    return err;
}