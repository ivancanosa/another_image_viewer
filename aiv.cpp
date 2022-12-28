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
#include "command.hpp"
#include "createFont.hpp"
#include "filesUtils.hpp"
#include "parseArguments.hpp"
#include "typesDefinition.hpp"

SdlWindow createSdlWindow(const WindowSettings& windowSettings);
SdlRenderer createSdlRenderer(const SdlWindow& sdlWindow);
SdlTexture createTexture(SDL_Texture* texture);

std::optional<SdlTexture> createTexture(const SdlRenderer& renderer,
                                        const std::string& filename);

std::optional<SdlTexture> createThumbnailWithSize(
    const SdlRenderer& renderer, const std::string& filename, int maxWidth,
    int maxHeight, int& returnWidht, int& returnHeight, long& returnMemory);

SdlTexture createThumbnail(const SdlRenderer& renderer,
                           const SdlTexture& texture, int maxWidth,
                           int maxHeight);

std::string memoryToHumanReadable(long bytes, int decimalPrecision = 2);

class ImageLoaderPolicy {
  public:
    ImageLoaderPolicy(int numImages) : loadedThumbnails(numImages, false) {
    }

    void loadInGrid(SdlContext& sdlContext) {
        int numColumns  = sdlContext.gridImagesState.numColumns;
        int numRows     = sdlContext.gridImagesState.numRows;
        int imageRow    = sdlContext.currentImage / numColumns;
        int imageColumn = sdlContext.currentImage % numColumns;
        int vscroll     = sdlContext.gridImagesState.rowsScroll;

        auto& v = loadedThumbnails;

        const auto getFrontierIterators = [&]() {
            auto firstIt = v.begin() + vscroll * numColumns;
            auto lastIt  = firstIt + numColumns * numRows;
            if (numColumns * numRows + vscroll * numColumns >= v.size()) {
                lastIt = v.end();
            }
            if (vscroll * numColumns >= v.size()) {
                firstIt = v.end();
            }
            auto rFirstIt = std::reverse_iterator<decltype(firstIt)>(firstIt);
            return std::make_tuple(rFirstIt, lastIt);
        };

        const auto isThumnailNotLoaded = [](const auto& value) {
            return !value;
        };

        const auto findUnloadedImageIterator = [&](auto rFirstIt, auto lastIt) {
            int currentLoadingPos = sdlContext.currentImage;
            if (sdlContext.currentImage == lastCurrentImage) {
                currentLoadingPos = lastLoadedThumbnail;
                lastCurrentImage  = sdlContext.currentImage;
            }

            // Forward loading from current image
            auto it = std::find_if(v.begin() + currentLoadingPos, lastIt,
                                   isThumnailNotLoaded);

            // Reverse loading from current image
            if (it == lastIt) {
                auto rit = std::find_if(std::rbegin(v) +
                                            (v.size() - currentLoadingPos - 1),
                                        rFirstIt, isThumnailNotLoaded);
                it       = (rit + 1).base();
            }
            return it;
        };

        const auto loadThumbnailLambda = [&](auto it) {
            std::size_t index =
                std::clamp((int)std::distance(v.begin(), it), 0, (int)v.size());
            if (loadedThumbnails[index]) {
                return;
            }
            int width, height;
			long memory;
            auto thumbnail = createThumbnailWithSize(
                sdlContext.renderer, sdlContext.imagesVector[index].fileAdress,
                100, 100, width, height, memory);
            lastLoadedThumbnail += 1;
            loadedThumbnails[index] = true;
            if (!thumbnail) {
                return;
            }

            sdlContext.imagesVector[index].thumbnail = std::move(thumbnail);
            sdlContext.imagesVector[index].width     = width;
            sdlContext.imagesVector[index].height    = height;
            sdlContext.imagesVector[index].memory     = memory;
        };

        const auto& [rFirstIt, lastIt] = getFrontierIterators();
        auto it = findUnloadedImageIterator(rFirstIt, lastIt);
        loadThumbnailLambda(it);
    }

