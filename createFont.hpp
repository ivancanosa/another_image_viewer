#pragma once

#include "SDL_ttf.h"
#include "typesDefinition.hpp"
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#else
#include <fontconfig/fontconfig.h>
#endif

// Function to create a font
auto createFont(const std::vector<std::string>& fontNames,
                                  float fontSize = 8) -> std::optional<SdlFont> {
    // Try to find and open each font in the list
    for (const std::string& fontName : fontNames) {
// Use preprocessor directives to compile the appropriate code for getting the
// font on Windows or Linux
#ifdef _WIN32
        // Get the path to the Windows directory
        char windowsDir[MAX_PATH];
        GetWindowsDirectoryA(windowsDir, MAX_PATH);

        // Append the path to the font file to the Windows directory
        std::string fontPath = windowsDir;
        fontPath += "\\Fonts\\" + fontName + ".ttf";

        // Open the font with SDL_ttf
        TTF_Font* font = TTF_OpenFont(fontPath.c_str(), fontSize);
#else
        // Create a pattern for the font
        FcPattern* pattern = FcNameParse((const FcChar8*)fontName.c_str());

        // Set the pattern attributes for the font style and size
        FcPatternAddString(pattern, FC_STYLE, (const FcChar8*)"Regular");
        FcPatternAddDouble(pattern, FC_SIZE, fontSize);

        // Find the font file on the system
        FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);
        FcResult result;
        FcPattern* fontPattern = FcFontMatch(nullptr, pattern, &result);

        // Extract the font file path from the pattern
        FcChar8* fontFile;
        FcPatternGetString(fontPattern, FC_FILE, 0, &fontFile);

        // Open the font with SDL_ttf
        TTF_Font* font = TTF_OpenFont((const char*)fontFile, (int)fontSize);

        // Destroy the fontconfig pattern
        FcPatternDestroy(pattern);
#endif

        // Check if the font was successfully opened
        if (font != nullptr) {
            // Return the font
            return std::unique_ptr<TTF_Font, void (*)(TTF_Font*)>(
                font, &TTF_CloseFont);
        }
    }
    // If no fonts were found, return std::nullopt
    return std::nullopt;
}
