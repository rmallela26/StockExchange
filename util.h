#include "orderBook.h"
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include <cstddef>
#include <stdexcept>
#include <cmath>

#ifndef UTIL_H
#define UTIL_H

#define CHUNK_SIZE 2

struct Chunk {
    int head = 0; // if head == tail, then it's empty
    int tail = 0; // tail always points to next insertion slot
    Chunk* next = nullptr;
    Order slots[CHUNK_SIZE];
};

class SlabAllocator {
public:
    void* region; 
    void* head;
    size_t slabSize;
    size_t totalSize;

    SlabAllocator(std::size_t bytes, size_t slab);
    void* getSlab();
    ~SlabAllocator();
};

extern SlabAllocator queueSlabAllocator;

class Queue {
public:
    Chunk* head;
    Chunk* tail;
    uint32_t priceLevel;

    Queue(uint32_t pl);
    Order* push(Order order);
    Order* pop();
    Order* peek();
};
#endif