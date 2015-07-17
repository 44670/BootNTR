#include "ctrcommon/app.hpp"

#include "service.hpp"

#include <sys/errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iomanip>
#include <sstream>

#include <3ds.h>
#include <ctrcommon/app.hpp>

u8 appMediatypeToByte(MediaType mediaType) {
    return mediaType == NAND ? mediatype_NAND : mediatype_SDMC;
}

AppPlatform appPlatformFromId(u16 id) {
    switch(id) {
        case 1:
            return PLATFORM_WII;
        case 3:
            return PLATFORM_DSI;
        case 4:
            return PLATFORM_3DS;
        case 5:
            return PLATFORM_WIIU;
        default:
            return PLATFORM_UNKNOWN;
    }
}

AppCategory appCategoryFromId(u16 id) {
    if((id & 0x8000) == 0x8000) {
        return CATEGORY_TWL;
    } else if((id & 0x10) == 0x10) {
        return CATEGORY_SYSTEM;
    } else if((id & 0x6) == 0x6) {
        return CATEGORY_PATCH;
    } else if((id & 0x4) == 0x4) {
        return CATEGORY_DLC;
    } else if((id & 0x2) == 0x2) {
        return CATEGORY_DEMO;
    }

    return CATEGORY_APP;
}

const std::string appGetResultString(AppResult result) {
    std::stringstream resultMsg;
    if(result == APP_SUCCESS) {
        resultMsg << "Operation succeeded.";
    } else if(result == APP_PROCESS_CLOSING) {
        resultMsg << "Process closing.";
    } else if(result == APP_OPERATION_CANCELLED) {
        resultMsg << "Operation cancelled.";
    } else if(result == APP_AM_INIT_FAILED) {
        resultMsg << "Could not initialize AM service.";
    } else if(result == APP_IO_ERROR) {
        resultMsg << "I/O Error." << "\n" << strerror(errno);
    } else if(result == APP_OPEN_FILE_FAILED) {
        resultMsg << "Could not open file." << "\n" << strerror(errno);
    } else if(result == APP_BEGIN_INSTALL_FAILED) {
        resultMsg << "Could not begin installation." << "\n" << platformGetErrorString(platformGetError());
    } else if(result == APP_INSTALL_ERROR) {
        resultMsg << "Could not install app." << "\n" << platformGetErrorString(platformGetError());
    } else if(result == APP_FINALIZE_INSTALL_FAILED) {
        resultMsg << "Could not finalize installation." << "\n" << platformGetErrorString(platformGetError());
    } else if(result == APP_DELETE_FAILED) {
        resultMsg << "Could not delete app." << "\n" << platformGetErrorString(platformGetError());
    } else if(result == APP_LAUNCH_FAILED) {
        resultMsg << "Could not launch app." << "\n" << platformGetErrorString(platformGetError());
    } else {
        resultMsg << "Unknown error.";
    }

    return resultMsg.str();
}

const std::string appGetPlatformName(AppPlatform platform) {
    switch(platform) {
        case PLATFORM_WII:
            return "Wii";
        case PLATFORM_DSI:
            return "DSi";
        case PLATFORM_3DS:
            return "3DS";
        case PLATFORM_WIIU:
            return "Wii U";
        default:
            return "Unknown";
    }
}

const std::string appGetCategoryName(AppCategory category) {
    switch(category) {
        case CATEGORY_APP:
            return "App";
        case CATEGORY_DEMO:
            return "Demo";
        case CATEGORY_DLC:
            return "DLC";
        case CATEGORY_PATCH:
            return "Patch";
        case CATEGORY_SYSTEM:
            return "System";
        case CATEGORY_TWL:
            return "TWL";
        default:
            return "Unknown";
    }
}

