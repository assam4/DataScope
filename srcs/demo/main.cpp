#include <boost/program_options.hpp>
#include "async_parser.hpp"
#include "config_provider.hpp"
#include "console_menu.hpp"

static std::string  parse_arguments(int, char **);
static void start_illustration_work(const std::string&);

int main(int argc, char** argv) {
    try {
        auto    configuration_file = parse_arguments(argc, argv);
        start_illustration_work(configuration_file);
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
    bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
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
    ConfigProvider cp(config_file);
    auto mode = cp.get_work_mode();
    if (mode == "RECEIVE_TS_PRICE_PAIR") {
        AsyncParser<datascope::AccFlags::RECEIVE_TS_PRICE_PAIR> parser;
        auto    result = parser.collect(cp.get_input_files());
        if (!result)
            throw std::runtime_error("Something went wrong...");
        ConsoleMenu  menu(std::move(result.value()), cp.get_output_dirs());
        menu.switchConsoleMenu();
    }
}