#include <algorithm>
#include <fstream>
#include <exception>
#include <format>
#include <spdlog/spdlog.h>
#include "text_chunker.hpp"

namespace datascope {

    static constexpr std::string_view   level_header = "receive_ts;exchange_ts;price;quantity;side;rebuild";
    static constexpr std::string_view   trade_header = "receive_ts;exchange_ts;price;quantity;side";
    static constexpr unsigned   CHUNK_SIZE = 4096;

    static std::expected<std::ifstream, bool> open_file(const std::string& file) {
        std::ifstream is(file);
        if (!is.is_open()) {
            spdlog::warn(std::format("Failed to open: {}", file));
            return std::unexpected<bool>(false);
        }
        return std::move(is);
    }

    static bool compare_and_erase_header(std::ifstream& is) {
        std::string header;
        std::getline(is, header);
        return header == level_header || header == trade_header;
    }

    static void get_chunks(const std::string& file, std::vector<std::string>& result) {
        auto is_opt = datascope::open_file(file);
        if (!is_opt)
            return;
        auto is = std::move(is_opt.value());
        if (!compare_and_erase_header(is)) {
            spdlog::warn(std::format("Invalid CSV header in file '{}'", file));
            return;
        }
        char    buffer[CHUNK_SIZE];
        while (is.read(buffer, CHUNK_SIZE)) {
            std::string current(buffer, is.gcount());
            if (current.length() == CHUNK_SIZE && current.back() != '\n') {
                std::string helper;
                std::getline(is, helper);
                current += helper;
            }
            result.push_back(current);
        }
    }

    std::vector<std::string>&&    TextChunker::extract_chunks(const std::vector<std::string>& files) {
        std::vector<std::string> result;
        std::for_each(files.begin(), files.end(), [&](const auto& file) { get_chunks(file, result); });
        return std::move(result);
    }

}