#include <limits>
#include <math.h>
#include <vector>
#include <iostream>
#include <cstddef>
#include "util.h"
#include "matchingEngine.h"

MatchingEngine::MatchingEngine() {
    orders.reserve(MAX_ORDERS);
    buySide.reserve(MAX_PRICE_LEVELS);
    sellSide.reserve(MAX_PRICE_LEVELS);
}

unsigned long long MatchingEngine::limit(bool side, uint32_t volume, uint32_t price) {
    if (id == MAX_ORDERS) {
        id = 1;
    }
    
    if (id < orders.size() && orders[id] != nullptr) {
        throw std::runtime_error("Max concurrent orders reached");
    }

    // modify later to pushing just paramters
    // to save time in creating object (since object
    // only gets copied over)
    Order order{id, side, volume, price};

    std::vector<Queue>& currSide = side == BUY_SIDE ? buySide : sellSide;
    std::vector<Queue>& otherSide = side == BUY_SIDE ? sellSide : buySide;
    auto cmp = (side == BUY_SIDE)
             ? [](uint32_t bid, uint32_t ask){ return bid >= ask; }
             : [](uint32_t ask, uint32_t bid){ return ask <= bid; };

    int ind = 0;
    int numLevels = otherSide.size();
    while (ind < numLevels && cmp(price, otherSide[ind].priceLevel) && volume > 0) {
        while (Order* o = otherSide[ind].peek()) {
            Order& ord = *o;
            uint32_t volumeTraded = std::min(volume, ord.volume);
            if (volumeTraded == ord.volume) {
                orders[ord.id] = nullptr;
                ord.tombstone();
                // broadcast result
                volume -= volumeTraded;
                if (volume == 0) {
                    // broadcast result
                    return id++;
                }
            } else {
                ord.volume -= volumeTraded;
                // broadcast result 
                return id++;
            }
        }
        ind++;
    }

    order.volume = volume; 

    // insert into current side
    ind = 0; 
    numLevels = currSide.size();
    while (ind < numLevels && !cmp(price, currSide[ind].priceLevel)) ind++;
    
    if (ind < numLevels && currSide[ind].priceLevel == price) {
        orders[id] = currSide[ind].push(order);
    } else {
        currSide.insert(currSide.begin() + ind, Queue(price));
        orders[id] = currSide[ind].push(order);
    }

    return id++;

}

void MatchingEngine::market(bool side, uint32_t volume) {

    std::vector<Queue>& currSide = side == BUY_SIDE ? buySide : sellSide;
    std::vector<Queue>& otherSide = side == BUY_SIDE ? sellSide : buySide;

    int ind = 0; 
    int numLevels = otherSide.size();
    while (ind < numLevels) {
        while (Order* o = otherSide[ind].peek()) {
            Order& ord = *o; 
            uint32_t volumeTraded = std::min(volume, ord.volume);
            if (volumeTraded == ord.volume) {
                orders[ord.id] = nullptr;
                ord.tombstone();
                // broadcast
                volume -= volumeTraded;
                if (volume == 0) {
                    id++;
                    return;
                }
            } else {
                ord.volume -= volumeTraded;
                // broadcast
                id++;
                return;
            }
        }
        ind++;
    }
    
    id++;
}

// Takes the address of the order to cancel
// Order address is found by some auxiliary thread and given to ME
void MatchingEngine::cancel(unsigned long long orderId) {
    Order& ord = *orders[orderId];
    orders[ord.id] = nullptr;
    ord.tombstone();
}

// returns the id of the new order 
unsigned long long MatchingEngine::modify(unsigned long long orderId, uint32_t newVolume, uint32_t newPrice) {
    // If only volume is different just change the volume 
    // If price is different, tombstone it and call limit (be careful with id's)
    Order& ord = *orders[orderId];
    if (newPrice == ord.price) {
        ord.volume = newVolume;
        return ord.id;
    } else {
        bool side = ord.side;
        orders[ord.id] = nullptr;
        ord.tombstone();
        return limit(side, newVolume, newPrice); // given new id 
    }
}