    void loadInViewer(SdlContext& sdlContext) {
        int index = sdlContext.currentImage;
        if (lastLoadedImage != index) {
            if (lastLoadedImage != -1) {
                sdlContext.imagesVector[lastLoadedImage].image = std::nullopt;
            }
            auto texture = createTexture(
                sdlContext.renderer, sdlContext.imagesVector[index].fileAdress);

            if (texture) {
                SDL_Point size;
                SDL_QueryTexture(texture.value().get(), NULL, NULL, &size.x,
                                 &size.y);
                std::filesystem::path file_path(
                    sdlContext.imagesVector[index].fileAdress);
                std::size_t memory = std::filesystem::file_size(file_path);

                sdlContext.imagesVector[index].image  = std::move(texture);
                sdlContext.imagesVector[index].memory = memory;
                sdlContext.imagesVector[index].width  = size.x;
                sdlContext.imagesVector[index].height = size.y;
            }
            lastLoadedImage = index;
        }
    }

    void loadNext(SdlContext& sdlContext) {
        if (sdlContext.isGridImages) {
            loadInGrid(sdlContext);
        } else {
            loadInViewer(sdlContext);
        }
    }

  private:
    std::vector<bool> loadedThumbnails;
    std::vector<std::size_t> loadedImages;
    int lastCurrentImage{-1};
    int lastLoadedThumbnail{-1};
    int lastLoadedImage{-1};
};

class ImageViewerApp {
  public:
    ImageViewerApp(SdlContext&& sdlContextArg)
        : sdlContext(std::move(sdlContextArg)),
          imageLoaderPolicy(sdlContext.imagesVector.size()) {
    }

    // Function to draw a bar at the bottom of the window
    void drawBottomBarBackground() {
        // Get the window dimensions
        int windowWidth, windowHeight;
        SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);

        // Set the color for drawing the bar
        SDL_SetRenderDrawColor(sdlContext.renderer.get(), 10, 10, 10, 255);

        // Calculate the dimensions for the bar
        int barHeight    = sdlContext.style.fontSize * 4;
        int barY         = windowHeight - barHeight;
        SDL_Rect barRect = {0, barY, windowWidth, barHeight};

