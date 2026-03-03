#include <map>
#include <format>
#include <spdlog/spdlog.h>
#include "text_chunker.hpp"
#include "selective_storage.hpp


namespace   datascope {

    static constexpr unsigned   CHUNK_SIZE = 4096;

    /**
     * @brief Static map providing regex patterns for validating file headers based on FileType.
     */
    static const std::map<FileType, std::regex> __regexHeaderProvider {
        {AccFlags::RECEIVE_TS_PRICE_PAIR, std::regex("^receive_ts;[^;]*;price(;.*)?$")},
        {AccFlags::PRICE_QUANTITY_PAIR, std::regex("[^;]*;[^;]*;price;quantity(;.*)?$")},
        {AccFlags::PRICE_SIDE_PAIR, std::regex("[^;]+;[^;]+;price;[^;]*;side(;.*)?$")},
        {AccFlags::LEVEL, std::regex("^receive_ts;exchange_ts;price;quantity;side(;.*)?$")},
        {AccFlags::TRADE, std::regex("^receive_ts;exchange_ts;price;quantity;side;rebuild(;.*)?$")}
    };

    TextChunker::TextChunker(const std::vector<std::string>& files, AccFlags type)
        : m_files(files)
        , m_regex(__regexHeaderProvider.contains(type) ? __regexHeaderProvider.at(type) : std::regex(""))
        , m_index(-1)
        , m_finished(false) {
    }

    void    TextChunker::open_subsequent_file() {
        do {
            if (m_current.is_open())
                m_current.close();
            if (m_index == m_files.size() - 1)
                m_finished = true;
            else
                m_current.open(m_files[++m_index]);
        }
        while (!check_stream_state());
    }

    bool    TextChunker::check_stream_state() {
        if (check_index_end()) {
            m_finished = true;
            return m_finished;
        }
        if (!m_current.is_open()) {
            spdlog::warn(std::format("Failed to open: {}", m_files[m_index]));
            return false;
        }
        std::string header;
        std::getline(m_current, header);
        if (!check_header(header)) {
            spdlog::warn(std::format("Invalid CSV header '{}' in file '{}'", header, m_files[m_index]));
            return false;
        }
        return true;
    }

    std::string&& TextChunker::get_chunk_from_current_stream() {
        char   buffer[CHUNK_SIZE];
        m_current.read(buffer, CHUNK_SIZE);
        if (m_current.gcount() <= 0) {
            m_current.close();
            return {};
        }
        std::string result(buffer, m_current.gcount());
        if (result.size() == CHUNK_SIZE && result.back() != '\n') {
            std::string helper;
            std::getline(m_current, helper);
            result += helper;
        }
        return std::move(result);
    }

    std::string&&   TextChunker::get_chunk() {
        std::string result;
        do {
            if (!m_current.is_open())
                open_subsequent_file();
            result = get_chunk_from_current_stream();
        } while (!check_index_end() && result.empty());
        return std::move(result);
    }

}