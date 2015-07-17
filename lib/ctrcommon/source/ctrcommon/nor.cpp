#include "ctrcommon/nor.hpp"
#include "ctrcommon/platform.hpp"

#include "service.hpp"

#include <3ds.h>

bool norRead(u32 offset, void* data, u32 size) {
    if(!serviceRequire("nor")) {
        return false;
    }

    Result result = CFGNOR_ReadData(offset, (u32*) data, size);
    if(result != 0) {
        platformSetError(serviceParseError((u32) result));
    }

    return result == 0;
}

bool norWrite(u32 offset, void* data, u32 size) {
    if(!serviceRequire("nor")) {
        return false;
    }

    Result result = CFGNOR_WriteData(offset, (u32*) data, size);
    if(result != 0) {
        platformSetError(serviceParseError((u32) result));
    }

    return result == 0;
}