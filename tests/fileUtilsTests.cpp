#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "filesUtils.hpp"

void printVec(const std::vector<std::string>& vec) {
    for (const auto& s : vec) {
        std::cout << s << std::endl;
    }
}

// Test function for expandInputFiles
void testExpandInputFiles() {
    const std::string parent{"../tests/testDir/"};

    const auto addParentLambda = [&](const auto& str) { return parent + str; };

    const auto addParent = [&](auto& vec) {
        std::transform(vec.begin(), vec.end(), vec.begin(), addParentLambda);
    };
    // Test 1: Input is a single file
    {
        std::vector<std::string> input{"test.png"};
        std::vector<std::string> expected{"test.png"};
        addParent(input);
        addParent(expected);
        std::vector<std::string> output = expandInputFiles(input);
        assert(output == expected);
    }

    // Test 2: Input is a directory
    {
        std::vector<std::string> input{"test_dir"};
        std::vector<std::string> expected{"test_dir/file1.png",
                                          "test_dir/file2.png"};
        std::vector<std::string> expected2{"test_dir/file2.png",
                                           "test_dir/file1.png"};
        addParent(input);
        addParent(expected);
        addParent(expected2);
        std::vector<std::string> output = expandInputFiles(input);
        assert(output == expected || output == expected2);
    }

    // Test 3: Input is a regular expression
    /*{
        std::vector<std::string> input{"test_dir/."};
        std::vector<std::string> expected{"test_dir/file1.png",
                                          "test_dir/file2.png"};
                addParent(input);
                addParent(expected);
        std::vector<std::string> output = expandInputFiles(input);
                printVec(output);
        assert(output == expected);
    }*/

    // Test 4: Input has duplicates
    {
        std::vector<std::string> input{"test.png", "test.png"};
        std::vector<std::string> expected{"test.png"};
        addParent(input);
        addParent(expected);
        std::vector<std::string> output = expandInputFiles(input);
        assert(output == expected);
    }
}

int main() {
    testExpandInputFiles();
    return 0;
}
