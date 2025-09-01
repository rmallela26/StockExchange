#include "orderBook.h"
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include <cstddef>
#include <stdexcept>
#include <cmath>
#include <atomic>

#ifndef UTIL_H
#define UTIL_H

#define CHUNK_SIZE 2
#define MESSAGE_QUEUE_SIZE 1024

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
extern SlabAllocator orderMessageAllocator;

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

#define LIMIT 0
#define MARKET 1
#define CANCEL 2
#define MODIFY 3

struct OrderMessage {
    uint8_t type; 
    uint32_t price;
    uint32_t volume;
    bool side;
    unsigned long long orderId;
};

struct Slot {
    OrderMessage* message; 
    std::atomic<size_t> seq;
};

// MPSC ring buffer that has spin waiting 
class MessageQueue {
public:
    Slot buffer[MESSAGE_QUEUE_SIZE];
    std::atomic<int> head; // pop from head
    std::atomic<int> tail; // push to tail 

    MessageQueue();
    void push(OrderMessage* m);
    OrderMessage* pop();
};
#endif