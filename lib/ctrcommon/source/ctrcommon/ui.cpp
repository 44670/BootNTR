#include "ctrcommon/ui.hpp"

#include "ctrcommon/app.hpp"
#include "ctrcommon/fs.hpp"
#include "ctrcommon/input.hpp"
#include "ctrcommon/platform.hpp"
#include "ctrcommon/socket.hpp"

#include <arpa/inet.h>
#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <string.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stack>
#include <ctrcommon/gpu.hpp>

struct uiAlphabetize {
    inline bool operator()(SelectableElement a, SelectableElement b) {
        return strcasecmp(a.name.c_str(), b.name.c_str()) < 0;
    }
};

u32 selectorTexture;
u32 selectorVbo;

void uiInit() {
    gpuCreateTexture(&selectorTexture);
    gpuTextureInfo(selectorTexture, 64, 64, PIXEL_RGBA8, TEXTURE_MIN_FILTER(FILTER_NEAREST) | TEXTURE_MAG_FILTER(FILTER_NEAREST));
    memset(gpuGetTextureData(selectorTexture), 0xFF, 64 * 64 * 4);

    gpuCreateVbo(&selectorVbo);
    gpuVboAttributes(selectorVbo, ATTRIBUTE(0, 3, ATTR_FLOAT) | ATTRIBUTE(1, 2, ATTR_FLOAT) | ATTRIBUTE(2, 4, ATTR_FLOAT), 3);

    const float vboData[] = {
            0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            320.0f, 0.0f, -0.1f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            320.0f, 12.0f, -0.1f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            320.0f, 12.0f, -0.1f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            0.0f, 12.0f, -0.1f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    };

    gpuVboData(selectorVbo, vboData, 6 * 9, PRIM_TRIANGLES);
}

void uiCleanup() {
    if(selectorTexture != 0) {
        gpuFreeTexture(selectorTexture);
        selectorTexture = 0;
    }

    if(selectorVbo != 0) {
        gpuFreeVbo(selectorVbo);
        selectorVbo = 0;
    }
}

