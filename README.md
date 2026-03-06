# DataScope - Asynchronous Market Data Parser Library

## Overview

**DataScope** is a high-performance C++20/23 library designed for parsing and processing large-scale financial market data from CSV files. It specializes in handling order book level updates and trade data with minimal memory footprint and maximum processing efficiency through template-based selective data storage and multi-threaded asynchronous parsing.

The library is ideal for quantitative trading systems, market data services, and financial data analysis platforms that need to process massive amounts of market data with selective field extraction.

## Key Features

- **Selective Field Storage**: Only allocate memory for fields you need using compile-time template specialization
- **Zero-Cost Abstraction**: Template-based design ensures no runtime overhead for unused fields
- **Asynchronous Processing**: Process multiple CSV files concurrently using a configurable thread pool
- **Efficient Memory Management**: Chunk-based file reading with minimal memory footprint
- **Type-Safe Parsing**: Compile-time type safety with CSV header validation
- **Large Dataset Support**: Designed to handle gigabytes of market data efficiently
- **Flexible Data Types**: Support for various financial data formats (level data, trade data, and custom combinations)
- **High Throughput**: ~490k rows/second on large datasets (250+ MB)

## Architecture Overview

### Core Components

#### 1. **SelectiveStorage** (`selective_storage.hpp`)
Implements the **Template Specialization Pattern** with **Empty Base Optimization (EBO)** for memory-efficient data storage.

**Features:**
- `AccFlags` enum: Bitwise flags determining which fields to parse
- `DataAccumulator<F>`: Template structure that dynamically includes fields based on flags
- Unused fields consume zero memory
- All field selection happens at compile-time

**Supported Fields:**
- `RECEIVE_TS`: Reception timestamp (uint64_t)
- `EXCHANGE_TS`: Exchange timestamp (uint64_t)
- `PRICE`: Asset price (double)
- `QUANTITY`: Order quantity (double)
- `SIDE`: Order side - bid/ask (bool)
- `REBUILD`: Rebuild flag (bool)

**Pre-defined Combinations:**
- `LEVEL`: Complete level book data (all fields)
- `TRADE`: Trade data (receive_ts, exchange_ts, price, quantity, side)
- `RECEIVE_TS_PRICE_PAIR`: Timestamp-price pairs
- `PRICE_QUANTITY_PAIR`: Price-quantity pairs
- `PRICE_SIDE_PAIR`: Price-side pairs

#### 2. **MarketDataParser** (`market_data_parser.hpp`)
Implements the **Template Method Pattern** with **Strategy Pattern** for flexible parsing logic.

**Features:**
- Static template-based parse function for type-safe data extraction
- CSV parsing with semicolon-separated fields
- Robust error handling with debug logging
- Field extraction utilities:
  - `field_jumper()`: Navigate to specific field position
  - `field_length()`: Calculate field length
  - `get_field()`: Extract specific field from CSV line
- Extensible architecture for custom field types

**Usage:**
```cpp
// Parse CSV chunk into LEVEL data
auto parsed = MarketDataParser::parse<AccFlags::LEVEL>(csv_chunk);

// Parse CSV chunk into TRADE data
auto trades = MarketDataParser::parse<AccFlags::TRADE>(csv_chunk);
```

#### 3. **TextChunker** (`text_chunker.hpp`)
Implements the **Iterator Pattern** with **Lazy Evaluation** for efficient file reading.

**Features:**
- Reads CSV files in 4KB chunks to minimize memory usage
- Transparent handling of multiple file switching
- CSV header validation with type-specific regex patterns
- Maintains file state and processing progress
- Handles file stream errors gracefully

**Key Methods:**
- `get_chunk()`: Retrieves next 4KB chunk of data
- `finished()`: Checks if all files have been processed
- `state()`: Returns current file index

#### 4. **AsyncParser** (`async_parser.hpp`)
Implements the **Thread Pool Pattern** with **Producer-Consumer Pattern** for concurrent processing.

**Features:**
- Configurable number of worker threads (hardware concurrency by default)
- Automatic load balancing between file reading and parsing threads
- Thread-safe task queue (std::queue with mutex protection)
- Condition variable-based thread synchronization
- Supports callback function for processing results

