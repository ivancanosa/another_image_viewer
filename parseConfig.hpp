#pragma once

#include "typesDefinition.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "monads.hpp"

class ParseConfig {
  public:
    ParseConfig() {
        using EitherT = Either<std::string, int>;

        const auto loadXdgPath = [](const auto& _) -> EitherT {
            const char* config_dirPtr = getenv("XDG_CONFIG_HOME");
            if (config_dirPtr == nullptr) {
                return 0;
            }
            return std::string(config_dirPtr);
        };

        const auto loadDefaultPath = [](const auto& _) -> EitherT {
            const char* config_dirPtr = getenv("HOME");
            if (config_dirPtr == nullptr) {
                return 0;
            }
            return std::string(config_dirPtr) + "/.config";
        };

        const auto createAivDir = [](const auto& config_dir) -> std::string {
            std::string aiv_dir = config_dir + "/aiv";
            int mkdir_result    = mkdir(aiv_dir.c_str(), 0700);
            if (mkdir_result != 0 && errno != EEXIST) {
                std::cerr << "Error creating directory '" << aiv_dir
                          << "': " << strerror(errno) << std::endl;
                return "";
            }
            return aiv_dir;
        };

        const auto configDir = EitherT(0) | loadXdgPath | loadDefaultPath;
        if (std::holds_alternative<std::string>(configDir)) {
            const auto aivConfig =
                createAivDir(std::get<std::string>(configDir));
            filename = aivConfig + "/key_commands.json";
        }
    }
    auto parseConfigFile() -> ConfigStruct;

  private:
    std::string filename{"key_commands.json"};
};

// *************** Implementation ****************
auto ParseConfig::parseConfigFile() -> ConfigStruct {
    ConfigStruct configStruct;

	//Check  if config fil exists
    if (!std::filesystem::exists(filename)) {
        std::ofstream output_file(filename);
		nlohmann::json config_json = configStruct;
        output_file << config_json.dump(4);
        return configStruct;
    }

    std::ifstream input_file(filename);
    if (!input_file.is_open()) {
        std::cerr << "Error: unable to open config file" << std::endl;
        configStruct;
    }

    // Parse the JSON file into a JSON object
    nlohmann::json config_json;
    input_file >> config_json;

    // Use the json::get method to parse the JSON object into a ConfigStruct
    // object
    configStruct = config_json.get<ConfigStruct>();
    return configStruct;
}
