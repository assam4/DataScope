#include <boost/program_options.hpp>
#include "async_parser.hpp"

static std::string  parse_arguments(int, char **);
static void start_illustration_work(const std::string&);

int main(int argc, char** argv) {
    try {
        auto    configuration_file = parse_arguments(argc, argv);
        start_illustration_work(config);
        return 0;
    }
    catch (const std::exception& e) {
        spdlog::error(std::format("Error: {}", e.what()));
        return 1;
    }
    catch (...) {
        spdlog::error("Error: Unknown bag...");
        return 2;
    }
}

std::string parse_arguments(int argc, char **argv) {
    namespace bpo = boost::program_options;
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("config", bpo::value<std::string>(), "Configuration file path")
        ("cfg", bpo::value<std::string>(), "Configuration file path");
    bpo::variables_map   vm;
    bpo::store(po::parse_command_line(argc, argv, desc), vm);
    std::string configFile =  "default_config.toml";
    if (vm.count("config"))
        configFile = vm["config"].as<std::string>();
    else if (vm.count("cfg"))
        configFile = vm["cfg"].as<std::string>();
    return configFile;
}

void    start_illustration_work(const std::string& config_file) {
    spdlog::info(std::format("launch of version 1.3.0 of the program"));
    spdlog::info(std::format("processing of the {} configuration file", config_file));
     
    auto    io = TomlConfigLoader::getPaths(config_file);
    auto    result
    auto    result = clp.collect(io.first);
    if (!result)
            throw std::runtime_error("Something went wrong...");
    Logger  logger(result.value(), io.second);
    logger.switchLogger();
}
    
 