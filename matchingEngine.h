#include <vector>
#include "util.h"

#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

const unsigned long long MAX_ORDERS = 1'000'000; // maximum number of add orders per shard 
const unsigned long long MAX_PRICE_LEVELS = 10'000;

class MatchingEngine {
    // std::vector<Order*> orders;
    // long long id = 1;
    // std::vector<Queue> buySide;
    // std::vector<Queue> sellSide;

public:
    /*
    Limit order - side, price, volume (returns id)
    Market order - side, price, volume (returns id)
    Cancel order - id
    Modify order - id, price, volume (assume side is the same) 
    */

    std::vector<Order*> orders;
    unsigned long long id = 1;
    std::vector<Queue> buySide;
    std::vector<Queue> sellSide;

    MatchingEngine();

    unsigned long long limit(bool side, uint32_t volume, uint32_t price);
    void market(bool side, uint32_t volume);
    void cancel(Order* ord);
    unsigned long long modify(Order* ord, uint32_t newVolume, uint32_t  newPrice);
};
#endif