        // Draw the bar
        SDL_RenderFillRect(sdlContext.renderer.get(), &barRect);
    }

    // Function to write a string at the bottom left of the window
    void drawBottomLeftText(const std::string& text) {
        // Get the window dimensions
        int windowWidth, windowHeight;
        SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);

        // Set the color for drawing the text
        SDL_SetRenderDrawColor(sdlContext.renderer.get(), 255, 255, 255, 255);

        // Render the text
        SDL_Surface* textSurface = TTF_RenderText_Blended(
            sdlContext.font.value().get(), text.c_str(), {255, 255, 255});
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(
            sdlContext.renderer.get(), textSurface);
        int barHeight     = 50;
        int barY          = windowHeight - barHeight;
        SDL_Rect textRect = {10, barY + 10, textSurface->w, textSurface->h};
        SDL_RenderCopy(sdlContext.renderer.get(), textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);
        SDL_FreeSurface(textSurface);
    }

    // Function to write a string at the bottom right of the window
    void drawBottomRightText(const std::string& text) {
        // Get the window dimensions
        int windowWidth, windowHeight;
        SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);

        // Set the color for drawing the text
        SDL_SetRenderDrawColor(sdlContext.renderer.get(), 255, 255, 255, 255);

        // Render the text
        SDL_Surface* textSurface = TTF_RenderText_Blended(
            sdlContext.font.value().get(), text.c_str(), {255, 255, 255});
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(
            sdlContext.renderer.get(), textSurface);
        int barHeight     = 50;
        int barY          = windowHeight - barHeight;
        SDL_Rect textRect = {windowWidth - 10 - textSurface->w, barY + 10,
                             textSurface->w, textSurface->h};
        SDL_RenderCopy(sdlContext.renderer.get(), textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);
        SDL_FreeSurface(textSurface);
    }

    void drawBottomBar() {
        if (sdlContext.showBar && sdlContext.font) {
            drawBottomBarBackground();
            const auto& imageHeader =
                sdlContext.imagesVector[sdlContext.currentImage];
            std::string leftInfo =
                "size : " + memoryToHumanReadable(imageHeader.memory);
            leftInfo += ", " + std::to_string(imageHeader.width) + "x" +
                        std::to_string(imageHeader.height);
            leftInfo += ", address: " + imageHeader.fileAdress;
            std::string rightInfo =
                std::to_string(sdlContext.currentImage) + "/" +
                std::to_string(sdlContext.imagesVector.size() - 1);
            drawBottomLeftText(leftInfo);
            drawBottomRightText(rightInfo);
        }
    }

    void maybeToggleFullscreen() {
        if (wasFullscreen != sdlContext.windowSettings.fullscreen) {
            wasFullscreen   = sdlContext.windowSettings.fullscreen;
            auto screenMode = wasFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
            SDL_SetWindowFullscreen(sdlContext.window.get(), screenMode);
            if (wasFullscreen) {
                SDL_ShowCursor(SDL_DISABLE);
            } else {
                SDL_ShowCursor(SDL_ENABLE);
            }
        }
    }

    void drawGrid() {
        int numColumns  = sdlContext.gridImagesState.numColumns;
        int numRows     = sdlContext.gridImagesState.numRows;
        int imageRow    = sdlContext.currentImage / numColumns;
        int imageColumn = sdlContext.currentImage % numColumns;
        const auto computeVerticalScroll = [&]() {
            if (imageRow >
                sdlContext.gridImagesState.rowsScroll + numRows - 1) {
                sdlContext.gridImagesState.rowsScroll = imageRow - numRows + 1;
            } else if (imageRow < sdlContext.gridImagesState.rowsScroll) {
                sdlContext.gridImagesState.rowsScroll = imageRow;
            }
        };

        const auto drawImageBorder = [&]() {
            if (!sdlContext.imagesVector.empty()) {
                SDL_Rect firstImageRect{
                    sdlContext.style.padding +
                        imageColumn * (sdlContext.style.thumbnailSize +
                                       sdlContext.style.padding) -
                        sdlContext.style.selectionWidth,
                    sdlContext.style.padding +
                        (imageRow - sdlContext.gridImagesState.rowsScroll) *
                            (sdlContext.style.thumbnailSize +
                             sdlContext.style.padding) -
                        sdlContext.style.selectionWidth,
                    sdlContext.style.thumbnailSize +
                        2 * sdlContext.style.selectionWidth,
                    sdlContext.style.thumbnailSize +
                        2 * sdlContext.style.selectionWidth};

                auto borderColor = sdlContext.style.selectionColor;
                SDL_SetRenderDrawColor(sdlContext.renderer.get(),
                                       borderColor[0], borderColor[1],
                                       borderColor[2], 0xFF);
                SDL_RenderDrawRect(sdlContext.renderer.get(), &firstImageRect);
            }
        };

        const auto drawImagesGrid = [&]() {
            auto rowsScroll = sdlContext.gridImagesState.rowsScroll;
            for (int i = rowsScroll; i < numRows + rowsScroll; ++i) {
                for (int j = 0; j < numColumns; ++j) {
                    int index = i * numColumns + j;
                    if (index >= sdlContext.imagesVector.size()) {
                        // we've reached the end of the images vector
                        return;
                    }
                    if (!sdlContext.imagesVector[index].thumbnail)
                        continue;
                    const auto& image = sdlContext.imagesVector[index];

                    // Calculate the new width and height based on the aspect
                    // ratio of the original image
                    int originalWidth  = sdlContext.imagesVector[index].width;
                    int originalHeight = sdlContext.imagesVector[index].height;
                    int newWidth, newHeight;
                    if (originalWidth > originalHeight) {
                        // Landscape image
                        newWidth = sdlContext.style.thumbnailSize;
                        newHeight =
                            (sdlContext.style.thumbnailSize * originalHeight) /
                            originalWidth;
                    } else {
                        // Portrait or square image
                        newWidth =
                            (sdlContext.style.thumbnailSize * originalWidth) /
                            originalHeight;
                        newHeight = sdlContext.style.thumbnailSize;
                    }

                    // Create the destination rect for the image with the new
                    // width and height
                    SDL_Rect destRect{sdlContext.style.padding +
                                          j * (sdlContext.style.thumbnailSize +
                                               sdlContext.style.padding),
                                      sdlContext.style.padding +
                                          (i - rowsScroll) *
                                              (sdlContext.style.thumbnailSize +
                                               sdlContext.style.padding),
                                      newWidth, newHeight};
                    SDL_RenderCopy(sdlContext.renderer.get(),
                                   image.thumbnail.value().get(), nullptr,
                                   &destRect);
                }
            }
        };

        computeVerticalScroll();
        drawImageBorder();
        drawImagesGrid();
    }

    void preInputProcessing() {
        int windowWidth, windowHeight;
        SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);
        sdlContext.gridImagesState.numColumns =
            (windowWidth - 2 * sdlContext.style.padding) /
            (sdlContext.style.thumbnailSize + sdlContext.style.padding);
        sdlContext.gridImagesState.numRows =
            (windowHeight - 2 * sdlContext.style.padding) /
            (sdlContext.style.thumbnailSize + sdlContext.style.padding);
    }

    void getInputCommand() {
        if (event.type == SDL_QUIT) {
            sdlContext.exit = true;
        } else if (event.type == SDL_KEYDOWN) {
            // Check if a key was pressed
            SDL_Keycode key = event.key.keysym.sym;
            switch (key) {
            case SDLK_RETURN:
                // Add a newline character to the inputCommand string
                inputCommand += "\n";
                break;
            case SDLK_UP:
                inputCommand += "k";
                break;
            case SDLK_DOWN:
                inputCommand += "j";
                break;
            case SDLK_LEFT:
                inputCommand += "h";
                break;
            case SDLK_RIGHT:
                inputCommand += "l";
                break;
            default:
                break;
            }
            commandExecuter.matchCommand(sdlContext, inputCommand);
        } else if (event.type == SDL_TEXTINPUT) {
            // Check if a character was entered
            char character = event.text.text[0];
            inputCommand += character;
            commandExecuter.matchCommand(sdlContext, inputCommand);
        }
    }

    void drawImageViewer() {
        const auto& renderer = sdlContext.renderer;
        const auto& image    = sdlContext.imagesVector[sdlContext.currentImage];
        const auto& imageViewerState = sdlContext.imageViewerState;

        if (!image.image) {
            return;
        }

        // Get the window size
        int windowWidth, windowHeight;
        SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);

        // Calculate the aspect ratio of the image
        float aspectRatio;
        if (imageViewerState.rotation == 1 || imageViewerState.rotation == 3) {
            aspectRatio = (float)image.height / image.width;
        } else {
            aspectRatio = (float)image.width / image.height;
        }

        // Calculate the width and height of the image to be drawn
        int drawWidth, drawHeight;
        if (imageViewerState.fitHeight) {
            // Fit the image to the height of the window
            drawHeight = windowHeight;
            drawWidth  = (int)(drawHeight * aspectRatio);
        } else if (imageViewerState.fitWidth) {
            // Fit the image to the width of the window
            drawWidth  = windowWidth;
            drawHeight = (int)(drawWidth / aspectRatio);
        } else {
            // Use the zoom and panning values to calculate the width
            drawWidth  = (int)(image.width * imageViewerState.zoom);
            drawHeight = (int)(image.height * imageViewerState.zoom);
        }
        // Swap the width and height if the rotation is 1 or 3 and is fit
        if ((imageViewerState.rotation == 1 ||
             imageViewerState.rotation == 3) &&
            (imageViewerState.fitWidth || imageViewerState.fitHeight)) {
            std::swap(drawWidth, drawHeight);
        }

        // Calculate the x and y position of the image to be drawn
        //        int drawX = (windowWidth - drawWidth) / 2 +
        //        imageViewerState.panningX; int drawY = (windowHeight -
        //        drawHeight) / 2 + imageViewerState.panningY;
        const ImageViewerState& state = sdlContext.imageViewerState;
        int xPos, yPos;
        if (drawWidth > windowWidth) {
            // If the image is wider than the window, allow horizontal panning
            xPos = (windowWidth - drawWidth) / 2 + state.panningX;
        } else {
            // If the image is not wider than the window, center it horizontally
            xPos = (windowWidth - drawWidth) / 2;
        }
        if (drawHeight > windowHeight) {
            // If the image is taller than the window, allow vertical panning
            yPos = (windowHeight - drawHeight) / 2 + state.panningY;
        } else {
            // If the image is not taller than the window, center it vertically
            yPos = (windowHeight - drawHeight) / 2;
        }

        // Create a SDL_Rect for the image to be drawn
        SDL_Rect imageRect{xPos, yPos, drawWidth, drawHeight};
        if (imageViewerState.fitWidth || imageViewerState.fitHeight) {
            sdlContext.imageViewerState.zoom =
                (float)drawWidth /
                sdlContext.imagesVector[sdlContext.currentImage].width;
        }

        // Flip the image if needed
        SDL_RendererFlip flip = SDL_FLIP_NONE;
        if (imageViewerState.flipHorizontal) {
            flip = (SDL_RendererFlip)(flip | SDL_FLIP_HORIZONTAL);
        }
        if (imageViewerState.flipVertical) {
            flip = (SDL_RendererFlip)(flip | SDL_FLIP_VERTICAL);
        }

        // Rotate the image if needed
        int angle = imageViewerState.rotation * 90;

        // Draw the image to the renderer
        SDL_RenderCopyEx(renderer.get(), image.image.value().get(), nullptr,
                         &imageRect, angle, nullptr, flip);
    }

    void mainLoop() {
        const int TARGET_FPS         = 30;
        const int TARGET_FRAME_DELAY = 1000 / TARGET_FPS;
        while (!sdlContext.exit) {
            Uint32 frameStartTime = SDL_GetTicks();

            while (SDL_PollEvent(&event)) {
                getInputCommand();
            }
            preInputProcessing();
            maybeToggleFullscreen();
            imageLoaderPolicy.loadNext(sdlContext);
            SDL_RenderClear(sdlContext.renderer.get());
            if (sdlContext.isGridImages) {
                drawGrid();
            } else {
                drawImageViewer();
            }
            drawBottomBar();
            SDL_SetRenderDrawColor(sdlContext.renderer.get(), 30, 30, 30, 0x00);
            SDL_RenderPresent(sdlContext.renderer.get());

            // Calculate the elapsed time for the frame
            int elapsedTime = SDL_GetTicks() - frameStartTime;

            // Delay the program to maintain the target frame rate
            if (elapsedTime < TARGET_FRAME_DELAY) {
                SDL_Delay(TARGET_FRAME_DELAY - elapsedTime);
            }
        }
        CacheFilenames cacheFilenames;
        cacheFilenames.saveActualImagePosition(sdlContext);
    }

  private:
    std::string inputCommand;
    SdlContext sdlContext;
    SDL_Event event;
    CommandExecuter commandExecuter;
    ImageLoaderPolicy imageLoaderPolicy;
    bool wasFullscreen{false};
};

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

