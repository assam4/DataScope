#ifndef TEXT_CHUNKER_HPP
# define TEXT_CHUNKER_HPP

# include <fstream> 
# include <vector>
# include <string>
# include <regex>
  

namespace datascope {

    /** 
     * @brief Class for reading files and splitting their content into chunks.
     * 
     * This class processes a list of files, validates their headers using regex patterns
     * based on the specified FileType, and reads content in chunks to handle large files
     * efficiently without loading everything into memory.
     */
    class   TextChunker {
        public:
             /**
             * @brief Constructs a TextChunker with a list of files and their type.
             * @param files Vector of file paths to process.
             * @param type The type of files, used to select the appropriate header regex.
             */
            TextChunker(const std::vector<std::string>&, AccFlags);
            ~TextChunker() = default;
            TextChunker(const TextChunker&) = delete;
            TextChunker& operator=(const TextChunker&) = delete;
            bool    finished() const noexcept { return m_finished; }
            int     state() const noexcept { return m_index; }
            std::string&& get_chunk();
        private:
            void    open_subsequent_file();
            bool    check_index_end() const noexcept { return m_index == m_files.size() - 1; }
            bool    check_header(const std::string& header) const { return std::regex_match(header, m_regex); }
            std::string&& get_chunk_from_current_stream();
        private:
            const std::vector<std::string> m_files;
            std::ifstream   m_current;
            std::regex  m_regex;
            int m_index;
            bool m_finished;
    };
}

#endif