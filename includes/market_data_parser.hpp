#ifndef MARKET_DATA_PARSER_HPP
# define MARKET_DATA_PARSER_HPP

# include <vector>
# include <expected>
# include <sstream>
# include <exception>
# include <format>
//# include <spdlog/spdlog.h>
# include "selective_storage.hpp"

namespace datascope {

    /**
     *  @class MarketDataParser
     *  @brief Market data parser for CSV format
     *  
     *  This class is designed as an extensible architecture based on template specializations.
     *  Each specialization of AccFlags defines unique parsing logic for a specific data type.
     * 
     *  The architecture allows for easy extension by adding new specializations without modifying.
     *  This follows the Open/Closed Principle.
     * 
     *  To add new parsing logic:
     * 1. Define a new flag in the AccFlags enum
     * 2. Create a template specialization parse<YourFlag>()
     * 3. Implement your custom parsing logic in the specialization
     * 4. The parsing logic will be completely isolated and independent
     * 
     */
    class MarketDataParser {
        public:
            MarketDataParser() = delete;
            /**
             *  @brief Template static parse function
             *  @details function get CSV format chunk and call foreach line parse_line specializated function with type F.
             */
            template <AccFlags F>
            static std::expected<std::vector<DataAccumulator<F>>, bool> parse(const std::string& chunk) {
                std::vector<DataAccumulator<F>> parsed;
                std::istringstream is(chunk);
                std::string line;        
                while (std::getline(is, line)) {
                    if (line.empty())
                        continue; 
                    try {
                        parsed.push_back(parse_line<F>(line));
                    }
                    catch (const std::exception& e) {
                      //  spdlog::debug(std::format("Parse error: {}", e.what()));
                    }
                    catch (...) {
                      //  spdlog::debug(std::format("Parse error: Unknown"));
                    }
                }
                return parsed;
            }

        private:
            /**
             *  @brief  static field_jumper function
             *  @details returns position after jumping set count(count of iteration) of sep(separatos) in s(line).
             *          if count is much than separators are in string function throws exception.
             */
            static size_t field_jumper(const std::string& s, size_t count, char sep = ';') {
                size_t  cursor = 0;
                while (count--) {
                    cursor = s.find_first_of(sep, cursor);
                    if (cursor == std::string::npos)
                        throw std::logic_error(std::format("Missing separator '{}' at parsed line", sep));
                    ++cursor;
                }
                return cursor;
            }

            /**
             *  @brief  static field_length function
             *  @details returns field length that stated with start_pos and ended with sep(character) in s(line).
             *          if sep is not fount in s function returns s.length() - start_pos.
             */
            static size_t field_length(const std::string& s, size_t start_pos, char sep = ';') {
                auto end_pos = s.find_first_of(sep, start_pos);
                if (end_pos == std::string::npos)
                    end_pos = s.length();
                return end_pos - start_pos;
            }

            /**
             *  @brief static get_field function
             *  @details get line and n (n is a count for jumping seps).
             *          Returns string that started with filed_jumper(line, n) until separator.
             */
            static std::string  get_field(const std::string& line, size_t n) {
                auto    start_pos = field_jumper(line, n);
                return line.substr(start_pos, field_length(line, start_pos));
            }
            /**
             *  @brief Template static method
             *  @details The function parses a single string into CSV format.
             *          With the help of template implementation, it is possible to extend the 
             *          functionality of the program by specializing this function.
             *          The general template returns an empty vector.
             */
            template <AccFlags F>
            static DataAccumulator<F> parse_line(const std::string&) {
                return {};
            }
    };


    /**
    *  @brief Specializing AccFlags::PRICE_LOG function parse_line
    *  @details Used when in our area of interest the values receive_ts and price are used,
    *          while the rest of the values are discarded. It is important to note that the parsing
    *          function works according to the rule that the CSV format is receive_ts; x; price; ...
    */
    template <>
    inline DataAccumulator<AccFlags::PRICE_LOG>    MarketDataParser::parse_line<AccFlags::PRICE_LOG>(const std::string& line) {
        constexpr size_t    receive_ts_start = 0, price_start = 2;
        DataAccumulator<AccFlags::PRICE_LOG> field;
        field.receive_ts = std::stoull(get_field(line, receive_ts_start));
        field.price = std::stod(get_field(line, price_start));
        return field;
    }
}

#endif