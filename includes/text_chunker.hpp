#ifndef TEXT_CHUNKER_HPP
# define TEXT_CHUNKER_HPP

# include <vector>
# include <string>

namespace datascope {

    /** \brief Class for reading files and splitting their content into chunks. */
    class   TextChunker {
        public:
            static  std::vector<std::string>&&    extract_chunks(const std::vector<std::string>&);
    };
}

#endif