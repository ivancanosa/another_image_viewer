#pragma once

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <regex>
#include <unordered_set>
#include <vector>
#include <tuple>

// This code has a vector of strings as input. The input strings could
// represent:
//   - A filename: If the file exists, it will be copied to the output vector.
//   - A directory: All files inside it will be copied to the output vector.
//   - A regular expression: All files that match will be copied to the output
//   vector.
// Lastly, the code eliminates all duplicated files from the output vector
auto
expandInputFiles(const std::vector<std::string>& files) -> std::vector<std::string> {
    const std::unordered_set<std::string> image_extensions = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff"};

    const auto getFileType =
        [&](const std::string& filename) -> std::tuple<bool, bool> {
        bool isImage     = false;
        bool isDirectory = false;
        if (std::filesystem::exists(filename)) {
            std::filesystem::directory_entry entry(filename);
            if (entry.is_regular_file()) {
                const std::string extension = entry.path().extension().string();
                if (image_extensions.find(extension) !=
                    image_extensions.end()) {
                    isImage = true;
                }
            }
            if (entry.is_directory()) {
                isDirectory = true;
            }
        }
        return {isImage, isDirectory};
    };

    const auto removeDuplicates = [](std::vector<std::string>& v) {
        std::unordered_set<std::string_view> seen;
        v.erase(std::remove_if(v.begin(), v.end(),
                               [&](const auto& x) {
                                   if (seen.find(x) == seen.end()) {
                                       seen.insert(x);
                                       return false;
                                   }
                                   return true;
                               }),
                v.end());
    };

    std::vector<std::string> existing;

    const auto expandToOutputVector = [&](const auto& file) {
        const auto& [isImage, isDirectory] = getFileType(file);
        if (isImage) {
            existing.push_back(file);
        } else if (isDirectory) {
            for (const auto& entry :
                 std::filesystem::directory_iterator(file)) {
				const auto entryStr = entry.path().string();
                const auto& [isImage, _] = getFileType(entryStr);
                if (isImage) {
                    existing.push_back(entryStr);
                }
            }
        } /*else
        if(std::filesystem::exists(std::filesystem::path(file).parent_path())) {
            std::smatch match;
            std::regex pattern(file);
            for (const auto& entry : std::filesystem::directory_iterator(
                     std::filesystem::path(file).parent_path())) {
                const std::string filename = entry.path().filename().string();
                if (entry.is_regular_file() &&
                    std::regex_match(filename, match, pattern)) {
                    existing.push_back(entry.path().string());
                }
            }
        }*/
    };

    std::for_each(files.begin(), files.end(), expandToOutputVector);
    removeDuplicates(existing);
    return existing;
}
