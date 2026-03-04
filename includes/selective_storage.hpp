#ifndef SELECTIVE_STORAGE_HPP
# define SELECTIVE_STORAGE_HPP

#include <string>
#include <cstdint>

namespace datascope {
    /**
    *   @brief  Bitwise flags for selective data storage from CSV
    *   
    *   Allows the compiler to choose at compile-time which fields
    *    should be present in the DataAccumulator structure for storing data.
    *
    *   @see DataAccumulator    
    */
    enum class  AccFlags : uint32_t {
        RECEIVE_TS  = 1 << 0,   ///< Reception timestamp
        EXCHANGE_TS = 1 << 1,   ///< Exchange timestamp
        PRICE       = 1 << 2,   ///< Asset price
        QUANTITY    = 1 << 3,   ///< Order quantity
        SIDE        = 1 << 4,   ///< Order side (bid/ask)
        REBUILD     = 1 << 5,   ///< Rebuild flag
        RECEIVE_TS_PRICE_PAIR   = RECEIVE_TS | PRICE | 1 << 6,   ///< Collecting receive_ts and price fields
        PRICE_QUANTITY_PAIR = PRICE | QUANTITY | 1 << 7, ///< Collecting price and quantity fields
        PRICE_SIDE_PAIR = PRICE | SIDE | 1 << 8, ///< Collecting price and side fields
        ///< Combination for order book level data
        LEVEL       = RECEIVE_TS | EXCHANGE_TS | PRICE | QUANTITY | SIDE | REBUILD,
        ///< Combination for order book trade data
        TRADE       = RECEIVE_TS | EXCHANGE_TS | PRICE | QUANTITY | SIDE 
    };

    /**
    *   @brief Empty placeholder for unused fields
    *   
    *   Used with Empty Base Optimization to reduce structure size
    *    when flags are not set.
     */
    struct __emptyReceiveTs {};
    struct __emptyExchangeTs {};
    struct __emptyPrice {};
    struct __emptyQuantity {};
    struct __emptySide {};
    struct __emptyRebuild {};

    /**
    *   @brief Named field structures
     */
    struct ReceiveTs { uint64_t receive_ts; };
    struct ExchangeTs { uint64_t exchange_ts; };
    struct Price { double price; };
    struct Quantity { double quantity; };
    struct Side { bool side; };
    struct Rebuild { bool rebuild; };

    /**
    *   @brief Data structure/accumulator with selective field storage
    *
    *   Template structure that uses bitwise flags (AccFlags) to determine
    *    at compile time which fields to include. Unused fields consume no memory.
    *   ( Empty Base Optimization)
    *
    *   @note structure size depends on set flags
    *   @note Zero rutime overhead - all operations are compile-time
     */
    template <AccFlags F>
    struct  DataAccumulator : 
        std::conditional_t<(static_cast<uint32_t>(F) & static_cast<uint32_t>(AccFlags::RECEIVE_TS)), ReceiveTs, __emptyReceiveTs>,
        std::conditional_t<(static_cast<uint32_t>(F) & static_cast<uint32_t>(AccFlags::EXCHANGE_TS)), ExchangeTs, __emptyExchangeTs>,
        std::conditional_t<(static_cast<uint32_t>(F) & static_cast<uint32_t>(AccFlags::PRICE)), Price, __emptyPrice>,
        std::conditional_t<(static_cast<uint32_t>(F) & static_cast<uint32_t>(AccFlags::QUANTITY)), Quantity, __emptyQuantity>,
        std::conditional_t<(static_cast<uint32_t>(F) & static_cast<uint32_t>(AccFlags::SIDE)), Side, __emptySide>,
        std::conditional_t<(static_cast<uint32_t>(F) & static_cast<uint32_t>(AccFlags::REBUILD)), Rebuild, __emptyRebuild>
    {};

    template <AccFlags F>
    struct  __compare { 
        bool operator()(const DataAccumulator<F>&, const DataAccumulator<F>&) const {
            static_assert(sizeof(DataAccumulator<F>) == 0, "Comparator not specialized for this AccFlags type");
            return false;
        }
    };

    template <>
    struct  __compare<AccFlags::RECEIVE_TS_PRICE_PAIR> {
        bool operator()(const DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR>& a, const DataAccumulator<AccFlags::RECEIVE_TS_PRICE_PAIR>& b) const {
            return a.receive_ts < b.receive_ts; 
        }
    };

    template <>
    struct  __compare<AccFlags::PRICE_QUANTITY_PAIR> {
        bool operator()(const DataAccumulator<AccFlags::PRICE_QUANTITY_PAIR>& a, const DataAccumulator<AccFlags::PRICE_QUANTITY_PAIR>& b) const {
            return a.price < b.price;
        }
    };

    template <>
    struct  __compare<AccFlags::PRICE_SIDE_PAIR> {
        bool operator()(const DataAccumulator<AccFlags::PRICE_SIDE_PAIR>& a, const DataAccumulator<AccFlags::PRICE_SIDE_PAIR>& b) const {
            return a.price < b.price;
        }
    };

    template <>
    struct  __compare<AccFlags::LEVEL> {
        bool operator()(const DataAccumulator<AccFlags::LEVEL>& a, const DataAccumulator<AccFlags::LEVEL>& b) const {
            return a.receive_ts < b.receive_ts; 
        }
    };

    template <>
    struct  __compare<AccFlags::TRADE> {
        bool operator()(const DataAccumulator<AccFlags::TRADE>& a, const DataAccumulator<AccFlags::TRADE>& b) const {
            return a.receive_ts < b.receive_ts; 
        }
    };

    template <AccFlags F>
    struct DataAccumulatorLess: __compare<F>
    {};
}

#endif