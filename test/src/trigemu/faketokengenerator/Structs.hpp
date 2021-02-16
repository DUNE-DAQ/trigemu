/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains struct and other type definitions for shema in 
 * namespace dunedaq::trigemu::faketokengenerator.
 */
#ifndef DUNEDAQ_TRIGEMU_FAKETOKENGENERATOR_STRUCTS_HPP
#define DUNEDAQ_TRIGEMU_FAKETOKENGENERATOR_STRUCTS_HPP

#include <cstdint>


namespace dunedaq::trigemu::faketokengenerator {

    // @brief 
    using interval_ms = int32_t;


    // @brief 
    using sigma_ms = int32_t;


    // @brief FakeTokenGenerator conf parameters
    struct ConfParams {

        // @brief Interval between token messages in ms
        interval_ms token_interval_ms = 1000;

        // @brief Variance of interval between token messages
        sigma_ms token_sigma_ms = 0;
    };

} // namespace dunedaq::trigemu::faketokengenerator

#endif // DUNEDAQ_TRIGEMU_FAKETOKENGENERATOR_STRUCTS_HPP