bool uiSelect(SelectableElement* selected, std::vector<SelectableElement> elements, std::function<bool(std::vector<SelectableElement> &currElements, SelectableElement currElement, bool &elementsDirty, bool &resetCursorIfDirty)> onLoop, std::function<bool(SelectableElement select)> onSelect, bool useTopScreen, bool alphabetize, bool dpadPageScroll) {
    int cursor = 0;
    int scroll = 0;

    u32 selectionScroll = 0;
    u64 selectionScrollEndTime = 0;

    u64 lastScrollTime = 0;

    bool elementsDirty = false;
    bool resetCursorIfDirty = true;
    if(alphabetize) {
        std::sort(elements.begin(), elements.end(), uiAlphabetize());
    }

    Button leftScrollButton = dpadPageScroll ? BUTTON_LEFT : BUTTON_L;
    Button rightScrollButton = dpadPageScroll ? BUTTON_RIGHT : BUTTON_R;

    bool canPageUp = false;
    bool canPageDown = false;
    while(platformIsRunning()) {
        inputPoll();
        if(inputIsPressed(BUTTON_A)) {
            SelectableElement select = elements.at((u32) cursor);
            if(onSelect == NULL || onSelect(select)) {
                *selected = select;
                return true;
            }
        }

        if(canPageUp) {
            canPageUp = !inputIsReleased(leftScrollButton);
        } else if(inputIsPressed(leftScrollButton)) {
            canPageUp = true;
        }

        if(canPageDown) {
            canPageDown = !inputIsReleased(rightScrollButton);
        } else if(inputIsPressed(rightScrollButton)) {
            canPageDown = true;
        }

        if(inputIsHeld(BUTTON_DOWN) || inputIsHeld(BUTTON_UP) || (inputIsHeld(leftScrollButton) && canPageUp) || (inputIsHeld(rightScrollButton) && canPageDown)) {
            if(lastScrollTime == 0 || platformGetTime() - lastScrollTime >= 180) {
                if(inputIsHeld(BUTTON_DOWN) && cursor < (int) elements.size() - 1) {
                    cursor++;
                    if(cursor >= scroll + 20) {
                        scroll++;
                    }
                }

                if(canPageDown && inputIsHeld(rightScrollButton) && cursor < (int) elements.size() - 1) {
                    cursor += 20;
                    if(cursor >= (int) elements.size()) {
                        cursor = elements.size() - 1;
                        if(cursor < 0) {
                            cursor = 0;
                        }
                    }

                    scroll += 20;
                    if(scroll >= (int) elements.size() - 19) {
                        scroll = elements.size() - 20;
                        if(scroll < 0) {
                            scroll = 0;
                        }
                    }
                }

                if(inputIsHeld(BUTTON_UP) && cursor > 0) {
                    cursor--;
                    if(cursor < scroll) {
                        scroll--;
                    }
                }

                if(canPageUp && inputIsHeld(leftScrollButton) && cursor > 0) {
                    cursor -= 20;
                    if(cursor < 0) {
                        cursor = 0;
                    }

                    scroll -= 20;
                    if(scroll < 0) {
                        scroll = 0;
                    }
                }

                selectionScroll = 0;
                selectionScrollEndTime = 0;

                lastScrollTime = platformGetTime();
            }
        } else {
            lastScrollTime = 0;
        }

        gpuViewport(BOTTOM_SCREEN, 0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT);
        gputOrtho(0, BOTTOM_WIDTH, 0, BOTTOM_HEIGHT, -1, 1);
        gpuClear();

        u32 screenWidth = (u32) gpuGetViewportWidth();
        int screenHeight = gpuGetViewportHeight();
        for(std::vector<SelectableElement>::iterator it = elements.begin() + scroll; it != elements.begin() + scroll + 20 && it != elements.end(); it++) {
            SelectableElement element = *it;
            int index = it - elements.begin();
            u8 color = 255;
            int offset = 0;
            float itemHeight = gputGetStringHeight(element.name, 8) + 4;
            if(index == cursor) {
                color = 0;

                gputPushModelView();
                gputTranslate(0, (screenHeight - 1) - ((index - scroll + 1) * itemHeight), 0);
                gpuBindTexture(TEXUNIT0, selectorTexture);
                gpuDrawVbo(selectorVbo);
                gputPopModelView();

                u32 width = (u32) gputGetStringWidth(element.name, 8);
                if(width > screenWidth) {
                    if(selectionScroll + screenWidth >= width) {
                        if(selectionScrollEndTime == 0) {
                            selectionScrollEndTime = platformGetTime();
                        } else if(platformGetTime() - selectionScrollEndTime >= 4000) {
                            selectionScroll = 0;
                            selectionScrollEndTime = 0;
                        }
                    } else {
                        selectionScroll++;
                    }
                }

                offset = -selectionScroll;
            }

            gputDrawString(element.name, offset, (screenHeight - 1) - ((index - scroll + 1) * itemHeight) + 2, 8, 8, color, color, color);
        }

        gpuFlush();
        gpuFlushBuffer();

        gpuViewport(TOP_SCREEN, 0, 0, TOP_WIDTH, TOP_HEIGHT);
        gputOrtho(0, TOP_WIDTH, 0, TOP_HEIGHT, -1, 1);
        if(useTopScreen) {
            gpuClear();

            SelectableElement currSelected = elements.at((u32) cursor);
            if(currSelected.details.size() != 0) {
                std::stringstream details;
                for(std::vector<std::string>::iterator it = currSelected.details.begin(); it != currSelected.details.end(); it++) {
                    details << *it << "\n";
                }

                gputDrawString(details.str(), 0, gpuGetViewportHeight() - 1 - gputGetStringHeight(details.str(), 8), 8, 8);
            }
        }

        bool result = onLoop != NULL && onLoop(elements, elements.at((u32) cursor), elementsDirty, resetCursorIfDirty);
        if(elementsDirty) {
            if(resetCursorIfDirty) {
                cursor = 0;
                scroll = 0;
            } else if(cursor >= (int) elements.size()) {
                cursor = elements.size() - 1;
                if(cursor < 0) {
                    cursor = 0;
                }

                scroll = elements.size() - 20;
                if(scroll < 0) {
                    scroll = 0;
                }
            }

            selectionScroll = 0;
            selectionScrollEndTime = 0;
            if(alphabetize) {
                std::sort(elements.begin(), elements.end(), uiAlphabetize());
            }

            elementsDirty = false;
            resetCursorIfDirty = true;
        }

        if(useTopScreen) {
            gpuFlush();
            gpuFlushBuffer();
        }

        gpuSwapBuffers(true);
        if(result) {
            break;
        }
    }

    return false;
}

