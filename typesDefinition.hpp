#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

using SdlWindow   = std::unique_ptr<SDL_Window, void (*)(SDL_Window*)>;
using SdlRenderer = std::unique_ptr<SDL_Renderer, void (*)(SDL_Renderer*)>;
using SdlTexture  = std::unique_ptr<SDL_Texture, void (*)(SDL_Texture*)>;
using SdlFont     = std::unique_ptr<TTF_Font, void (*)(TTF_Font*)>;

using Color = std::array<Uint8, 3>;

struct ConfigStruct {
    std::unordered_map<std::string, std::string> keyCommands;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConfigStruct, keyCommands);

struct SdlAnimation {
    std::vector<SdlTexture> frames;
    int actualFrame{0};
    int fps{24};
};

struct WindowSettings {
    std::string Title{};
    int width{100};
    int height{100};
    bool borderless{true};
    bool fullscreen{false};

    bool useCacheFile{true};
    bool outputFilename{false};
    bool useBilinearInterpolation{true};
};

struct Style {
    int thumbnailSize{100};
    int padding{20};
    Color backgroundColor{30};
    int fontSize{14};

    int currentImageWidthBorder{10};
    Color currentImageColorBorder{255, 109, 18};

    int selectedImageWidthBorder{5};
    Color selectedImageColorBorder{43, 150, 18};
};

struct GridImagesState {
    int numColumns{1};
    int numRows{1};
    int rowsScroll{0};
};

struct ImageViewerState {
    bool fitHeight{true};
    bool fitWidth{false};
    bool flipVertical{false};
    bool flipHorizontal{false};

    // rotation will be 0, 1, 2 or 3. It will be used as 90*rotation degrees
    int rotation{0};
    float zoom{1.};
    int panningX{0};
    int panningY{0};
};

struct ImageHeader {
    std::optional<SdlTexture> image{std::nullopt};
    std::optional<SdlTexture> thumbnail{std::nullopt};
    std::optional<SdlAnimation> animation{std::nullopt};

    long memory{10};
    int width{6000};
    int height{6000};
    std::string fileAdress{""};
};

struct SdlContext {
    SdlWindow window;
    SdlRenderer renderer;
    std::optional<SdlFont> font{std::nullopt};
    WindowSettings windowSettings{};
    Style style{};

    GridImagesState gridImagesState;
    ImageViewerState imageViewerState;

    std::vector<ImageHeader> imagesVector;
    std::unordered_set<std::size_t> imagesToLoad;
    bool isGridImages{true};
    bool showBar{true};
    bool contiguousView{false};
    int fps{24};
    int currentImage{0};
    std::unordered_set<std::size_t> selectedImages{};
    bool exit{false};

    ConfigStruct configStruct;
};
