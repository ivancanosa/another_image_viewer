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

#include "ImageLoaderPolicy.hpp"
#include "ImageViewerApp.hpp"
#include "command.hpp"
#include "filesUtils.hpp"
#include "parseArguments.hpp"
#include "typesDefinition.hpp"
#include "sdlUtils.hpp"
#include "createFont.hpp"


auto main(int argc, char* argv[]) -> int {
	SdlContext context = createSdlContext(argc, argv);
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