App appGetCiaInfo(const std::string file, MediaType mediaType) {
    if(!serviceRequire("am")) {
        return {};
    }

    FS_archive archive = (FS_archive) {ARCH_SDMC, (FS_path) {PATH_EMPTY, 1, (u8*) ""}};
    Result archiveResult = FSUSER_OpenArchive(NULL, &archive);
    if(archiveResult != 0) {
        platformSetError(serviceParseError((u32) archiveResult));
        return {};
    }

    Handle handle = 0;
    Result openResult = FSUSER_OpenFile(NULL, &handle, archive, FS_makePath(PATH_CHAR, file.c_str()), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
    if(openResult != 0) {
        platformSetError(serviceParseError((u32) openResult));
        return {};
    }

    TitleList titleInfo;
    Result infoResult = AM_GetCiaFileInfo(appMediatypeToByte(mediaType), &titleInfo, handle);
    if(infoResult != 0) {
        platformSetError(serviceParseError((u32) infoResult));
        return {};
    }

    FSFILE_Close(handle);
    FSUSER_CloseArchive(NULL, &archive);

    App app;
    app.titleId = titleInfo.titleID;
    app.uniqueId = ((u32*) &titleInfo.titleID)[0];
    strcpy(app.productCode, "<N/A>");
    app.mediaType = mediaType;
    app.platform = appPlatformFromId(((u16*) &titleInfo.titleID)[3]);
    app.category = appCategoryFromId(((u16*) &titleInfo.titleID)[2]);
    app.version = titleInfo.titleVersion;
    app.size = titleInfo.size;
    return app;
}

bool appIsInstalled(App app) {
    if(!serviceRequire("am")) {
        return false;
    }

    u32 titleCount;
    Result titleCountResult = AM_GetTitleCount(appMediatypeToByte(app.mediaType), &titleCount);
    if(titleCountResult != 0) {
        platformSetError(serviceParseError((u32) titleCountResult));
        return false;
    }

    u64 titleIds[titleCount];
    Result titleIdsResult = AM_GetTitleIdList(appMediatypeToByte(app.mediaType), titleCount, titleIds);
    if(titleIdsResult != 0) {
        platformSetError(serviceParseError((u32) titleIdsResult));
        return false;
    }

    for(u32 i = 0; i < titleCount; i++) {
        if(titleIds[i] == app.titleId) {
            return true;
        }
    }

    return false;
}

std::vector<App> appList(MediaType mediaType) {
    std::vector<App> titles;
    if(!serviceRequire("am")) {
        return titles;
    }

    u32 titleCount;
    Result titleCountResult = AM_GetTitleCount(appMediatypeToByte(mediaType), &titleCount);
    if(titleCountResult != 0) {
        platformSetError(serviceParseError((u32) titleCountResult));
        return titles;
    }

    u64 titleIds[titleCount];
    Result titleIdsResult = AM_GetTitleIdList(appMediatypeToByte(mediaType), titleCount, titleIds);
    if(titleIdsResult != 0) {
        platformSetError(serviceParseError((u32) titleIdsResult));
        return titles;
    }

    TitleList titleList[titleCount];
    Result titleListResult = AM_ListTitles(appMediatypeToByte(mediaType), titleCount, titleIds, titleList);
    if(titleListResult != 0) {
        platformSetError(serviceParseError((u32) titleListResult));
        return titles;
    }

    for(u32 i = 0; i < titleCount; i++) {
        u64 titleId = titleList[i].titleID;

        App app;
        app.titleId = titleId;
        app.uniqueId = ((u32*) &titleId)[0];
        AM_GetTitleProductCode(appMediatypeToByte(mediaType), titleId, app.productCode);
        if(strcmp(app.productCode, "") == 0) {
            strcpy(app.productCode, "<N/A>");
        }

        app.mediaType = mediaType;
        app.platform = appPlatformFromId(((u16*) &titleId)[3]);
        app.category = appCategoryFromId(((u16*) &titleId)[2]);
        app.version = titleList[i].titleVersion;
        app.size = titleList[i].size;

        titles.push_back(app);
    }

    return titles;
}

AppResult appInstallFile(MediaType mediaType, const std::string path, std::function<bool(u64 pos, u64 totalSize)> onProgress) {
    if(!serviceRequire("am")) {
        return APP_AM_INIT_FAILED;
    }

    FILE* fd = fopen(path.c_str(), "r");
    if(!fd) {
        return APP_OPEN_FILE_FAILED;
    }

    struct stat st;
    stat(path.c_str(), &st);
    u64 size = (u64) (u32) st.st_size;

    AppResult ret = appInstall(mediaType, fd, size, onProgress);

    fclose(fd);
    return ret;
}

AppResult appInstall(MediaType mediaType, FILE* fd, u64 size, std::function<bool(u64 pos, u64 totalSize)> onProgress) {
    if(!serviceRequire("am")) {
        return APP_AM_INIT_FAILED;
    }

    if(onProgress != NULL) {
        onProgress(0, size);
    }

    Handle ciaHandle;
    Result startResult = AM_StartCiaInstall(appMediatypeToByte(mediaType), &ciaHandle);
    if(startResult != 0) {
        platformSetError(serviceParseError((u32) startResult));
        return APP_BEGIN_INSTALL_FAILED;
    }

    u32 bufSize = 1024 * 128; // 128KB
    void* buf = malloc(bufSize);
    bool cancelled = false;
    u64 pos = 0;
    while(platformIsRunning()) {
        if(onProgress != NULL && !onProgress(pos, size)) {
            cancelled = true;
            break;
        }

        size_t bytesRead = fread(buf, 1, bufSize, fd);
        if(bytesRead > 0) {
            Result writeResult = FSFILE_Write(ciaHandle, NULL, pos, buf, (u32) bytesRead, FS_WRITE_NOFLUSH);
            if(writeResult != 0) {
                AM_CancelCIAInstall(&ciaHandle);
                platformSetError(serviceParseError((u32) writeResult));
                return APP_INSTALL_ERROR;
            }

            pos += bytesRead;
        }

        if((ferror(fd) && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINPROGRESS) || (size != 0 && pos == size)) {
            break;
        }
    }

    free(buf);

    if(cancelled) {
        AM_CancelCIAInstall(&ciaHandle);
        return APP_OPERATION_CANCELLED;
    }

    if(!platformIsRunning()) {
        AM_CancelCIAInstall(&ciaHandle);
        return APP_PROCESS_CLOSING;
    }

    if(size != 0 && pos != size) {
        AM_CancelCIAInstall(&ciaHandle);
        return APP_IO_ERROR;
    }

    if(onProgress != NULL) {
        onProgress(size, size);
    }

    Result finishResult = AM_FinishCiaInstall(appMediatypeToByte(mediaType), &ciaHandle);
    if(finishResult != 0) {
        platformSetError(serviceParseError((u32) finishResult));
        return APP_FINALIZE_INSTALL_FAILED;
    }

    return APP_SUCCESS;
}

AppResult appDelete(App app) {
    if(!serviceRequire("am")) {
        return APP_AM_INIT_FAILED;
    }

    Result res = AM_DeleteTitle(appMediatypeToByte(app.mediaType), app.titleId);
    if(res != 0) {
        platformSetError(serviceParseError((u32) res));
        return APP_DELETE_FAILED;
    }

    return APP_SUCCESS;
}

AppResult appLaunch(App app) {
    u8 buf0[0x300];
    u8 buf1[0x20];

    aptOpenSession();
    Result prepareResult = APT_PrepareToDoAppJump(NULL, 0, app.titleId, appMediatypeToByte(app.mediaType));
    if(prepareResult != 0) {
        aptCloseSession();
        platformSetError(serviceParseError((u32) prepareResult));
        return APP_LAUNCH_FAILED;
    }

    Result doResult = APT_DoAppJump(NULL, 0x300, 0x20, buf0, buf1);
    if(doResult != 0) {
        aptCloseSession();
        platformSetError(serviceParseError((u32) doResult));
        return APP_LAUNCH_FAILED;
    }

    aptCloseSession();
    return APP_SUCCESS;
}