**Architecture:**
- **Generator Thread**: Reads files in chunks and queues tasks
- **Worker Threads**: Process CSV chunks and invoke callback
- **Thread-Safe Queue**: Protects task distribution

**Key Methods:**
- `collect()`: Main entry point for processing multiple files

---

### 2. **MarketDataParser** - Data Parser

```cpp
class MarketDataParser {
    template <AccFlags F>
    static std::vector<DataAccumulator<F>> parse(const std::string& chunk);
    
    template <AccFlags F>
    static DataAccumulator<F> parse_line(const std::string& line);
};
```

**Purpose:** Transforms CSV rows into typed data structures.

**Features:**
- **Template specialization** — different implementations for each data type (RECEIVE_TS_PRICE_PAIR, LEVEL, TRADE, etc.)
- **Error handling** — invalid rows are skipped with logging
- **Performance** — minimal runtime type checks

**Specialization Examples:**
```cpp
// For pair (receive timestamp + price)
template <>
DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR> 
MarketDataParser::parse_line<AccFlags::RECEIVE_TS_PRICE_PAIR>(const std::string& line);

// For full order book level data
template <>
DataAccumulator<AccFlags::LEVEL> 
MarketDataParser::parse_line<AccFlags::LEVEL>(const std::string& line);
```

---

### 3. **DataAccumulator<F>** - Flexible Data Structure

```cpp
template <AccFlags F>
struct DataAccumulator : 
    std::conditional_t<...RECEIVE_TS...>,
    std::conditional_t<...EXCHANGE_TS...>,
    std::conditional_t<...PRICE...>,
    std::conditional_t<...QUANTITY...>,
    std::conditional_t<...SIDE...>,
    std::conditional_t<...REBUILD...>
{};
```

**Purpose:** Stores only needed fields based on compile-time flag.

**Concept:** **Empty Base Optimization (EBO)** + **Conditional Type Selection**

**Advantages:**
- If `PRICE` flag is not set, Price field takes no memory
- Structure size = sum of only active field sizes
- Zero runtime overhead — all decisions made by compiler

**Examples:**
```cpp
// RECEIVE_TS_PRICE_PAIR: only receive_ts + price (~16 bytes)
DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR> data1;

// LEVEL: all fields receive_ts + exchange_ts + price + qty + side + rebuild (~48 bytes)
DataAccumulator<AccFlags::LEVEL> data2;
```

---

### 4. **TextChunker<T>** - Chunk Splitter

```cpp
template <AccFlags T>
class TextChunker
```

**Purpose:** Reads files and forms chunks for efficient processing.

**How it works:**
- Opens files sequentially
- Groups multiple rows into one chunk
- Optimizes memory usage and I/O operations

---

### 5. **ConsoleMenu<T>** - Interactive Interface

```cpp
template <AccFlags T>
class ConsoleMenu
```

**Purpose:** Provides menu for selecting statistical operations (median, min, max).

**Features:**
- Compute median, min, and max values of fields via Boost.Accumulators
- Export results to CSV log files
- Display processed data information

---

## Design Patterns Used

### 1. **Template Specialization (Iterator Pattern + Strategy)**
Each data type has its own parser implementation:
```cpp
// Generic version (should not be called)
template <AccFlags F>
static DataAccumulator<F> parse_line(const std::string&) {
    static_assert(sizeof(DataAccumulator<F>) == 0, "Not specialized");
}

// Specialized version for RECEIVE_TS_PRICE_PAIR
template <>
static DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR> 
parse_line<AccFlags::RECEIVE_TS_PRICE_PAIR>(const std::string& line) {
    // Type-specific parsing logic
}
```

### 2. **Callback/Function Object (Observer Pattern)**
Result processing via `std::function`:
```cpp
parser.collect(files, [&result](std::vector<DataAccumulator<T>> parsed) {
    result.insert(result.end(), 
                  std::make_move_iterator(parsed.begin()), 
                  std::make_move_iterator(parsed.end()));
});
```

