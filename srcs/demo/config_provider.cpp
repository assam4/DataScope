#include "config_provider.hpp"

std::vector<std::string> ConfigProvider::get_dirs(const std::string& name) {
    auto main_table = (*m_config)["main"].as_table();
    if (!main_table || !main_table->contains(name))
        throw std::runtime_error(std::format("Invalid configuration file: missing {} parameter.", name));    
    auto&   value = (*main_table)[name];
    if (auto arr = value.as_array()) {
        std::vector<std::string>    result;
        for (const auto& item : *arr) {
            if (auto str = item.value<std::string>())
                result.push_back(*str);
        }
        return result;
    }    
    if (auto str = value.value<std::string>())
        return {*str};
    throw std::runtime_error(std::format("Invalid type for {} parameter.", name));
}

std::vector<std::string>    ConfigProvider::get_files_mask() {
    auto    main_table = (*m_config)["main"].as_table();
    if (!main_table || !main_table->contains("filename_mask"))
        return {}; 
    auto    masks_array = (*main_table)["filename_mask"].as_array();
    if (!masks_array)
        return {};
    std::vector<std::string> masks;
    for (const auto& item : *masks_array) {
        if (auto str = item.value<std::string>()) {
            masks.emplace_back(*str);
        }
    }
    return masks;
}

std::vector<std::string> ConfigProvider::get_input_files() {
    auto    dirs = get_dirs("input");
    auto    masks = get_files_mask();
    std::vector<std::string>    input_files;
    for (const auto& dir : dirs) {
        if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
            spdlog::warn(std::format("Skipping invalid input directory: {}", dir));
            continue;
        }
        for (const auto& file : std::filesystem::directory_iterator(dir)) {
            if (!file.is_regular_file() || file.path().extension() != ".csv")
                continue;
            const std::string filename = file.path().filename().string();
            if (masks.empty()) {
                input_files.emplace_back(file.path().string());
                continue;
            }        
            bool matches = false;
            for (const auto& mask : masks)
                if (filename.find(mask) != std::string::npos) {
                    matches = true;
                    break;
                }
            if (matches)
                input_files.emplace_back(file.path().string());
        }
    }
    if (input_files.empty())
        spdlog::warn("No CSV files matching the criteria found in any input directories");
    return input_files;
}

std::string ConfigProvider::get_output_path() {
    auto    dirs = get_dirs("output");
    auto    dir = dirs.empty() ? "" : dirs[0];
    if (dir.empty()) {
        dir = "./output";
        spdlog::info(std::format("Output directory not specified. Using default: {}", dir));
    }    
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
        spdlog::info(std::format("Created output directory: {}", dir));
    }
    if (!std::filesystem::is_directory(dir))
        throw std::runtime_error(std::format("Output path '{}' is not a directory", dir));
    return dir;
}

std::string ConfigProvider::get_work_mode() {
    auto dirs = get_dirs("mode");
    return dirs.empty() ? "" : dirs[0];
}