void uiGetDirContents(std::vector<SelectableElement> &elements, const std::string directory, std::vector<std::string> extensions) {
    elements.clear();
    elements.push_back({".", "."});
    elements.push_back({"..", ".."});

    std::vector<FileInfo> contents = fsGetDirectoryContents(directory);
    for(std::vector<FileInfo>::iterator it = contents.begin(); it != contents.end(); it++) {
        const std::string name = (*it).name;
        const std::string path = (*it).path;
        if(fsIsDirectory(path)) {
            elements.push_back({path, name});
        } else if(fsHasExtensions(path, extensions)) {
            struct stat st;
            stat(path.c_str(), &st);

            std::vector<std::string> info;
            std::stringstream stream;
            stream << "File Size: " << ((u32) st.st_size) << " bytes (" << std::fixed << std::setprecision(2) << ((u32) st.st_size) / 1024.0f / 1024.0f << "MB)";
            info.push_back(stream.str());

            std::string extension = fsGetExtension(path);
            if(extension.compare("cia") == 0) {
                App app = appGetCiaInfo(path, SD);

                std::stringstream titleId;
                titleId << "0x" << std::setfill('0') << std::setw(16) << std::hex << app.titleId;

                std::stringstream uniqueId;
                uniqueId << "0x" << std::setfill('0') << std::setw(8) << std::hex << app.uniqueId;

                std::stringstream version;
                version << "0x" << std::setfill('0') << std::hex << app.version;

                std::stringstream size;
                size << "" << app.size << " bytes (" << std::fixed << std::setprecision(2) << app.size / 1024.0f / 1024.0f << "MB)";

                info.push_back("Installed Size: " + size.str());
                info.push_back("Title ID: " + titleId.str());
                info.push_back("Unique ID: " + uniqueId.str());
                info.push_back("Product Code: " + std::string(app.productCode));
                info.push_back("Platform: " + appGetPlatformName(app.platform));
                info.push_back("Category: " + appGetCategoryName(app.category));
                info.push_back("Version: " + version.str());
            }

            elements.push_back({path, name, info});
        }
    }
}

bool uiSelectFile(std::string* selectedFile, const std::string rootDirectory, std::vector<std::string> extensions, std::function<bool(const std::string currDirectory, bool inRoot, bool &updateList)> onLoop, std::function<bool(const std::string path, bool &updateList)> onSelect, bool useTopScreen, bool dpadPageScroll) {
    std::stack<std::string> directoryStack;
    std::string currDirectory = rootDirectory;

    std::vector<SelectableElement> elements;
    uiGetDirContents(elements, currDirectory, extensions);

    bool updateContents = false;
    bool resetCursor = true;
    SelectableElement selected;
    bool result = uiSelect(&selected, elements, [&](std::vector<SelectableElement> &currElements, SelectableElement currElement, bool &elementsDirty, bool &resetCursorIfDirty) {
        if(onLoop != NULL && onLoop(currDirectory, directoryStack.empty(), updateContents)) {
            return true;
        }

        if(inputIsPressed(BUTTON_B) && !directoryStack.empty()) {
            currDirectory = directoryStack.top();
            directoryStack.pop();
            updateContents = true;
        }

        if(updateContents) {
            uiGetDirContents(currElements, currDirectory, extensions);
            elementsDirty = true;
            resetCursorIfDirty = resetCursor;
            updateContents = false;
            resetCursor = true;
        }

        return false;
    }, [&](SelectableElement select) {
        if(select.name.compare(".") == 0) {
            return false;
        } else if(select.name.compare("..") == 0) {
            if(!directoryStack.empty()) {
                currDirectory = directoryStack.top();
                directoryStack.pop();
                updateContents = true;
            }

            return false;
        } else if(fsIsDirectory(select.id)) {
            directoryStack.push(currDirectory);
            currDirectory = select.id;
            updateContents = true;
            return false;
        }

        bool updateList = false;
        bool ret = onSelect(select.id, updateList);
        if(updateList) {
            updateContents = true;
            resetCursor = false;
        }

        return ret;
    }, useTopScreen, true, dpadPageScroll);

    if(result) {
        *selectedFile = selected.id;
    }

    return result;
}