Benefits:
- Decouples parser from processing logic
- Enables any callable object (lambdas, functors, functions)
- Flexibility in result handling

### 3. **Producer-Consumer (Task Queue)**
Multi-threaded architecture with role separation:
- **Producer** (`task_generating`): reads files, adds chunks to queue
- **Consumer** (`task_processing`): retrieves chunks from queue, parses, invokes callback

Synchronization:
- `std::queue` for safe chunk storage
- `std::mutex` + `std::condition_variable` for thread coordination
- `std::jthread` + `std::stop_token` for lifecycle management

### 4. **Comparator Strategy (Template Specialization)**
Each type has its own comparator for sorting:
```cpp
template <>
struct __compare<AccFlags::RECEIVE_TS_PRICE_PAIR> {
    bool operator()(const DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR>& a,
                    const DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR>& b) const {
        return a.receive_ts < b.receive_ts;
    }
};
```

### 5. **Logging with Decorator Pattern (spdlog)**
All events logged via spdlog with different levels:
```cpp
spdlog::info("Processing file: {} size: {} bytes", file, size);
spdlog::debug("Parse error: {}", e.what());
spdlog::error("data doesn't parsed from input files");
```

---

## 📊 Design Patterns|Concepts Summary

| File | Patterns / Concepts | Purpose |
|------|----------|---------|
| `selective_storage.hpp` | EBO, Conditional Types, Template Specialization | Optimized storage |
| `market_data_parser.hpp` | Template Specialization, Strategy | CSV parsing to different types |
| `text_chunker.hpp` | Iterator, Lazy Evaluation | Chunk-based file reading |
| `async_parser.hpp` | Producer-Consumer, Thread Pool | Asynchronous processing |
| `main.cpp` | Callback/Observer, Comparator Strategy | Component coordination |

**Conclusion:** DataScope uses **6 core design patterns** that work together to achieve high performance, type safety, and flexibility! 🚀

---

## How to Use in Demo

### Step 1: Create Parser and Callback

```cpp
// Select data type for parsing
AsyncParser<datascope::AccFlags::RECEIVE_TS_PRICE_PAIR> parser;

// Container for results
std::vector<datascope::DataAccumulator<datascope::AccFlags::RECEIVE_TS_PRICE_PAIR>> result;

// Define callback function for result processing
auto callback = [&result](std::vector<datascope::DataAccumulator<datascope::AccFlags::RECEIVE_TS_PRICE_PAIR>> parsed) {
    // Move results to finalized container
    result.insert(result.end(), 
                  std::make_move_iterator(parsed.begin()), 
                  std::make_move_iterator(parsed.end()));
};
```

### Step 2: Run Asynchronous Processing

```cpp
// Get file paths from configuration
auto files = config_provider.get_input_files();

// Start processing with callback
// - Logs file size for each file
// - Distributes processing across threads
// - Invokes callback for each batch of results
// - Returns after all threads complete
parser.collect(files, callback);
```

### Step 3: Process Results

```cpp
// Verify data was obtained
if (result.empty())
    throw std::runtime_error("No data parsed");

// Sort results (using specialized comparator)
std::sort(result.begin(), result.end(), 
          datascope::DataAccumulatorLess<datascope::AccFlags::RECEIVE_TS_PRICE_PAIR>());

// Log information
spdlog::info("Parsing finished: parsed {} valid lines", result.size());

// Pass to console menu for further processing
ConsoleMenu<datascope::AccFlags::RECEIVE_TS_PRICE_PAIR> menu(std::move(result), 
                                                              config_provider.get_output_dirs());
menu.switchConsoleMenu();
```

---

## Configuration Parameters (default_config.toml)

```toml
[input]
files = [
    "examples/input/level.csv",
    "examples/input/trade.csv",
    "examples/input/large_level.csv",
    "examples/input/large_trade.csv"
]

[output]
directories = ["examples"]

[processing]
work_mode = "RECEIVE_TS_PRICE_PAIR"
```

---

## Data Flags (AccFlags)

