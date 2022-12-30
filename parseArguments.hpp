#pragma once

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "argparse.hpp"
#include "cacheFilenames.hpp"
#include "createFont.hpp"
#include "filesUtils.hpp"
#include "sdlUtils.hpp"
#include "typesDefinition.hpp"

#ifdef _WIN32
// Maybe some window libraries
#else
#include <fcntl.h>
#include <fstream>
#include <sys/fcntl.h>
#endif

SdlContext parseCommandLineArguments(int argc, char** argv) {
    SdlWindow window(nullptr, [](SDL_Window* window) {});
    SdlRenderer renderer(nullptr, [](SDL_Renderer* renderer) {});
    SdlContext sdlContext{std::move(window), std::move(renderer)};

    argparse::ArgumentParser parser("aiv");

    parser.add_argument("-p")
        .help("Use nearest pixel interpolation")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("-o")
        .help(
            "Print selected image filename to standart output on program exit")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("-c")
        .help("Disable the use of cache file for the last viewed image")
        .default_value(false)
        .implicit_value(true);

    auto args = parser.parse_known_args(argc, argv);

    if (parser["-p"] == true) {
        sdlContext.windowSettings.useBilinearInterpolation = false;
    }
    if (parser["-o"] == true) {
        sdlContext.windowSettings.outputFilename = true;
    }
    if (parser["-c"] == true) {
        sdlContext.windowSettings.useCacheFile = false;
    }

    return sdlContext;
}

std::vector<std::string> getFilenamesFromArguments(int argc, char** argv) {
    std::vector<std::string> args;
    // Iterate over the command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            args.push_back(argv[i]);
        }
    }

#ifdef _WIN32
    // TODO: Windows code to get input from pipeline in a non blocknig way
#else
    int flags2 = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags2 | O_NONBLOCK);

    std::string line;
    while (std::getline(std::cin, line)) {
        args.push_back(line);
    }
#endif
    return args;
}

SdlContext createSdlContext(int argc, char** argv) {
    SdlContext sdlContext = parseCommandLineArguments(argc, argv);
    Style style{};
    if (TTF_Init() == -1) {
        assert(false && "An error occurred while initializing SDL_ttf");
    }
    std::vector<std::string> fontNames = {"DejaVu Sans", "Arial",
                                          "Liberation Sans", "FreeMono"};
    auto font                          = createFont(fontNames, style.fontSize);

    if (!font) {
        std::cerr << "Font not found" << std::endl;
    }

    auto window   = createSdlWindow(sdlContext.windowSettings);
    auto renderer = createSdlRenderer(window);
    auto files    = getFilenamesFromArguments(argc, argv);
    files         = expandInputFiles(files);

    sdlContext.style = style;

    if (sdlContext.windowSettings.useCacheFile) {
        CacheFilenames cacheFilenames;
        sdlContext.currentImage = cacheFilenames.existsString(files);
    }
    if (files.size() == 0) {
        return;
    }
    sdlContext.imagesVector.reserve(files.size());
    for (auto& filename : files) {
        ImageHeader ih;
        ih.fileAdress = std::move(filename);
        sdlContext.imagesVector.emplace_back(std::move(ih));
    }
    sdlContext.window   = std::move(window);
    sdlContext.renderer = std::move(renderer);

    return sdlContext;
}