void uiGetApps(std::vector<SelectableElement> &elements, std::vector<App> apps) {
    elements.clear();
    for(std::vector<App>::iterator it = apps.begin(); it != apps.end(); it++) {
        App app = *it;

        std::stringstream titleId;
        titleId << "0x" << std::setfill('0') << std::setw(16) << std::hex << app.titleId;

        std::stringstream uniqueId;
        uniqueId << "0x" << std::setfill('0') << std::setw(8) << std::hex << app.uniqueId;

        std::stringstream version;
        version << "0x" << std::setfill('0') << std::hex << app.version;

        std::stringstream size;
        size << "" << app.size << " bytes (" << std::fixed << std::setprecision(2) << app.size / 1024.0f / 1024.0f << "MB)";

        std::vector<std::string> details;
        details.push_back("Title ID: " + titleId.str());
        details.push_back("Unique ID: " + uniqueId.str());
        details.push_back("Product Code: " + std::string(app.productCode));
        details.push_back("Platform: " + appGetPlatformName(app.platform));
        details.push_back("Category: " + appGetCategoryName(app.category));
        details.push_back("Version: " + version.str());
        details.push_back("Size: " + size.str());

        elements.push_back({titleId.str(), app.productCode, details});
    }

    if(elements.size() == 0) {
        elements.push_back({"None", "None"});
    }
}

bool uiFindApp(App* result, std::string id, std::vector<App> apps) {
    for(std::vector<App>::iterator it = apps.begin(); it != apps.end(); it++) {
        App app = *it;
        if(app.titleId == (u64) strtoll(id.c_str(), NULL, 16)) {
            *result = app;
            return true;
        }
    }

    return false;
}

bool uiSelectApp(App* selectedApp, MediaType mediaType, std::function<bool(bool &updateList)> onLoop, std::function<bool(App app, bool &updateList)> onSelect, bool useTopScreen, bool dpadPageScroll) {
    std::vector<SelectableElement> elements;

    std::vector<App> apps = appList(mediaType);
    uiGetApps(elements, apps);

    bool updateContents = false;
    SelectableElement selected;
    bool result = uiSelect(&selected, elements, [&](std::vector<SelectableElement> &currElements, SelectableElement currElement, bool &elementsDirty, bool &resetCursorIfDirty) {
        if(onLoop != NULL && onLoop(updateContents)) {
            return true;
        }

        if(updateContents) {
            apps = appList(mediaType);
            uiGetApps(currElements, apps);
            elementsDirty = true;
            resetCursorIfDirty = false;
            updateContents = false;
        }

        return false;
    }, [&](SelectableElement select) {
        if(select.name.compare("None") != 0) {
            App app;
            if(uiFindApp(&app, select.id, apps)) {
                bool updateList = false;
                bool ret = onSelect(app, updateList);
                if(updateList) {
                    updateContents = true;
                }

                return ret;
            }
        }

        return false;
    }, useTopScreen, true, dpadPageScroll);

    if(result) {
        App app;
        if(uiFindApp(&app, selected.id, apps)) {
            *selectedApp = app;
        }
    }

    return result;
}

void uiDisplayMessage(Screen screen, const std::string message) {
    gpuViewport(screen, 0, 0, screen == TOP_SCREEN ? TOP_WIDTH : BOTTOM_WIDTH, screen == TOP_SCREEN ? TOP_HEIGHT : BOTTOM_HEIGHT);
    gputOrtho(0, screen == TOP_SCREEN ? TOP_WIDTH : BOTTOM_WIDTH, 0, screen == TOP_SCREEN ? TOP_HEIGHT : BOTTOM_HEIGHT, -1, 1);

    gpuClear();
    gputDrawString(message, (gpuGetViewportWidth() - gputGetStringWidth(message, 8)) / 2, (gpuGetViewportHeight() - gputGetStringHeight(message, 8)) / 2, 8, 8);
    gpuFlush();
    gpuFlushBuffer();
    gpuSwapBuffers(true);

    gpuViewport(TOP_SCREEN, 0, 0, TOP_WIDTH, TOP_HEIGHT);
    gputOrtho(0, TOP_WIDTH, 0, TOP_HEIGHT, -1, 1);
}

bool uiPrompt(Screen screen, const std::string message, bool question) {
    std::stringstream stream;
    stream << message << "\n";
    if(question) {
        stream << "Press A to confirm, B to cancel." << "\n";
    } else {
        stream << "Press Start to continue." << "\n";
    }

    bool result = false;
    std::string str = stream.str();
    while(platformIsRunning()) {
        inputPoll();
        if(question) {
            if(inputIsPressed(BUTTON_A)) {
                result = true;
                break;
            }

            if(inputIsPressed(BUTTON_B)) {
                result = false;
                break;
            }
        } else {
            if(inputIsPressed(BUTTON_START)) {
                result = true;
                break;
            }
        }

        uiDisplayMessage(screen, str);
    }

    inputPoll();
    return result;
}

