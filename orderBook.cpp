#include <vector> 
#include "orderBook.h"

#define BUY_SIDE true
#define SELL_SIDE false

// struct Order {
//     uint32_t id; 
//     bool side; 
//     uint32_t volume; 
//     uint8_t price; 

//     bool isTombStone() const noexcept {
//         return id == 0;
//     }

//     static Order tombstone() noexcept {
//         return Order{0, BUY_SIDE, 0, 0};
//     }
// };

// class OrderBook {
//     int 
// public:

// };