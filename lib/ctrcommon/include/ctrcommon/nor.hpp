#ifndef __CTRCOMMON_NOR_HPP__
#define __CTRCOMMON_NOR_HPP__

#include "ctrcommon/types.hpp"

bool norRead(u32 offset, void* data, u32 size);
bool norWrite(u32 offset, void* data, u32 size);

#endif
