#pragma once

#include "typesDefinition.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <stdlib.h>
#include <unordered_map>

//******** Image viewer commands *********

void executeSystemCommand(SdlContext& sdlContext, const std::string& command);

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
void toggleSelectedImage(SdlContext& sdlContext, int num);

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

void toggleContiguousView(SdlContext& sdlContext, int num);

class CommandExecuter {
  public:
    CommandExecuter(ConfigStruct configStruct)
        : systemCommands{configStruct.keyCommands},
          generalCommands{{"f", toggleFullscreen},   {"n", nextImage},
                          {" ", toggleSelectedImage},          {"p", previousImage},
                          {"gg", goToImagePosition}, {"G", goLastImage},
                          {"r", reloadCurrentImage}, {"\n", toggleViewerState},
                          {"b", toggleBottomBar},    {"q", exitCommand}},

          gridImagesCommands{{"+", zoomUpGrid},   {"-", zoomDownGrid},
                             {"j", moveDownGrid}, {"k", moveUpGrid},
                             {"h", moveLeftGrid}, {"l", moveRightGrid}},

          imageViewerCommands{{"+", zoomUpViewer},
                              {"-", zoomDownViewer},
                              {"j", moveDownViewer},
                              {"k", moveUpViewer},
                              {"h", moveLeftViewer},
                              {"l", moveRightViewer},
                              {"e", fitWidth},
                              {"E", fitHeight},
                              {">", rotateRight},
                              {"<", rotateLeft},
                              {"c", toggleContiguousView}} {

        const auto identity = [](const auto& arg) { return arg; };

        const auto escapeSpecialCharacters = [](const auto& arg) {
            std::string escaped;
            for (char c : arg) {
                if (c == '.' || c == '[' || c == ']' || c == '(' || c == ')' ||
                    c == '{' || c == '}' || c == '\\' || c == '^' || c == '$' ||
                    c == '|' || c == '?' || c == '*' || c == '+') {
                    escaped += '\\';
                }
                escaped += c;
            }
            return escaped;
        };

        const auto wordToAllSubstring = [](const auto& arg) {
            std::string regex_string = "";
            for (int i = 0; i < arg.length(); i++) {
                // Check if the current character is an escape character
                if (arg[i] == '\\' && i + 1 < arg.length()) {
                    i++;
                }
                regex_string += arg.substr(0, i + 1) + "|";
            }
            // remove the last "|" from the regex string
            regex_string = regex_string.substr(0, regex_string.length() - 1);
            return regex_string;
        };

        const auto addKeyToString = [&](bool& first, const std::string& key,
                                        std::string& pattern, const auto& tr) {
            if (!first) {
                pattern += "|" + tr(escapeSpecialCharacters(key));
            } else {
                pattern += tr(escapeSpecialCharacters(key));
                first = false;
            }
        };

        const auto buildRegex = [&](const auto& map, const auto& tr) {
            std::string patternString = "^(\\d*)(";
            bool first{true};
            for (const auto& [key, _] : map) {
                addKeyToString(first, key, patternString, tr);
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

        regexGrid        = buildRegex(gridImagesCommands, identity);
        regexViewer      = buildRegex(imageViewerCommands, identity);
        maybeRegexGrid   = buildRegex(gridImagesCommands, wordToAllSubstring);
        maybeRegexViewer = buildRegex(imageViewerCommands, wordToAllSubstring);
        regexSystemCommands      = buildRegex(systemCommands, identity);
        maybeRegexSystemCommands = buildRegex(systemCommands, identity);
    }

    void matchCommand(SdlContext& context, std::string& inputCommand) {
        if (inputCommand == "") {
            return;
        }

        auto match = [&](const auto& regex) {
            std::smatch match;
            return std::regex_match(inputCommand, match, regex);
        };

        auto matchAndExecute = [&](const auto& regex, auto& commandMap) {
            std::smatch match;
            if (std::regex_match(inputCommand, match, regex)) {
                int number{0};
                std::string command;
                if (match[1].length() > 0) {
                    number = std::stoi(match[1]);
                }
                command = match[2];
                if (command.length() > 0) {
                    commandMap[command](context, number);
                    inputCommand = "";
                }
            }
        };

        bool maybeCommand = match(maybeRegexSystemCommands) ||
                            (context.isGridImages ? match(maybeRegexGrid)
                                                  : match(maybeRegexViewer));
        if (maybeCommand) {
            std::smatch match;
            if (std::regex_match(inputCommand, match, regexSystemCommands)) {
                executeSystemCommand(context, systemCommands[match[0]]);
                inputCommand = "";
            }
            if (context.isGridImages) {
                matchAndExecute(regexGrid, gridImagesCommands);
            } else {
                matchAndExecute(regexViewer, imageViewerCommands);
            }
        } else {
            inputCommand = "";
        }
    }

  private:
    std::regex regexGrid;
    std::regex regexViewer;
    std::regex maybeRegexGrid;
    std::regex maybeRegexViewer;
    std::regex maybeRegexSystemCommands;
    std::regex regexSystemCommands;

    std::unordered_map<std::string, std::string> systemCommands;

    std::unordered_map<std::string, void (*)(SdlContext&, int)> generalCommands;
    std::unordered_map<std::string, void (*)(SdlContext&, int)>
        gridImagesCommands;
    std::unordered_map<std::string, void (*)(SdlContext&, int)>
        imageViewerCommands;
};

// **************************************************************************
// ************************* Commands implementation ************************
// **************************************************************************

void executeSystemCommand(SdlContext& sdlContext, const std::string& command) {
	const auto& filename = sdlContext.imagesVector[sdlContext.currentImage].fileAdress;
	std::string filenames;
	for(const auto& s: sdlContext.selectedImages){
		filenames += sdlContext.imagesVector[s].fileAdress + " ";
	}

    setenv("AIV_CURRENT_IMAGE", filename.c_str(), 1);
	setenv("AIV_SELECTED_IMAGES", filenames.c_str(), 1);
    std::system(command.c_str());
}

void toggleFullscreen(SdlContext& sdlContext, int num) {
    sdlContext.windowSettings.fullscreen =
        !sdlContext.windowSettings.fullscreen;
}

void toggleSelectedImage(SdlContext& sdlContext, int num){
	auto imageId = sdlContext.currentImage;
	auto& set = sdlContext.selectedImages;
	if(set.find(imageId) == set.end()){
		set.insert(imageId);
	}else{
		set.erase(imageId);
	}

	nextImage(sdlContext, num);
}

void nextImage(SdlContext& sdlContext, int num) {
    int size = (int)sdlContext.imagesVector.size();
    num      = num == 0 ? 1 : num;
    sdlContext.currentImage =
        std::clamp(sdlContext.currentImage + num, 0, size - 1);
}

void previousImage(SdlContext& sdlContext, int num) {
    int size = (int)sdlContext.imagesVector.size();
    num      = num == 0 ? 1 : num;
    sdlContext.currentImage =
        std::clamp(sdlContext.currentImage - num, 0, size - 1);
}

void goFirstImage(SdlContext& sdlContext, int num) {
    sdlContext.currentImage = 0;
}

void goLastImage(SdlContext& sdlContext, int num) {
    int size                = (int)sdlContext.imagesVector.size();
    sdlContext.currentImage = size - 1;
}

void goToImagePosition(SdlContext& sdlContext, int num) {
    int size                = (int)sdlContext.imagesVector.size();
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
    sdlContext.imageViewerState.zoom *= 1 / 1.2;
}

void toggleContiguousView(SdlContext& sdlContext, int num) {
    sdlContext.imageViewerState.panningY = 0;
    sdlContext.imageViewerState.panningX = 0;
    sdlContext.contiguousView            = !sdlContext.contiguousView;
}
