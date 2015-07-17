#include "ctrcommon/ir.hpp"

#include "service.hpp"

#include <3ds.h>

u32 irGetState() {
    if(!serviceRequire("ir")) {
        return 0;
    }

    u32 state;
    IRU_GetIRLEDRecvState(&state);
    return state;
}

void irSetState(u32 state) {
    if(!serviceRequire("ir")) {
        return;
    }

    IRU_SetIRLEDState(state);
}