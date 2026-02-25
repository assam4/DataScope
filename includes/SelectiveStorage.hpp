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
        ///< Combination for order book level data
        LEVEL       = RECEIVE_TS | EXCHANGE_TS | PRICE | QUANTITY | SIDE,
        ///< Combination for order book trade data
        TRADE       = RECEIVE_TS | EXCHANGE_TS | PRICE | QUANTITY | SIDE | REBUILD
    };

    /**
    *   @brief Empty placeholder for unused fields
    *   
    *   Used with Empty Base Optimization to reduce structure size
    *    when flags are not set.
     */
    struct  __emptyField;

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
    struct  DataAccumulator: std::conditional_t<(F & AccFlags::RECEIVE_TS), struct { uint64_t  receive_ts;}, __emptyField>,
                            std::conditional_t<(F & AccFlags::EXCHANGE_TS), struct { uint64_t  exchange_ts;}, __emptyField>,
                            std::conditional_t<(F & AccFlags::PRICE), struct { double  price;}, __emptyField>,
                            std::conditional_t<(F & AccFlags::QUANTITY), struct { double   price;}, __emptyField>,
                            std::conditional_t<(F & AccFlags::SIDE), struct { bool side;}, __emptyField>,
                            std::conditional_t<(F & AccFlags::REBUILD), struct { bool rebuild;}, __emptyField>
    {};
}

#endif