//**************************************************************
//********************* Implementation *************************
//**************************************************************

std::optional<SdlTexture> createThumbnailWithSize(
    const SdlRenderer& renderer, const std::string& filename, int maxWidth,
    int maxHeight, int& returnWidht, int& returnHeight, long& returnMemory) {
    auto texture = createTexture(renderer, filename);
    if (!texture) {
        return std::nullopt;
    }
    SDL_Point size;
    SDL_QueryTexture(texture.value().get(), NULL, NULL, &size.x, &size.y);
    returnMemory = std::filesystem::file_size(filename);
    returnWidht  = size.x;
    returnHeight = size.y;
    return createThumbnail(renderer, texture.value(), maxWidth, maxHeight);
}

SdlTexture createThumbnail(const SdlRenderer& renderer,
                           const SdlTexture& texture, int maxWidth,
                           int maxHeight) {
    SDL_Texture* originalTexture = texture.get();

    // Get the original dimensions of the texture
    int w, h;
    SDL_QueryTexture(originalTexture, NULL, NULL, &w, &h);

    // Calculate the new dimensions for the resized texture
    int newW, newH;
    if (w > h) {
        // Landscape image
        newW = maxWidth;
        newH = (h * maxWidth) / w;
    } else {
        // Portrait or square image
        newW = (w * maxHeight) / h;
        newH = maxHeight;
    }
    // Create the resized texture
    SDL_Texture* resizedTexture =
        SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_TARGET, newW, newH);

    // Copy the original texture to the resized texture, maintaining the aspect
    // ratio
    SDL_SetRenderTarget(renderer.get(), resizedTexture);
    SDL_RenderCopy(renderer.get(), originalTexture, NULL, NULL);
    SDL_SetRenderTarget(renderer.get(), NULL);

    return createTexture(resizedTexture);
}

