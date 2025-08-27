#include <cstdint>
#include <cstddef>
#include <sys/mman.h>
#include <stdexcept>
#pragma once

#ifndef BUY_SIDE
#define BUY_SIDE true
#define SELL_SIDE false
#endif

struct Order {
    unsigned long long id; 
    bool side; 
    uint32_t volume; 
    uint8_t price; 

    bool isTombStone() const noexcept {
        return id == 0;
    }

    static Order tombstone() noexcept {
        return Order{0, BUY_SIDE, 0, 0};
    }
};