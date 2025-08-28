#include <cstdint>
#include <cstddef>
#include <sys/mman.h>
#include <stdexcept>
#pragma once

#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#define BUY_SIDE true
#define SELL_SIDE false

struct Order {
    unsigned long long id; 
    bool side; 
    uint32_t volume; 
    uint32_t price; 

    bool isTombStone() const noexcept {
        return id == 0;
    }

    void tombstone() noexcept {
        id = 0;
    }
};
#endif