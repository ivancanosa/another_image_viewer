#pragma once

#include <filesystem>
#include <sstream>

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

bool loadGifAnimation(SdlRenderer& renderer, ImageHeader& imageHeader);

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

    // Copy the original texture to the resized texture, maintaining the
    // aspect ratio
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

	if(windowSettings.useBilinearInterpolation){
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	}
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

bool loadGifAnimation(SdlRenderer& renderer, ImageHeader& imageHeader) {
    // Check if the file is a GIF
    SDL_RWops* io = SDL_RWFromFile(imageHeader.fileAdress.c_str(), "rb");
    if (io == nullptr) {
        return false;
    }
    if (IMG_isGIF(io) == 0) {
        // File is not a GIF, return
        return false;
    }

    // Load the GIF animation
    IMG_Animation* gifAnimation =
        IMG_LoadAnimation(imageHeader.fileAdress.c_str());
    if (gifAnimation == NULL) {
        return false;
    }

    // Create an SdlAnimation object to store the frames and FPS of the GIF
    // animation
    SdlAnimation animation;
    //   animation.fps = gifAnimation->fps;

    animation.fps = std::round(
        std::clamp((1. / (*gifAnimation->delays * 0.001)), 1., 100.));
    // Convert each frame of the GIF animation to an SdlTexture and add it to
    // the SdlAnimation object
    for (int i = 0; i < gifAnimation->count; i++) {
        SDL_Surface* surface = gifAnimation->frames[i];
        SdlTexture texture(
            SDL_CreateTextureFromSurface(renderer.get(), surface),
            SDL_DestroyTexture);
        animation.frames.push_back(std::move(texture));
    }

    // Update the width, height, and animation fields of the ImageHeader object
    imageHeader.width     = gifAnimation->w;
    imageHeader.height    = gifAnimation->h;
    imageHeader.animation = std::move(animation);
    imageHeader.memory =
        std::filesystem::file_size(imageHeader.fileAdress.c_str());

    // Free the IMG_Animation object
    IMG_FreeAnimation(gifAnimation);
    return true;
}
