#ifndef ASYNC_PARSER_HPP
# define ASYNC_PARSER_HPP

# include <queue>
# include <atomic>
# include <thread>
# include <mutex>
# include <condition_variable>
# include <algorithm>
# include <expected>
# include <spdlog/spdlog.h>
# include "market_data_parser.hpp"
# include "text_chunker.hpp"

namespace datascope {
    /**
     * @brief Asynchronous CSV data parser with multi-threaded processing
     * @tparam T Template parameter for AccFlags specifying which fields to parse
     * 
     * Processes multiple CSV files concurrently using thread pool. Distributes parsing load across
     * worker threads and collects results in a thread-safe manner. Suitable for large datasets.
     */
    template <AccFlags T>
    class   AsyncParser {
        public:
            AsyncParser(): generate_threads_count(1) {
                process_threads_count = std::thread::hardware_concurrency();
                if (!process_threads_count)
                    process_threads_count = 2;
                workers.reserve(process_threads_count);
            }
            ~AsyncParser() = default;
            AsyncParser(const AsyncParser &) = delete;
            AsyncParser &operator=(const AsyncParser &) = delete;

        std::expected<std::vector<DataAccumulator<T>>, bool>  collect(std::vector<std::string> files) {
            process_launch(files);
            wait_for_end();
            notify_all_stop();
            if (!data_conversion())
                return std::unexpected<bool>(false);
           else
                return shared_data;
        }

        private:
            void    task_generating(const std::vector<std::string>& files) {
                TextChunker<T> generator(files);
                std::string chunk;
                while (true) {
                    chunk = generator.get_chunk();
                    if (!chunk.empty()) {
                        std::lock_guard<std::mutex> lock(tasks_mutex);
                        tasks.emplace(std::move(chunk));
                        cv.notify_one();
                    }
                    else
                        break ;
                }
                --generate_threads_count;
            }
       
            void    task_processing(std::stop_token st) {
                while (!st.stop_requested()) {
                    std::unique_lock<std::mutex> lock(tasks_mutex);
                    cv.wait(lock, [this, &st] { return !tasks.empty() || st.stop_requested(); });
                    if (st.stop_requested() && tasks.empty())
                        return;
                    auto    work = tasks.front();
                    tasks.pop();
                    lock.unlock();
                    auto    result = MarketDataParser::parse<T>(work);
                    if (!result.empty()) {
                        std::lock_guard<std::mutex> result_lock(data_mutex);
                        shared_data.insert(shared_data.end(), std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
                    }
                }
            }
       
            void    process_launch(std::vector<std::string>& files) {
                auto    size = files.size();
                if (size > 3 && process_threads_count > 2) {
                    auto mid = size / 2; 
                    auto part_files = std::vector<std::string>(files.begin() + mid, files.end());
                    files.erase(files.begin() + mid, files.end());
                    workers.emplace_back([this, part_files]() { this->task_generating(part_files); });
                    --process_threads_count;
                    ++generate_threads_count;
                }
                for (size_t i = 0; i < process_threads_count; ++i)
                    workers.emplace_back([this](std::stop_token st) { this->task_processing(st); });
                task_generating(files);
            }

            void    wait_for_end() {
                while (true) {
                    {
                        if (!generate_threads_count) {
                            std::lock_guard<std::mutex> lock(tasks_mutex);
                            if (tasks.empty() && !generate_threads_count)
                                break ;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
    
            void    notify_all_stop() {
                for (auto& w : workers)
                    w.request_stop();
                cv.notify_all();
                workers.clear();
            }

            bool    data_conversion() {
                if (shared_data.empty()) {
                    spdlog::error("data doesn't parsed from input files");
                    return false;
                }
                std::sort(shared_data.begin(), shared_data.end(), DataAccumulatorLess<T>());
                spdlog::info(std::format("Parsing finished: parsed {} valid lines", std::to_string(shared_data.size())));
                return true;
            }

        private:
            std::queue<std::string> tasks;
            std::vector<DataAccumulator<T>> shared_data;
            std::vector<std::jthread>   workers;
            std::condition_variable cv;
            std::mutex  data_mutex;
            std::mutex  tasks_mutex;
            std::atomic<int>  generate_threads_count;
            size_t  process_threads_count;
    };
}

#endif