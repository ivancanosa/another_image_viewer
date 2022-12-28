#pragma once

#include "typesDefinition.hpp"
#include <algorithm>
#include <iostream>
#include <regex>
#include <unordered_map>

//******** General commands ***********

void toggleFullscreen(SdlContext& sdlContext, int num);
void nextImage(SdlContext& sdlContext, int num);
void previousImage(SdlContext& sdlContext, int num);
void goFirstImage(SdlContext& sdlContext, int num);
void goLastImage(SdlContext& sdlContext, int num);
void goToImagePosition(SdlContext& sdlContext, int num);
void reloadCurrentImage(SdlContext& sdlContext, int num);
void toggleBottomBar(SdlContext& sdlContext, int num);
void toggleViewerState(SdlContext& sdlContext, int num);
void exitCommand(SdlContext& sdlContext, int num);

//******** Grid images commands ***********

void zoomUpGrid(SdlContext& sdlContext, int num);
void zoomDownGrid(SdlContext& sdlContext, int num);

void moveUpGrid(SdlContext& sdlContext, int num);
void moveRightGrid(SdlContext& sdlContext, int num);
void moveDownGrid(SdlContext& sdlContext, int num);
void moveLeftGrid(SdlContext& sdlContext, int num);

//******** Image viewer commands *********

void moveUpViewer(SdlContext& sdlContext, int num);
void moveRightViewer(SdlContext& sdlContext, int num);
void moveDownViewer(SdlContext& sdlContext, int num);
void moveLeftViewer(SdlContext& sdlContext, int num);

void fitHeight(SdlContext& sdlContext, int num);
void fitWidth(SdlContext& sdlContext, int num);

void rotateRight(SdlContext& sdlContext, int num);
void rotateLeft(SdlContext& sdlContext, int num);

void zoomUpViewer(SdlContext& sdlContext, int num);
void zoomDownViewer(SdlContext& sdlContext, int num);

class CommandExecuter {
  public:
    CommandExecuter()
        : generalCommands{{"f", toggleFullscreen},   {"n", nextImage},
                          {" ", nextImage},          {"p", previousImage},
                          {"gg", goFirstImage},      {"g", goToImagePosition},
                          {"G", goLastImage},        {"r", reloadCurrentImage},
                          {"\n", toggleViewerState}, {"b", toggleBottomBar},
                          {"q", exitCommand}},

          gridImagesCommands{{"+", zoomUpGrid},   {"-", zoomDownGrid},
                             {"j", moveDownGrid}, {"k", moveUpGrid},
                             {"h", moveLeftGrid}, {"l", moveRightGrid}},

          imageViewerCommands{
              {"+", zoomUpViewer},   {"-", zoomDownViewer},
              {"j", moveDownViewer}, {"k", moveUpViewer},
              {"h", moveLeftViewer}, {"l", moveRightViewer},
              {"e", fitWidth},       {"E", fitHeight},
              {">", rotateRight},    {"<", rotateLeft},
          } {

        const auto addKeyToString = [](bool& first, const std::string& key,
                                       std::string& pattern) {
            if (!first) {
                if (key != "+") {
                    pattern += "|" + key;
                } else {
                    pattern += "|\\+";
                }
            } else {
                first = false;
                if (key != "+") {
                    pattern += key;
                } else {
                    pattern += "\\+";
                }
            }
        };

        const auto buildRegex = [&](const auto& map) {
            std::string patternString = "^(\\d*)(";
            bool first{true};
            for (const auto& [key, _] : map) {
                addKeyToString(first, key, patternString);
            }
            patternString += ")?$";
            return std::regex(patternString);
        };

        const auto addMap = [](const auto& inputMap, auto& outputMap) {
            for (const auto& [key, value] : inputMap) {
                outputMap[key] = value;
            }
        };

        addMap(generalCommands, gridImagesCommands);
        addMap(generalCommands, imageViewerCommands);

        regexGrid   = buildRegex(gridImagesCommands);
        regexViewer = buildRegex(imageViewerCommands);
    }

    void matchCommand(SdlContext& context, std::string& inputCommand) {
        if (inputCommand == "") {
            return;
        }

        auto matchAndExecute = [&](const auto& regex, auto& commandMap) {
            std::smatch match;
            if (std::regex_match(inputCommand, match, regex)) {
                int number{0};
                std::string command;
                if (match[0].length() > 1) {
                    number = std::stoi(match[1]);
                }
                command = match[2];
                if (command.length() > 0) {
                    commandMap[command](context, number);
                    inputCommand = "";
                }
            } else {
                inputCommand = "";
            }
        };

        if (context.isGridImages) {
            matchAndExecute(regexGrid, gridImagesCommands);
        } else {
            matchAndExecute(regexViewer, imageViewerCommands);
        }
    }

