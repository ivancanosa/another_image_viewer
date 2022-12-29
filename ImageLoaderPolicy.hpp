#pragma once
#include "typesDefinition.hpp"
#include "sdlUtils.hpp"

class ImageLoaderPolicy {
  public:
    ImageLoaderPolicy(int numImages) : loadedThumbnails(numImages, false) {
    }

    void loadInGrid(SdlContext& sdlContext);
    void loadInViewer(SdlContext& sdlContext);
    void loadNext(SdlContext& sdlContext);

  private:
    std::vector<bool> loadedThumbnails;
    std::vector<std::size_t> loadedImages;
    int lastCurrentImage{-1};
    int lastLoadedThumbnail{-1};
    int lastLoadedImage{-1};
};

//**************************************************************
//********************* Implementation *************************
//**************************************************************

void ImageLoaderPolicy::loadInGrid(SdlContext& sdlContext) {
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

    const auto isThumnailNotLoaded = [](const auto& value) { return !value; };

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
            sdlContext.renderer, sdlContext.imagesVector[index].fileAdress, 100,
            100, width, height, memory);
        lastLoadedThumbnail += 1;
        loadedThumbnails[index] = true;
        if (!thumbnail) {
            return;
        }

        sdlContext.imagesVector[index].thumbnail = std::move(thumbnail);
        sdlContext.imagesVector[index].width     = width;
        sdlContext.imagesVector[index].height    = height;
        sdlContext.imagesVector[index].memory    = memory;
    };

    const auto& [rFirstIt, lastIt] = getFrontierIterators();
    auto it = findUnloadedImageIterator(rFirstIt, lastIt);
    loadThumbnailLambda(it);
}

void ImageLoaderPolicy::loadInViewer(SdlContext& sdlContext) {
    int index = sdlContext.currentImage;
    if (lastLoadedImage != index) {
        if (lastLoadedImage != -1) {
            sdlContext.imagesVector[lastLoadedImage].image     = std::nullopt;
            sdlContext.imagesVector[lastLoadedImage].animation = std::nullopt;
        }
        if (!loadGifAnimation(sdlContext.renderer,
                              sdlContext.imagesVector[index])) {
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
        }
        lastLoadedImage = index;
    }
}

void ImageLoaderPolicy::loadNext(SdlContext& sdlContext) {
    if (sdlContext.isGridImages) {
        loadInGrid(sdlContext);
    } else {
        loadInViewer(sdlContext);
    }
}
