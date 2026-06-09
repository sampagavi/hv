#include <iostream>
#include <string>
#include "tui/app.hpp"

int main(int argc, char* argv[]) {
    AppConfig cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--replay" && i + 1 < argc) {
            cfg.replay_mode = true;
            cfg.replay_file = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: llm-tracer [--replay <file>]\n";
            return 0;
        }
    }

    run_app(cfg);
    return 0;
}
