#ifndef ASYNC_PARSER_HPP
# define ASYNC_PARSER_HPP

# include <queue>
# include <atomic>
# include <thread>
# include <mutex>
# include <condition_variable>
# include <algorithm>
# include <expected>
# include <functional>
# include <filesystem>
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

            void  collect(std::vector<std::string> files, std::function<void(std::vector<DataAccumulator<T>>)> func) {
                std::for_each(files.begin(), files.end(), [](const std::string& file) {
                    try {
                        auto size = std::filesystem::file_size(file);
                        spdlog::info("Processing file: {} size: {} bytes", file, size);
                    } catch (const std::filesystem::filesystem_error& e) {
                        spdlog::warn("Could not get size for file: {} error: {}", file, e.what());
                    }
                });
                process_launch(files, func);
                wait_for_end();
                notify_all_stop();
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
       
            void    task_processing(std::stop_token st, std::function<void(std::vector<DataAccumulator<T>>)> func) {
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
                        func(result);
                    }
                }
            }
       
            void    process_launch(std::vector<std::string>& files, std::function<void(std::vector<DataAccumulator<T>>)> func) {
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
                    workers.emplace_back([this, func](std::stop_token st) { this->task_processing(st, func); });
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
        private:
            std::queue<std::string> tasks;
            std::vector<std::jthread>   workers;
            std::condition_variable cv;
            std::mutex  data_mutex;
            std::mutex  tasks_mutex;
            std::atomic<int>  generate_threads_count;
            size_t  process_threads_count;
    };
}

#endif