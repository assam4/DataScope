#ifndef TEXT_CHUNKER_HPP
# define TEXT_CHUNKER_HPP

# include <fstream> 
# include <vector>
# include <regex>
# include <format>
# include <spdlog/spdlog.h>
# include "selective_storage.hpp"  

namespace datascope {

    inline static constexpr unsigned CHUNK_SIZE = 4096;

    /**
     * @brief Efficient CSV file reader with header validation and chunked processing
     * @tparam T Template parameter for AccFlags determining valid header format
     * 
     * Reads multiple files chunk-by-chunk to minimize memory footprint. Validates CSV headers
     * against type-specific regex patterns and handles file switching transparently.
     */
    template<AccFlags T>
    class   TextChunker {
        public:
             /**
             * @brief Constructs a TextChunker with a list of files and their type.
             * @param files Vector of file paths to process.
             * @param type The type of files, used to select the appropriate header regex.
             */
            TextChunker(const std::vector<std::string>& files): m_files(files), m_index(-1), m_finished(false) {
            }
            ~TextChunker() = default;
            TextChunker(const TextChunker&) = delete;
            TextChunker& operator=(const TextChunker&) = delete;
            bool    finished() const noexcept { return m_finished; }
            int     state() const noexcept { return m_index; }
            std::string get_chunk();
        private:
            void    open_subsequent_file();
            bool    check_index_end() const noexcept { return m_index == static_cast<int>(m_files.size()) - 1; }
            bool    check_header(const std::string&) const;
            bool    check_stream_state();
            std::string get_chunk_from_current_stream();
        private:
            const std::vector<std::string> m_files;
            std::ifstream   m_current;
            int m_index;  // -1 means no file opened yet
            bool m_finished;
    };

    template <AccFlags T>
    void TextChunker<T>::open_subsequent_file() {
        do {
            if (m_current.is_open())
                m_current.close();
            if (m_index == static_cast<int>(m_files.size() - 1))
                m_finished = true;
            else
                m_current.open(m_files[++m_index]);
        }
        while (!check_stream_state());
    }

    template <AccFlags T>
    bool TextChunker<T>::check_stream_state() {
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

    template <AccFlags T>
    std::string TextChunker<T>::get_chunk_from_current_stream() {
        char buffer[CHUNK_SIZE];
        m_current.read(buffer, CHUNK_SIZE);
        if (m_current.gcount() <= 0) {
            m_current.close();
            return "";
        }
        std::string result(buffer, m_current.gcount());
        if (result.size() == CHUNK_SIZE && result.back() != '\n') {
            std::string helper;
            std::getline(m_current, helper);
            result += helper;
        }
        return result;
    }

    template <AccFlags T>
    std::string TextChunker<T>::get_chunk() {
        std::string result;
        do {
            if (!m_current.is_open())
                open_subsequent_file();
            result = get_chunk_from_current_stream();
        } while (!check_index_end() && result.empty());
        return result;
    }

    template <>
    inline bool TextChunker<AccFlags::RECEIVE_TS_PRICE_PAIR>::check_header(const std::string& header) const {
        return std::regex_match(header, std::regex("^receive_ts;[^;]*;price(;.*)?$"));
    }

    template <>
    inline bool TextChunker<AccFlags::PRICE_QUANTITY_PAIR>::check_header(const std::string& header) const {
        return std::regex_match(header, std::regex("[^;]*;[^;]*;price;quantity(;.*)?$"));
    }

    template <>
    inline bool TextChunker<AccFlags::PRICE_SIDE_PAIR>::check_header(const std::string& header) const {
        return std::regex_match(header, std::regex("[^;]+;[^;]+;price;[^;]*;side(;.*)?$"));
    }

    template <>
    inline bool TextChunker<AccFlags::LEVEL>::check_header(const std::string& header) const {
        return std::regex_match(header, std::regex("^receive_ts;exchange_ts;price;quantity;side(;.*)?$"));
    }

    template <>
    inline bool TextChunker<AccFlags::TRADE>::check_header(const std::string& header) const {
        return std::regex_match(header, std::regex("^receive_ts;exchange_ts;price;quantity;side(;.*)?$"));
    }
}

#endif