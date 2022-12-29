#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "cacheFilenames.hpp"
#include "ImageLoaderPolicy.hpp"
#include "ImageViewerApp.hpp"
#include "command.hpp"
#include "createFont.hpp"
#include "filesUtils.hpp"
#include "parseArguments.hpp"
#include "typesDefinition.hpp"
#include "sdlUtils.hpp"


int main(int argc, char* argv[]) {
    Style style{};
    if (TTF_Init() == -1) {
        assert(false && "An error occurred while initializing SDL_ttf");
        return -1;
    }
    std::vector<std::string> fontNames = {"DejaVu Sans", "Arial",
                                          "Liberation Sans", "FreeMono"};
    auto font                          = createFont(fontNames, style.fontSize);

    if (!font) {
        std::cout << "Font not found" << std::endl;
    }

    WindowSettings setting{};
    auto window         = createSdlWindow(setting);
    auto renderer       = createSdlRenderer(window);
    auto [files, flags] = parseLineArguments(argc, argv);
    files               = expandInputFiles(files);
    SdlContext context{std::move(window), std::move(renderer), std::move(font),
                       setting};
    context.style = style;
    CacheFilenames cacheFilenames;
    context.currentImage = cacheFilenames.existsString(files);
    if (files.size() == 0) {
        return;
    }
    context.imagesVector.reserve(files.size());
    for (auto& filename : files) {
        ImageHeader ih;
        ih.fileAdress = std::move(filename);
        context.imagesVector.emplace_back(std::move(ih));
    }
    if (context.imagesVector.size() >= 0) {
        if (context.imagesVector.size() == 1) {
            context.isGridImages = false;
        }
        ImageViewerApp app(std::move(context));
        app.mainLoop();
    }

    SDL_Quit();
    return 0;
}