SdlWindow createSdlWindow(const WindowSettings& windowSettings) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::string error{"Error initializing SDL: "};
        error += SDL_GetError();
        throw std::runtime_error{error};
    }

    unsigned int flags{0};

    flags |= windowSettings.borderless ? SDL_WINDOW_BORDERLESS : 0;
    flags |= windowSettings.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    flags |= SDL_WINDOW_RESIZABLE;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    //    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    auto sdlWin =
        SDL_CreateWindow(windowSettings.Title.c_str(), SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, windowSettings.width,
                         windowSettings.height, flags);

    if (sdlWin == nullptr) {
        std::string error = "Error creating SDL window: ";
        error += SDL_GetError();
        throw std::runtime_error{error};
    }

    return std::unique_ptr<SDL_Window, void (*)(SDL_Window*)>(
        sdlWin, &SDL_DestroyWindow);
}

SdlRenderer createSdlRenderer(const SdlWindow& sdlWindow) {
    auto renderer =
        SDL_CreateRenderer(sdlWindow.get(), -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::string error{"Error creating renderer: "};
        error += SDL_GetError();

        throw std::runtime_error{error};
    }

    return std::unique_ptr<SDL_Renderer, void (*)(SDL_Renderer*)>(
        renderer, &SDL_DestroyRenderer);
}

