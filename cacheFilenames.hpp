#pragma once

#include "typesDefinition.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <sys/stat.h>

class CacheFilenames {
  public:
    CacheFilenames() {
        const char* cache_dirPtr = getenv("XDG_CACHE_HOME");
        std::string cache_dir;
        if (cache_dirPtr == nullptr) {
            cache_dirPtr = getenv("HOME");
            if (cache_dirPtr == nullptr) {
                return;
            }
            cache_dir = std::string(cache_dirPtr) + "/.cache";
        } else {
            cache_dir = std::string(cache_dirPtr);
        }

        // Create the "iv" directory if it doesn't already exist
        std::string aiv_dir = cache_dir + "/aiv";
        int mkdir_result    = mkdir(aiv_dir.c_str(), 0700);
        if (mkdir_result != 0 && errno != EEXIST) {
            std::cerr << "Error creating directory '" << aiv_dir
                      << "': " << strerror(errno) << std::endl;
            return;
        }

        filename = aiv_dir + "/imageNamesCache";
    }
    auto existsString(const std::vector<std::string>& vector) -> std::size_t;
    void saveActualImagePosition(const SdlContext& sdlContext);

  private:
    std::string filename{""};
};

// *************** Implementation ****************

void deleteLineFromFile(std::string input_filename, int line_number) {
    std::string output_filename = input_filename + "_aux";
    // Open the input file in input mode and the output file in output mode
    std::ifstream input_file(input_filename);
    std::ofstream output_file(output_filename);

    // Read the input file line by line
    std::string line;
    int current_line = 1;
    while (getline(input_file, line)) {
        // If the line number is not the line you want to delete, write it to
        // the output file
        if (current_line != line_number) {
            output_file << line << std::endl;
        }

        // Increment the current line number
        current_line++;
    }

    // Close both files
    input_file.close();
    output_file.close();

    // Delete the original input file
    remove(input_filename.c_str());

    // Rename the output file to the original file name
    rename(output_filename.c_str(), input_filename.c_str());
}

void CacheFilenames::saveActualImagePosition(const SdlContext& sdlContext) {
    if (sdlContext.currentImage == 0 || filename == "" ||
        sdlContext.currentImage == sdlContext.imagesVector.size() - 1) {
        return;
    }
    std::string imageFilename =
        sdlContext.imagesVector[sdlContext.currentImage].fileAdress;
    std::ofstream file(filename, std::ios::out | std::ios::app);
    file << imageFilename << std::endl;
    file.close();
}

auto
CacheFilenames::existsString(const std::vector<std::string>& vector) -> std::size_t {
    if (filename == "") {
        return 0;
    }

    const auto chomp = [](std::string& s) {
        if (s.empty()) {
            return;
        }

        if (s.back() == '\n') {
            s.pop_back();
        }

        if (s.back() == '\r') {
            s.pop_back();
        }
    };

    // Create an unordered set from the strings vector
    std::unordered_set<std::string_view> set(vector.begin(), vector.end());

    // Open the file for reading and writing
    std::fstream file(filename, std::ios::in | std::ios::out);
    if (!file.is_open()) {
        return 0;
    }

    // Iterate through the lines of the file
    std::size_t lineNumber = 0;
    std::string line;
    while (std::getline(file, line)) {
        ++lineNumber;

        chomp(line);
        // Check if the line is in the set
        if (set.find(line) != set.end()) {
            auto position = std::distance(
                vector.begin(), std::find(vector.begin(), vector.end(), line));

            file.close();

            // Delete line from the cache
            deleteLineFromFile(filename, (int)lineNumber);
            return position;
        }
    }

    // Close the file
    file.close();

    // If no match is found, return 0
    return 0;
}
