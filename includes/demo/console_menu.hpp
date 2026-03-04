#ifndef CONSOLE_MENU_HPP
# define CONSOLE_MENU_HPP

# include <fstream>
# include <iostream>
# include <spdlog/spdlog.h>
# include <format>
# include <boost/accumulators/accumulators.hpp>
# include <boost/accumulators/statistics/stats.hpp>
# include <boost/accumulators/statistics/median.hpp>
# include <boost/accumulators/statistics/min.hpp>
# include <boost/accumulators/statistics/max.hpp>
# include "selective_storage.hpp"

/**
 * @class ConsoleMenu
 * @brief Interactive console interface for data analysis and log generation
 * @details Provides a menu-driven interface allowing users to select statistical
 * operations (median, min, max) on processed data. Generates CSV log files
 * with computed values and timestamps for further analysis.
 */

 using namespace datascope;

template <AccFlags T>
class   ConsoleMenu {
    public:
        /**
         * @brief Constructs a ConsoleMenu with processed data and output paths
         * @param data Vector of DataAccumulator objects containing parsed data
         * @param output Vector of output directory paths for log files
         */
        ConsoleMenu(std::vector<DataAccumulator<T>>&& data, std::vector<std::string>&& output)
                : m_data(std::move(data)), m_out(std::move(output)) {}
        ConsoleMenu(const ConsoleMenu&) = delete;
        ConsoleMenu&    operator=(const ConsoleMenu&) = delete;
        /**
         * @brief Starts the interactive console menu loop
         * @details Displays menu options and processes user input until exit
         */
        void    switchConsoleMenu() const {
            spdlog::info(std::format("Data processed. Program ready to perform tasks."));
            spdlog::info(std::format("Select the desired log (the result will be saved to the specified path)"));
            while (true) {
                int index = -1;
                spdlog::info(std::format("Insert\n'1' - for median logs\n'2' - for min logs\n'3' - for max logs\n'0' - for exit"));
                if (!(std::cin >> index) || index < 0 || index > 3) {
                    spdlog::warn(std::format("Incorrect entry. Please be more careful."));
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                    continue;
                }
                switch (index) {
                    case 1:
                        getPriceMedian();
                        break;
                    case 2:
                        getPriceMin();
                        break;
                    case 3:
                        getPriceMax();
                        break;
                    default:
                        spdlog::info(std::format("We hope you are satisfied. The program has been suspended."));
                        return;
                }
            }
        }
    private:
        void    getPriceMedian() const {
            using namespace boost::accumulators;
            for (const auto& output_path: m_out) {
                std::ofstream os(output_path + "_price_median.log");
                if (!os.is_open()) {
                    spdlog::error(std::format("Failed to open output file: {}median.log", output_path));
                    return;
                }
                int count = 1;
                os << "receive_ts;price_median\n";
                accumulator_set<double, stats<tag::median(with_p_square_quantile)>> acc;
                double current_median = std::numeric_limits<double>::quiet_NaN();
                for (const auto& x : this->m_data) {
                    acc(x.price);
                    double new_median = (count < 3) ? x.price : median(acc);
                    if (std::isnan(current_median) || current_median != new_median) {
                        os << std::to_string(x.receive_ts) << ";" << std::to_string(new_median) << '\n';
                        current_median = new_median;
                        ++count;
                    }
                }
                spdlog::info(std::format("Median log written to: {}median.log - {} lines", output_path, std::to_string(count)));
            }
        }
        void    getPriceMin() const {
            for( const auto& output_path: m_out) {
                using namespace boost::accumulators;
                std::ofstream os(output_path + "_price_min.log");
                if (!os.is_open()) {
                    spdlog::error(std::format("Failed to open output file: {}min.log", output_path));
                    return;
                }
                os << "receive_ts;price_min\n";
                accumulator_set<double, stats<tag::min>> acc;
                double current_min = std::numeric_limits<double>::quiet_NaN();
                int count = 1;
                for (const auto& x : this->m_data) {
                    acc(x.price);
                    double new_min = min(acc);
                    if (std::isnan(current_min) || new_min < current_min) {
                        os << std::to_string(x.receive_ts) << ";" << std::to_string(new_min) << '\n';
                        current_min = new_min;
                        ++count;
                    }
                }
                spdlog::info(std::format("Min log written to: {}min.log - {} lines", output_path, std::to_string(count)));
            }
        }
        void    getPriceMax() const {
            for (const auto& output_path: m_out) {
                using namespace boost::accumulators;
                std::ofstream os(output_path + "_price_max.log");
                if (!os.is_open()) {
                    spdlog::error(std::format("Failed to open output file: {}max.log", output_path));
                    return;
                }
                os << "receive_ts;price_max\n";
                int count = 1;
                accumulator_set<double, stats<tag::max>> acc;
                double current_max = std::numeric_limits<double>::quiet_NaN();
                for (const auto& x : this->m_data) {
                    acc(x.price);
                    double new_max = max(acc);
                    if (std::isnan(current_max) || current_max < new_max) {
                        os << std::to_string(x.receive_ts) << ";" << std::to_string(new_max) << '\n';
                        current_max = new_max;
                    }
                }
                spdlog::info(std::format("Max log written to: {}max.log - {} lines", output_path, std::to_string(count)));
            }
        }
    private:
        std::vector<DataAccumulator<T>>   m_data;
        std::vector<std::string>    m_out;
};

#endif