  private:
    std::regex regexGrid;
    std::regex regexViewer;

    std::unordered_map<std::string, void (*)(SdlContext&, int)> generalCommands;
    std::unordered_map<std::string, void (*)(SdlContext&, int)>
        gridImagesCommands;
    std::unordered_map<std::string, void (*)(SdlContext&, int)>
        imageViewerCommands;
};

// **************************************************************************
// ************************* Commands implementation ************************
// **************************************************************************

void toggleFullscreen(SdlContext& sdlContext, int num) {
    sdlContext.windowSettings.fullscreen =
        !sdlContext.windowSettings.fullscreen;
}

void nextImage(SdlContext& sdlContext, int num) {
    int size = sdlContext.imagesVector.size();
    num      = num == 0 ? 1 : num;
    sdlContext.currentImage =
        std::clamp(sdlContext.currentImage + num, 0, size - 1);
}

void previousImage(SdlContext& sdlContext, int num) {
    int size = sdlContext.imagesVector.size();
    num      = num == 0 ? 1 : num;
    sdlContext.currentImage =
        std::clamp(sdlContext.currentImage - num, 0, size - 1);
}

void goFirstImage(SdlContext& sdlContext, int num) {
    sdlContext.currentImage = 0;
}

void goLastImage(SdlContext& sdlContext, int num) {
    int size                = sdlContext.imagesVector.size();
    sdlContext.currentImage = size - 1;
}

void goToImagePosition(SdlContext& sdlContext, int num) {
    int size                = sdlContext.imagesVector.size();
    sdlContext.currentImage = std::clamp(num, 0, size - 1);
}

// TODO: reload
void reloadCurrentImage(SdlContext& sdlContext, int num) {
}

void toggleBottomBar(SdlContext& sdlContext, int num) {
    sdlContext.showBar = !sdlContext.showBar;
}

void toggleViewerState(SdlContext& sdlContext, int num) {
    sdlContext.isGridImages = !sdlContext.isGridImages;
}

void exitCommand(SdlContext& sdlContext, int num) {
    sdlContext.exit = true;
}

//******** Grid images commands ***********

void zoomUpGrid(SdlContext& sdlContext, int num) {
	int size = std::clamp(sdlContext.style.thumbnailSize + 20, 30, 400);
	sdlContext.style.thumbnailSize = size;
}

void zoomDownGrid(SdlContext& sdlContext, int num) {
	int size = std::clamp(sdlContext.style.thumbnailSize - 20, 30, 400);
	sdlContext.style.thumbnailSize = size;
}

void moveUpGrid(SdlContext& sdlContext, int num) {
    int cols = sdlContext.gridImagesState.numColumns;
    num      = num == 0 ? 1 : num;
    previousImage(sdlContext, num * cols);
}

void moveRightGrid(SdlContext& sdlContext, int num) {
    nextImage(sdlContext, num);
}

void moveDownGrid(SdlContext& sdlContext, int num) {
    int cols = sdlContext.gridImagesState.numColumns;
    num      = num == 0 ? 1 : num;
    nextImage(sdlContext, num * cols);
}

void moveLeftGrid(SdlContext& sdlContext, int num) {
    previousImage(sdlContext, num);
}

//******** Image viewer commands *********

void moveUpViewer(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.panningY += 100;
}

void moveRightViewer(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.panningX -= 100;
}

void moveDownViewer(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.panningY -= 100;
}

void moveLeftViewer(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.panningX += 100;
}

void fitHeight(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.panningX  = 0;
    sdlContext.imageViewerState.panningY  = 0;
    sdlContext.imageViewerState.fitHeight = true;
    sdlContext.imageViewerState.fitWidth  = false;
    sdlContext.imageViewerState.zoom      = 1.;
}

void fitWidth(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.panningX  = 0;
    sdlContext.imageViewerState.panningY  = 0;
    sdlContext.imageViewerState.fitHeight = false;
    sdlContext.imageViewerState.fitWidth  = true;
    sdlContext.imageViewerState.zoom      = 1.;
}

void rotateRight(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.rotation =
        (sdlContext.imageViewerState.rotation + 1) % 4;
}

void rotateLeft(SdlContext& sdlContext, int num) {
    int rot = sdlContext.imageViewerState.rotation - 1;
    rot     = rot < 0 ? 3 : rot;
    sdlContext.imageViewerState.rotation = rot;
}

void zoomUpViewer(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.fitHeight = false;
    sdlContext.imageViewerState.fitWidth  = false;
    sdlContext.imageViewerState.zoom *= 1.2;
}

void zoomDownViewer(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.fitHeight = false;
    sdlContext.imageViewerState.fitWidth  = false;
    sdlContext.imageViewerState.zoom *= 1/1.2;
}
