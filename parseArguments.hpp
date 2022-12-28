#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#ifdef _WIN32
// Maybe some window libraries
#else
#include <fcntl.h>
#include <fstream>
#include <sys/fcntl.h>
#endif

std::tuple<std::vector<std::string>, std::vector<std::string>>
parseLineArguments(int argc, char** argv) {
    std::vector<std::string> args;
    std::vector<std::string> flags;
    // Iterate over the command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            flags.push_back(argv[i]);
        } else {
            args.push_back(argv[i]);
        }
    }

#ifdef _WIN32
    // TODO: Windows code to get input from pipeline in a non blocknig way
#else
    int flags2 = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags2 | O_NONBLOCK);

    std::string line;
    while (std::getline(std::cin, line)) {
        args.push_back(line);
    }
#endif
    return {args, flags};
}
