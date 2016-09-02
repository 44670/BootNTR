#include "ctrcommon/fs.hpp"

#include "service.hpp"

#include <sys/dirent.h>
#include <stdio.h>

#include <algorithm>
#include <sstream>

#include <3ds.h>

u64 fsGetFreeSpace(MediaType mediaType) {
    u32 clusterSize;
    u32 freeClusters;
    Result res = 0;
    if(mediaType == NAND) {
        res = FSUSER_GetNandArchiveResource(NULL, NULL, &clusterSize, NULL, &freeClusters);
    } else {
        res = FSUSER_GetSdmcArchiveResource(NULL, NULL, &clusterSize, NULL, &freeClusters);
    }

    if(res != 0) {
        platformSetError(serviceParseError((u32) res));
        return 0;
    }

    return (u64) clusterSize * (u64) freeClusters;
}

bool fsExists(const std::string path) {
    FILE* fd = fopen(path.c_str(), "r");
    if(fd) {
        fclose(fd);
        return true;
    }

    return fsIsDirectory(path);
}

bool fsIsDirectory(const std::string path) {
    DIR* dir = opendir(path.c_str());
    if(dir) {
        closedir(dir);
        return true;
    }

    return false;
}

std::string fsGetExtension(const std::string path) {
    std::string::size_type dotPos = path.rfind('.');
    if(dotPos == std::string::npos) {
        return "";
    }

    return path.substr(dotPos + 1);
}

bool fsHasExtension(const std::string path, const std::string extension) {
    std::string::size_type dotPos = path.rfind('.');
    return dotPos != std::string::npos && path.substr(dotPos + 1).compare(extension) == 0;
}

bool fsHasExtensions(const std::string path, const std::vector<std::string> extensions) {
    std::string extension = fsGetExtension(path);
    return extensions.empty() || (extension.compare("") != 0 && std::find(extensions.begin(), extensions.end(), extension) != extensions.end());
}

std::vector<FileInfo> fsGetDirectoryContents(const std::string directory) {
    std::vector<FileInfo> result;
    bool hasSlash = directory.size() != 0 && directory[directory.size() - 1] == '/';
    const std::string dirWithSlash = hasSlash ? directory : directory + "/";

    DIR* dir = opendir(dirWithSlash.c_str());
    if(dir == NULL) {
        return result;
    }

    while(true) {
        struct dirent* ent = readdir(dir);
        if(ent == NULL) {
            break;
        }

        result.push_back({dirWithSlash + std::string(ent->d_name), std::string(ent->d_name)});
    }

    closedir(dir);
    return result;
}