```cpp
enum class AccFlags : uint32_t {
    RECEIVE_TS  = 1 << 0,   // Receive timestamp
    EXCHANGE_TS = 1 << 1,   // Exchange timestamp
    PRICE       = 1 << 2,   // Price
    QUANTITY    = 1 << 3,   // Quantity
    SIDE        = 1 << 4,   // Side (bid/ask)
    REBUILD     = 1 << 5,   // Rebuild flag
    
    // Combinations for typical scenarios:
    RECEIVE_TS_PRICE_PAIR   = RECEIVE_TS | PRICE | (1 << 6),      // Timestamp + Price
    PRICE_QUANTITY_PAIR     = PRICE | QUANTITY | (1 << 7),         // Price + Quantity
    PRICE_SIDE_PAIR         = PRICE | SIDE | (1 << 8),             // Price + Side
    LEVEL = RECEIVE_TS | EXCHANGE_TS | PRICE | QUANTITY | SIDE | REBUILD, // Full data
    TRADE = RECEIVE_TS | EXCHANGE_TS | PRICE | QUANTITY | SIDE   // Trade data
};
```

---

## CSV Formats

### level.csv (Order Book Data)
```
receive_ts;exchange_ts;price;quantity;side;rebuild
1716810808593627;1716810808574000;68480.00000000;10.10900000;bid;1
...
```

### trade.csv (Trade Data)
```
receive_ts;exchange_ts;price;quantity;side
1716810808663260;1716810808661000;68480.10000000;0.01100000;bid
...
```

---

## Performance

### Testing on 250+ MB Data

```
Input files:
- large_level.csv: 134 MB (2M rows)
- large_trade.csv: 128 MB (2M rows)
- Total: 4M+ valid lines

Results:
- Parsing time: 8.173 seconds
- Throughput: ~490,000 lines/sec
- Memory: Optimized via EBO
- Threads: Auto-scaled to CPU core count
```

---

## Build and Run

### Requirements
- C++23 (std::jthread, std::stop_token, std::format)
- CMake 3.23+
- Boost (program_options, accumulators)
- spdlog (for logging)
- toml++ (for configuration)

### Build
```bash
cd /home/assam04/Desktop/DataScope
mkdir -p build && cd build
cmake ..
make
```

### Run
```bash
cp ../default_config.toml ./
./stats_logger
```

### With Custom Config
```bash
./stats_logger --config path/to/config.toml
# or
./stats_logger --cfg path/to/config.toml
```

---

## Pattern Usage Examples

### 1. Using Custom Callback
```cpp
AsyncParser<AccFlags::PRICE_QUANTITY_PAIR> parser;

parser.collect(files, [](auto parsed) {
    for (const auto& item : parsed) {
        std::cout << "Price: " << item.price 
                  << ", Qty: " << item.quantity << "\n";
    }
});
```

### 2. Using Functor as Callback
```cpp
struct DataProcessor {
    void operator()(std::vector<DataAccumulator<AccFlags::LEVEL>> parsed) {
        for (const auto& item : parsed) {
            process(item);
        }
    }
    void process(const auto& item) { /* ... */ }
};

parser.collect(files, DataProcessor());
```

### 3. Using Regular Function as Callback
```cpp
void process_batch(std::vector<DataAccumulator<AccFlags::TRADE>> parsed) {
    // Process batch data
}

parser.collect(files, process_batch);
```

---

## Extensions and Improvements

### Possible Extension Modules:

1. **Time-series Analysis** — analyze quote time-series
2. **Real-time Streaming** — integration with live data feeds
3. **Database Export** — save to databases (PostgreSQL, SQLite)
4. **Machine Learning** — integration with ML models
5. **Rest API** — HTTP server for data access
6. **WebSocket Handler** — handle streaming data updates

---

## Contact and Support

This project is used for processing financial market data. 
For questions and suggestions, please create issues in the repository.

---

## Version History

### v1.3.0 (2026-03-04)
- ✅ Asynchronous parser with multi-threading
- ✅ Callback-based architecture via std::function
- ✅ Compile-time optimization via AccFlags
- ✅ File size logging via std::for_each + spdlog
- ✅ Support for >250MB data
- ✅ Performance ~490k lines/sec
