#include <vector>
#include "util.h"

class MatchingEngine {
    std::vector<Order*> orders{10000};
    long long id = 1;
    std::vector<Queue> priceLevels{100};

public:
    /*
    Limit order - side, price, volume (returns id)
    Market order - side, price, volume (returns id)
    Cancel order - id
    Modify order - id, price, volume (assume side is the same) 
    */

    unsigned long long limit(bool side, uint32_t volume, uint8_t price);
    unsigned long long market(bool side, uint32_t volume, uint8_t price);
    void cancel(unsigned long long id);
    void modify(unsigned long long id, uint32_t newVolume, uint8_t newPrice);
};