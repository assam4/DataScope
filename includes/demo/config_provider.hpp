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
*   @class  ConfigProvider
*   @brief  utility class for reading .toml configuration
*   @details returns std::pair<input, output>, where input is std::vector of input(.log) files
*          , until output is a directory path.
*/

class   ConfigProvider {
    public:
        ConfigProvider(const std::string& file): m_config(toml::parse_file(file)) {
            if (!m_config)
                throw std::runtime_error(std::format("Failed to parse config: {}", m_config.error()));
        }
        ~ConfigProvider() = default;
        ConfigProvider(const ConfigProvider&) = delete;
        ConfigProvider& operator=(const ConfigProvider&) = delete;
        std::vector<std::string>    get_input_files();
        std::string get_output_path();
        std::string get_work_mode();
    private:
        std::vector<std::string> get_dirs(const std::string&);
        std::vector<std::string>    get_files_mask();
    private:
        toml::parse_result  m_config;
};

#endif