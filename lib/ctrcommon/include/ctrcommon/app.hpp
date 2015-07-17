#ifndef __CTRCOMMON_APP_HPP__
#define __CTRCOMMON_APP_HPP__

#include "ctrcommon/fs.hpp"

#include <functional>
#include <string>
#include <vector>

typedef enum {
    PLATFORM_WII,
    PLATFORM_DSI,
    PLATFORM_3DS,
    PLATFORM_WIIU,
    PLATFORM_UNKNOWN
} AppPlatform;

// TODO: verify categories.
typedef enum {
    CATEGORY_APP,
    CATEGORY_DEMO,
    CATEGORY_DLC,
    CATEGORY_PATCH,
    CATEGORY_SYSTEM,
    CATEGORY_TWL
} AppCategory;

typedef struct {
    u64 titleId;
    u32 uniqueId;
    char productCode[16];
    MediaType mediaType;
    AppPlatform platform;
    AppCategory category;
    u16 version;
    u64 size;
} App;

typedef enum {
    APP_SUCCESS,
    APP_PROCESS_CLOSING,
    APP_OPERATION_CANCELLED,
    APP_AM_INIT_FAILED,
    APP_IO_ERROR,
    APP_OPEN_FILE_FAILED,
    APP_BEGIN_INSTALL_FAILED,
    APP_INSTALL_ERROR,
    APP_FINALIZE_INSTALL_FAILED,
    APP_DELETE_FAILED,
    APP_LAUNCH_FAILED
} AppResult;

const std::string appGetResultString(AppResult result);
const std::string appGetPlatformName(AppPlatform platform);
const std::string appGetCategoryName(AppCategory category);
App appGetCiaInfo(const std::string file, MediaType mediaType);
bool appIsInstalled(App app);
std::vector<App> appList(MediaType mediaType);
AppResult appInstallFile(MediaType mediaType, const std::string path, std::function<bool(u64 pos, u64 totalSize)> onProgress);
AppResult appInstall(MediaType mediaType, FILE* fd, u64 size, std::function<bool(u64 pos, u64 totalSize)> onProgress);
AppResult appDelete(App app);
AppResult appLaunch(App app);

#endif