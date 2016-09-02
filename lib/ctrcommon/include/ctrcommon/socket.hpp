#ifndef __CTRCOMMON_SOCKET_HPP__
#define __CTRCOMMON_SOCKET_HPP__

#include "ctrcommon/types.hpp"

#include <stdio.h>

#include <string>

u64 htonll(u64 value);
u64 ntohll(u64 value);
u32 socketGetHostIP();
int socketListen(u16 port);
FILE* socketAccept(int listeningSocket, std::string* acceptedIp = NULL);
FILE* socketConnect(const std::string ipAddress, u16 port, int timeout = 10);

#endif
