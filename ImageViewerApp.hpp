#pragma once

#include "ImageLoaderPolicy.hpp"
#include "cacheFilenames.hpp"
#include "command.hpp"
#include "typesDefinition.hpp"

class ImageViewerApp {
  public:
    ImageViewerApp(SdlContext&& sdlContextArg)
        : sdlContext(std::move(sdlContextArg)),
          imageLoaderPolicy(sdlContext.imagesVector.size()) {
    }

    void drawBottomBarBackground();
    void drawBottomLeftText(const std::string& text);
    void drawBottomRightText(const std::string& text);
    void drawBottomBar();

    void maybeToggleFullscreen();
    void preInputProcessing();
    void getInputCommand();

    void setImagesToLoad();

    void drawGrid();
    void drawImageViewer();
    void drawImageViewerContiguous();

    void mainLoop();

  private:
    std::string inputCommand;
    SdlContext sdlContext;
    SDL_Event event;
    CommandExecuter commandExecuter;
    ImageLoaderPolicy imageLoaderPolicy;
    bool wasFullscreen{false};
};

//**************************************************************
//********************* Class Functions *************************
//**************************************************************

void ImageViewerApp::drawBottomBarBackground() {
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

void ImageViewerApp::drawBottomLeftText(const std::string& text) {
    // Get the window dimensions
    int windowWidth, windowHeight;
    SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);

    // Set the color for drawing the text
    SDL_SetRenderDrawColor(sdlContext.renderer.get(), 255, 255, 255, 255);

    // Render the text
    SDL_Surface* textSurface = TTF_RenderText_Blended(
        sdlContext.font.value().get(), text.c_str(), {255, 255, 255});
    SDL_Texture* textTexture =
        SDL_CreateTextureFromSurface(sdlContext.renderer.get(), textSurface);
    int barHeight     = 50;
    int barY          = windowHeight - barHeight;
    SDL_Rect textRect = {10, barY + 10, textSurface->w, textSurface->h};
    SDL_RenderCopy(sdlContext.renderer.get(), textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void ImageViewerApp::drawBottomRightText(const std::string& text) {
    // Get the window dimensions
    int windowWidth, windowHeight;
    SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);

    // Set the color for drawing the text
    SDL_SetRenderDrawColor(sdlContext.renderer.get(), 255, 255, 255, 255);

    // Render the text
    SDL_Surface* textSurface = TTF_RenderText_Blended(
        sdlContext.font.value().get(), text.c_str(), {255, 255, 255});
    SDL_Texture* textTexture =
        SDL_CreateTextureFromSurface(sdlContext.renderer.get(), textSurface);
    int barHeight     = 50;
    int barY          = windowHeight - barHeight;
    SDL_Rect textRect = {windowWidth - 10 - textSurface->w, barY + 10,
                         textSurface->w, textSurface->h};
    SDL_RenderCopy(sdlContext.renderer.get(), textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void ImageViewerApp::drawBottomBar() {
    if (sdlContext.showBar && sdlContext.font) {
        drawBottomBarBackground();
        const auto& imageHeader =
            sdlContext.imagesVector[sdlContext.currentImage];
        std::string leftInfo =
            "size : " + memoryToHumanReadable(imageHeader.memory);
        leftInfo += ", " + std::to_string(imageHeader.width) + "x" +
                    std::to_string(imageHeader.height);
        leftInfo += ", address: " + imageHeader.fileAdress;
        std::string rightInfo;

        if (imageHeader.animation) {
            rightInfo +=
                std::to_string(imageHeader.animation.value().actualFrame) +
                "/" +
                std::to_string(imageHeader.animation.value().frames.size() -
                               1) +
                ", ";
        }
        rightInfo += std::to_string(sdlContext.currentImage) + "/" +
                     std::to_string(sdlContext.imagesVector.size() - 1);
        drawBottomLeftText(leftInfo);
        drawBottomRightText(rightInfo);
    }
}

void ImageViewerApp::drawGrid() {
    int numColumns                   = sdlContext.gridImagesState.numColumns;
    int numRows                      = sdlContext.gridImagesState.numRows;
    int imageRow                     = sdlContext.currentImage / numColumns;
    int imageColumn                  = sdlContext.currentImage % numColumns;
    const auto computeVerticalScroll = [&]() {
        if (imageRow > sdlContext.gridImagesState.rowsScroll + numRows - 1) {
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
            SDL_SetRenderDrawColor(sdlContext.renderer.get(), borderColor[0],
                                   borderColor[1], borderColor[2], 0xFF);
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

                // Calculate the new width and height based on the
                // aspect ratio of the original image
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

                // Create the destination rect for the image with the
                // new width and height
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

void ImageViewerApp::maybeToggleFullscreen() {
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

void ImageViewerApp::preInputProcessing() {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);
    sdlContext.gridImagesState.numColumns =
        (windowWidth - 2 * sdlContext.style.padding) /
        (sdlContext.style.thumbnailSize + sdlContext.style.padding);
    sdlContext.gridImagesState.numRows =
        (windowHeight - 2 * sdlContext.style.padding) /
        (sdlContext.style.thumbnailSize + sdlContext.style.padding);
}

void ImageViewerApp::getInputCommand() {
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

void ImageViewerApp::drawImageViewer() {
    const auto& renderer = sdlContext.renderer;
    auto& image          = sdlContext.imagesVector[sdlContext.currentImage];
    const auto& imageViewerState = sdlContext.imageViewerState;

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
    if ((imageViewerState.rotation == 1 || imageViewerState.rotation == 3) &&
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
        // If the image is wider than the window, allow horizontal
        // panning
        xPos = (windowWidth - drawWidth) / 2 + state.panningX;
    } else {
        // If the image is not wider than the window, center it
        // horizontally
        xPos = (windowWidth - drawWidth) / 2;
    }
    if (drawHeight > windowHeight) {
        // If the image is taller than the window, allow vertical
        // panning
        yPos = (windowHeight - drawHeight) / 2 + state.panningY;
    } else {
        // If the image is not taller than the window, center it
        // vertically
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
    if (image.image) {
        SDL_RenderCopyEx(renderer.get(), image.image.value().get(), nullptr,
                         &imageRect, angle, nullptr, flip);
    }
    if (image.animation) {
        SDL_RenderCopyEx(renderer.get(),
                         image.animation.value()
                             .frames[image.animation.value().actualFrame]
                             .get(),
                         nullptr, &imageRect, angle, nullptr, flip);
        image.animation.value().actualFrame =
            (image.animation.value().actualFrame + 1) %
            image.animation.value().frames.size();
        sdlContext.fps = image.animation.value().fps;
    }
}

void ImageViewerApp::drawImageViewerContiguous() {
    const auto& renderer = sdlContext.renderer;
    auto& images = sdlContext.imagesVector; // vector of images to be drawn
    const auto& imageViewerState = sdlContext.imageViewerState;

    // Get the window size
    int windowWidth, windowHeight;
    SDL_GetWindowSize(sdlContext.window.get(), &windowWidth, &windowHeight);

    // Calculate the total height of all images
    int totalHeight = 0;
    for (const auto& image : images) {
        totalHeight += image.height;
    }

    // Calculate the width and height of the images to be drawn
    int drawWidth = windowWidth, drawHeight = 0;
    float zoom = imageViewerState.zoom;
    if (totalHeight * zoom > windowHeight) {
        // If the combined height of all images is greater than the window
        // height, allow vertical panning
        drawHeight = windowHeight;
        zoom       = (float)drawHeight /
               totalHeight; // recalculate zoom to fit the window height
    } else {
        // If the combined height of all images is not greater than the window
        // height, center them vertically
        drawHeight = (int)(totalHeight * zoom);
    }

    // Calculate the x and y position of the images to be drawn
    int xPos = (windowWidth - drawWidth) / 2;
    int yPos = (windowHeight - drawHeight) / 2 +
               imageViewerState.panningY; // allow vertical panning

    // Create a SDL_Rect for the images to be drawn
    SDL_Rect imagesRect{xPos, yPos, drawWidth, drawHeight};

    // Loop through the images and render them to the window
    int currentY = 0; // keep track of the current y position
    std::size_t index{0};
    for (const auto& image : images) {
        // Calculate the height and width of the current image to be drawn based
        // on its aspect ratio
        float aspectRatio = (float)image.width / image.height;
        int currentHeight = (int)(drawWidth / aspectRatio);
        int currentWidth  = drawWidth;
        if (currentHeight > drawHeight) {
            // If the calculated height is greater than the drawHeight, adjust
            // the width instead
            currentWidth  = (int)(drawHeight * aspectRatio);
            currentHeight = drawHeight;
        }

        // Check if the current image is partially or fully within the window
        if (currentY + currentHeight >= -imageViewerState.panningY &&
            currentY <= -imageViewerState.panningY + windowHeight) {
            sdlContext.imagesToLoad.insert(index);
            // The current image is partially or fully within the window, so
            // render it
            if (image.image) {
				xPos = (windowWidth - currentWidth) / 2;
                SDL_Rect imageRect{xPos, yPos + currentY, currentWidth,
                                   currentHeight};
                SDL_RenderCopy(renderer.get(), image.image.value().get(),
                               nullptr, &imageRect);
            }
        } else {
            sdlContext.imagesToLoad.erase(index);
        }

        // Update the current y position
        currentY += currentHeight;
        index += 1;
    }
}

void ImageViewerApp::setImagesToLoad() {
    if (sdlContext.isGridImages) {
        sdlContext.imagesToLoad.clear();
        return;
    }
    if (sdlContext.contiguousView) {
        // Do nothing here
    } else {
        sdlContext.imagesToLoad.clear();
        sdlContext.imagesToLoad.insert(sdlContext.currentImage);
    }
}

void ImageViewerApp::mainLoop() {
    while (!sdlContext.exit) {
        Uint32 frameStartTime = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            getInputCommand();
        }
        preInputProcessing();
        maybeToggleFullscreen();
        setImagesToLoad();
        imageLoaderPolicy.loadNext(sdlContext);
        SDL_RenderClear(sdlContext.renderer.get());
        if (sdlContext.isGridImages) {
            drawGrid();
        } else if (!sdlContext.contiguousView) {
            drawImageViewer();
        } else if (sdlContext.contiguousView) {
            drawImageViewerContiguous();
        }
        drawBottomBar();
        SDL_SetRenderDrawColor(sdlContext.renderer.get(), 30, 30, 30, 0x00);
        SDL_RenderPresent(sdlContext.renderer.get());

        // Calculate the elapsed time for the frame
        int elapsedTime = SDL_GetTicks() - frameStartTime;

        int frameDelay = 1000 / sdlContext.fps;
        // Delay the program to maintain the target frame rate
        if (elapsedTime < frameDelay) {
            SDL_Delay(frameDelay - elapsedTime);
        }
        sdlContext.fps = 24;
    }
    CacheFilenames cacheFilenames;
    cacheFilenames.saveActualImagePosition(sdlContext);
}
