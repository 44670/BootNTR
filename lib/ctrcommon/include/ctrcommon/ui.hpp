#ifndef __CTRCOMMON_UI_HPP__
#define __CTRCOMMON_UI_HPP__

#include "ctrcommon/app.hpp"
#include "ctrcommon/gpu.hpp"
#include "ctrcommon/types.hpp"

#include <functional>
#include <string>
#include <vector>

typedef struct {
    std::string id;
    std::string name;
    std::vector<std::string> details;
} SelectableElement;

typedef struct {
    FILE* fd;
    u64 fileSize;
} RemoteFile;

bool uiSelect(SelectableElement* selected, std::vector<SelectableElement> elements, std::function<bool(std::vector<SelectableElement> &currElements, SelectableElement currElement, bool &elementsDirty, bool &resetCursorIfDirty)> onLoop, std::function<bool(SelectableElement select)> onSelect, bool useTopScreen = true, bool alphabetize = true, bool dpadPageScroll = true);
bool uiSelectFile(std::string* selectedFile, const std::string rootDirectory, std::vector<std::string> extensions, std::function<bool(const std::string currDirectory, bool inRoot, bool &updateList)> onLoop, std::function<bool(const std::string path, bool &updateList)> onSelect, bool useTopScreen = true, bool dpadPageScroll = true);
bool uiSelectApp(App* selectedApp, MediaType mediaType, std::function<bool(bool &updateList)> onLoop, std::function<bool(App app, bool &updateList)> onSelect, bool useTopScreen = true, bool dpadPageScroll = true);
void uiDisplayMessage(Screen screen, const std::string message);
bool uiPrompt(Screen screen, const std::string message, bool question);
void uiDisplayProgress(Screen screen, const std::string operation, const std::string details, bool quickSwap, u32 progress);
RemoteFile uiAcceptRemoteFile(Screen screen, std::function<void(std::stringstream& infoStream)> onWait = [&](std::stringstream& infoStream){});

#endif
