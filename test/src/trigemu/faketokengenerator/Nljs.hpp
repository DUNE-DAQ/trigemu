/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains functions struct and other type definitions for shema in 
 * namespace dunedaq::trigemu::faketokengenerator to be serialized via nlohmann::json.
 */
#ifndef DUNEDAQ_TRIGEMU_FAKETOKENGENERATOR_NLJS_HPP
#define DUNEDAQ_TRIGEMU_FAKETOKENGENERATOR_NLJS_HPP

// My structs
#include "trigemu/faketokengenerator/Structs.hpp"


#include <nlohmann/json.hpp>

namespace dunedaq::trigemu::faketokengenerator {

    using data_t = nlohmann::json;
    
    inline void to_json(data_t& j, const ConfParams& obj) {
        j["token_interval_ms"] = obj.token_interval_ms;
        j["token_sigma_ms"] = obj.token_sigma_ms;
    }
    
    inline void from_json(const data_t& j, ConfParams& obj) {
        if (j.contains("token_interval_ms"))
            j.at("token_interval_ms").get_to(obj.token_interval_ms);    
        if (j.contains("token_sigma_ms"))
            j.at("token_sigma_ms").get_to(obj.token_sigma_ms);    
    }
    
} // namespace dunedaq::trigemu::faketokengenerator

#endif // DUNEDAQ_TRIGEMU_FAKETOKENGENERATOR_NLJS_HPP