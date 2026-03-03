#ifndef MARKET_DATA_PARSER_HPP
# define MARKET_DATA_PARSER_HPP

# include <vector>
# include <sstream>
# include <exception>
# include <format>
# include <spdlog/spdlog.h>
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
            static std::vector<DataAccumulator<F>   parse(const std::string& chunk) {
                std::vector<DataAccumulator<F>> parsed;
                std::istringstream  is(chunk);
                std::string line;        
                while (std::getline(is, line)) {
                    if (line.empty())
                        continue; 
                    try {
                        parsed.push_back(parse_line<F>(line));
                    }
                    catch (const std::exception& e) {
                        spdlog::debug(std::format("Parse error: {}", e.what()));
                    }
                    catch (...) {
                        spdlog::debug(std::format("Parse error: Unknown"));
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
     *  @brief Combination for collecting receive timestamp and price fields.
     *      Suitable for time-series price logging with local reception timestamps.
     *      Enables calculations like price changes over time or latency analysis.
     *      It is important to note that the parsing function works according to the rule that the CSV format is
     *      " receive_ts; x; price; ... ".
    */
    template <>
    inline DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR>    MarketDataParser::parse_line<AccFlags::RECEIVE_TS_PRICE_PAIR>(const std::string& line) {
        constexpr size_t    receive_ts_start = 0, price_start = 2;
        DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR> field;
        field.receive_ts = std::stoull(get_field(line, receive_ts_start));
        field.price = std::stod(get_field(line, price_start));
        return field;
    }

    /**
     *  @brief Combination for collecting price and quantity fields.
     *      Useful for volume-price analysis, such as VWAP or order size tracking.
     *      Allows aggregating trade volumes at specific price levels.
     *      It is important to note that the parsing function works according to the rule that the CSV format is
     *      " x; x; price; quantity; ... ".
    */
    template <>
    inline DataAccumulator<AccFlags::PRICE_QUANTITY_PAIR>   MarketDataParser::parse_line<AccFlags::PRICE_QUANTITY_PAIR>(const std::string& line) {
        constexpr size_t    price_start = 2, quantity_start = 3;
        DataAccumulator<AccFlags::PRICE_QUANTITY_PAIR>  field;
        field.price = std::stod(get_field(line, price_start));
        field.quantity = std::stod(get_field(line, quantity_start));
        return field;
    }

    /**
     *  @brief Combination for collecting price and side fields.
     *      Direction analysis: Allows you to track how prices change during purchases/sales
     *       (for example, whether prices rise on buy or fall on sell).
     *      Indicators: Suitable for calculating momentum, RSI, or directional price movements.
     *      Calculation examples: Average price on each side; identification of the dominant side (bullish/bearish pressure).
     *      It is important to note that the parsing function works according to the rule that the CSV format is
     *      " x; x; price; x; side; ... ".
     */
    template <>
    inline DataAccumulator<AccFlags::PRICE_SIDE_PAIR>   MarketDataParser::parse_line<AccFlags::PRICE_SIDE_PAIR>(const std::string& line) {
        constexpr size_t    price_start = 2, side_start = 4;
        DataAccumulator<AccFlags::PRICE_SIDE_PAIR> field;
        field.price = std::stod(get_field(line, price_start));
        field.side = (get_field(line, side_start) == "bid") ? true : false;
        return field;
    }

    /**
    *   @brief Specialization for order book level data.
    *         Parses receive_ts, exchange_ts, price, quantity, side.
    *         It is important to note that the parsing function works according to the rule that the CSV format is
    *         " receive_ts; exchange_ts; price; quantity; side; ... ".
    */
    template<>
    inline DataAccumulator<AccFlags::LEVEL> MarketDataParser::parse_line<AccFlags::LEVEL>(const std::string& line) {
        constexpr size_t    receive_ts_start = 0, exchange_ts_start = 1, price_start = 2, quantity_start = 3, side_start = 4;
        DataAccumulator<AccFlags::LEVEL>    field;
        field.receive_ts = std::stoull(get_field(line, receive_ts_start));
        field.exchange_ts = std::stoull(get_field(line, exchange_ts_start));
        field.price = std::stod(get_field(line, price_start));
        field.quantity = std::stod(get_field(line, quantity_start));
        field.side = (get_field(line, side_start) == "bid") ? true : false;
        return field;
    }

    /**
    *   @brief Specialization for order book level data.
    *         Parses receive_ts, exchange_ts, price, quantity, side, rebuild.
    *         It is important to note that the parsing function works according to the rule that the CSV format is
    *         " receive_ts; exchange_ts; price; quantity; side; rebuild; ... ".
    */
    template<>
    inline DataAccumulator<AccFlags::TRADE> MarketDataParser::parse_line<AccFlags::TRADE>(const std::string& line) {
        constexpr size_t    receive_ts_start = 0, exchange_ts_start = 1, price_start = 2, quantity_start = 3, side_start = 4, rebuild_start = 5;
        DataAccumulator<AccFlags::TRADE>    field;
        field.receive_ts = std::stoull(get_field(line, receive_ts_start));
        field.exchange_ts = std::stoull(get_field(line, exchange_ts_start));
        field.price = std::stod(get_field(line, price_start));
        field.quantity = std::stod(get_field(line, quantity_start));
        field.side = (get_field(line, side_start) == "bid") ? true : false;
        field.rebuild = static_cast<bool>(std::stoi(get_field(line, rebuild_start)));
        return field;
    }
}

#endif