SdlTexture createTexture(SDL_Texture* texture) {
    return std::unique_ptr<SDL_Texture, void (*)(SDL_Texture*)>(
        texture, &SDL_DestroyTexture);
}

std::optional<SdlTexture> createTexture(const SdlRenderer& renderer,
                                        const std::string& filename) {
    auto tex = IMG_LoadTexture(renderer.get(), filename.c_str());
    if (tex) {
        return createTexture(tex);
    }
    return std::nullopt;
}

std::string memoryToHumanReadable(long bytes, int decimalPrecision) {
    constexpr static int kKilobyte = 1024;
    constexpr static int kMegabyte = 1024 * kKilobyte;
    constexpr static int kGigabyte = 1024 * kMegabyte;

    double value;
    std::string unit;
    if (bytes >= kGigabyte) {
        value = static_cast<double>(bytes) / kGigabyte;
        unit  = "GB";
    } else if (bytes >= kMegabyte) {
        value = static_cast<double>(bytes) / kMegabyte;
        unit  = "MB";
    } else if (bytes >= kKilobyte) {
        value = static_cast<double>(bytes) / kKilobyte;
        unit  = "KB";
    } else {
        value = bytes;
        unit  = "B";
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(decimalPrecision) << value << " "
       << unit;
    return ss.str();
}