void uiDisplayProgress(Screen screen, const std::string operation, const std::string details, bool quickSwap, u32 progress) {
    std::stringstream stream;
    stream << operation << ": [";
    u32 progressBars = progress / 4;
    for(u32 i = 0; i < 25; i++) {
        if(i < progressBars) {
            stream << '|';
        } else {
            stream << ' ';
        }
    }

    stream << "] " << std::setfill(' ') << std::setw(3) << progress << "%" << "\n";
    stream << details << "\n";

    std::string str = stream.str();

    gpuViewport(screen, 0, 0, screen == TOP_SCREEN ? TOP_WIDTH : BOTTOM_WIDTH, screen == TOP_SCREEN ? TOP_HEIGHT : BOTTOM_HEIGHT);
    gputOrtho(0, screen == TOP_SCREEN ? TOP_WIDTH : BOTTOM_WIDTH, 0, screen == TOP_SCREEN ? TOP_HEIGHT : BOTTOM_HEIGHT, -1, 1);

    gpuClear();
    gputDrawString(str, (gpuGetViewportWidth() - gputGetStringWidth(str, 8)) / 2, (gpuGetViewportHeight() - gputGetStringHeight(str, 8)) / 2, 8, 8);
    gpuFlush();
    gpuFlushBuffer();
    gpuSwapBuffers(!quickSwap);

    gpuViewport(TOP_SCREEN, 0, 0, TOP_WIDTH, TOP_HEIGHT);
    gputOrtho(0, TOP_WIDTH, 0, TOP_HEIGHT, -1, 1);
}

RemoteFile uiAcceptRemoteFile(Screen screen, std::function<void(std::stringstream& infoStream)> onWait) {
    uiDisplayMessage(screen, "Initializing...");

    int listen = socketListen(5000);
    if(listen < 0) {
        std::stringstream errStream;
        errStream << "Failed to initialize." << "\n" << strerror(errno) << "\n";
        uiPrompt(screen, errStream.str(), false);
        return {NULL, 0};
    }

    std::stringstream baseInfoStream;
    baseInfoStream << "Waiting for peer to connect..." << "\n";
    baseInfoStream << "IP: " << inet_ntoa({socketGetHostIP()}) << "\n";
    baseInfoStream << "Press B to cancel." << "\n";
    std::string baseInfo = baseInfoStream.str();

    FILE* socket;
    while((socket = socketAccept(listen)) == NULL) {
        if(!platformIsRunning()) {
            close(listen);
            return {NULL, 0};
        }

        if(errno != EAGAIN && errno != EWOULDBLOCK && errno != EINPROGRESS) {
            close(listen);

            std::stringstream errStream;
            errStream << "Failed to accept peer." << "\n" << strerror(errno) << "\n";
            uiPrompt(screen, errStream.str(), false);

            return {NULL, 0};
        }

        inputPoll();
        if(inputIsPressed(BUTTON_B)) {
            close(listen);
            return {NULL, 0};
        }

        std::stringstream infoStream;
        infoStream << baseInfo;
        onWait(infoStream);
        uiDisplayMessage(screen, infoStream.str());
    }

    close(listen);

    uiDisplayMessage(screen, "Reading info...\nPress B to cancel.");

    u64 fileSize = 0;
    u64 bytesRead = 0;
    while(bytesRead < sizeof(fileSize)) {
        if(!platformIsRunning()) {
            fclose(socket);
            return {NULL, 0};
        }

        size_t currBytesRead = fread(&fileSize + bytesRead, 1, (size_t) (sizeof(fileSize) - bytesRead), socket);
        if(currBytesRead > 0) {
            bytesRead += currBytesRead;
        }

        if(ferror(socket) && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINPROGRESS) {
            fclose(socket);

            std::stringstream errStream;
            errStream << "Failed to read info." << "\n" << strerror(errno) << "\n";
            uiPrompt(screen, errStream.str(), false);

            return {NULL, 0};
        }

        inputPoll();
        if(inputIsPressed(BUTTON_B)) {
            fclose(socket);
            return {NULL, 0};
        }
    }

    fileSize = ntohll(fileSize);
    return {socket, fileSize};
}