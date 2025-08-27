#include <vector>
#include <iostream>
#include <cstddef>
#include "util.h"
#include "matchingEngine.h"

const unsigned long long MAX_CONCURRENT_ORDERS = 10000000; // ten million 

unsigned long long MatchingEngine::limit(bool side, uint32_t volume, uint8_t price) {
    Order order{id, side, volume, price};
    id++;

}

unsigned long long MatchingEngine::market(bool side, uint32_t volume, uint8_t price) {

}

void MatchingEngine::cancel(unsigned long long id) {

}

void MatchingEngine::modify(unsigned long long id, uint32_t newVolume, uint8_t newPrice) {

}