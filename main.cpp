#include "prime_search.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

Config read_config(const std::string& path = "config.txt") {
    Config cfg;
    cfg.threads = 4;
    cfg.max_n = 1000;
    cfg.variant = 1;

    std::ifstream in(path);
    if (!in) {
        std::cerr << "Warning: could not open " << path << ", using defaults.\n";
        return cfg;
    }
    std::string line;
    while (std::getline(in, line)) {
        // strip comments and whitespace
        auto pos = line.find('#');
        if (pos != std::string::npos) line = line.substr(0, pos);
        std::istringstream iss(line);
        std::string kv;
        if (!(iss >> kv)) continue;
        auto eq = kv.find('=');
        if (eq == std::string::npos) continue;
        std::string key = kv.substr(0, eq);
        std::string val = kv.substr(eq + 1);
        if (key == "threads") cfg.threads = std::stoi(val);
        else if (key == "max") cfg.max_n = std::stoi(val);
        else if (key == "variant") cfg.variant = std::stoi(val);
    }
    return cfg;
}

int main(int argc, char** argv) {
    std::string cfgpath = "config.txt";
    if (argc > 1) cfgpath = argv[1];

    Config cfg = read_config(cfgpath);
    PrimeSearcher searcher(cfg);
    searcher.run();

    return 0;
}
