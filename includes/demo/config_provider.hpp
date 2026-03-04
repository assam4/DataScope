#ifndef CONFIG_PROVIDER_HPP
# define CONFIG_PROVIDER_HPP

# include <utility>
# include <vector>
# include <string>
# include <filesystem>
# include <spdlog/spdlog.h>
# include <format>
# include <toml++/toml.hpp>

/**
 * @class ConfigProvider
 * @brief Utility class for reading and parsing TOML configuration files
 * @details Provides methods to extract input file paths, output directories,
 * and processing modes from a TOML configuration. Supports both single values
 * and arrays for flexible configuration options.
 */

class   ConfigProvider {
    public:
        /**
         * @brief Constructs a ConfigProvider from a TOML file path
         * @param file Path to the TOML configuration file
         * @throws std::runtime_error if file parsing fails
         */
        ConfigProvider(const std::string& file): m_config(toml::parse_file(file)) {}
        ~ConfigProvider() = default;
        ConfigProvider(const ConfigProvider&) = delete;
        ConfigProvider& operator=(const ConfigProvider&) = delete;
        /**
         * @brief Retrieves list of input CSV files matching configuration criteria
         * @return Vector of file paths to process
         */
        std::vector<std::string>    get_input_files();
        /**
         * @brief Retrieves list of output directories for log files
         * @return Vector of output directory paths
         */
        std::vector<std::string> get_output_dirs();
        /**
         * @brief Retrieves the processing mode from configuration
         * @return String representing the selected processing mode
         */
        std::string get_work_mode();
    private:
        std::vector<std::string> get_dirs(const std::string&);
        std::vector<std::string>    get_files_mask();
    private:
        toml::table  m_config;
};

#endif