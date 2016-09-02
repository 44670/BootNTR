#ifndef __CTRCOMMON_FS_HPP__
#define __CTRCOMMON_FS_HPP__

#include "ctrcommon/types.hpp"

#include <string>
#include <vector>

typedef enum {
    NAND,
    SD
} MediaType;

typedef struct {
    const std::string path;
    const std::string name;
} FileInfo;

u64 fsGetFreeSpace(MediaType mediaType);
bool fsExists(const std::string path);
bool fsIsDirectory(const std::string path);
std::string fsGetExtension(const std::string path);
bool fsHasExtension(const std::string path, const std::string extension);
bool fsHasExtensions(const std::string path, const std::vector<std::string> extensions);
std::vector<FileInfo> fsGetDirectoryContents(const std::string directory);

#endif
