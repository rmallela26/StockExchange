#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include <cstddef>
#include <stdexcept>
#include <cmath>
#include <iostream>

#include "orderBook.h"
#include "util.h"

SlabAllocator::SlabAllocator(std::size_t bytes, size_t slab) :
    slabSize(slab),  totalSize(bytes) {
    region = mmap(nullptr, bytes,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0);

    if (region == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    head = region;
}

void* SlabAllocator::getSlab() {
    void *ptr = head; 
    if (static_cast<std::byte*>(head) + slabSize > static_cast<std::byte*>(region) + totalSize) {
        throw std::runtime_error("SlabAllocator out of memory");
    } else 
        head = static_cast<std::byte*>(head) + slabSize;
    return ptr;
}

SlabAllocator::~SlabAllocator() {
    munmap(region, totalSize);
}

SlabAllocator queueSlabAllocator(128 * 1000000ULL, sizeof(Chunk));  // 128 MB



Queue::Queue() {
    head = new (queueSlabAllocator.getSlab()) Chunk();
    tail = head;
}

void Queue::push(Order order) {
    int h = tail->head;
    int t = tail->tail;

    int next = (t + 1) % CHUNK_SIZE;
    if (next == h) {
        // full
        Chunk* nextChunk = new (queueSlabAllocator.getSlab()) Chunk();
        tail->next = nextChunk;
        tail = tail->next;
        tail->slots[0] = order;
        tail->tail = 1;
    } else {
        tail->slots[t] = order;
        tail->tail = next;
    }
}

Order* Queue::pop() {
    int h = head->head;
    int t = head->tail; 
    
    if (h == t) {
        if (head->next) {
            head = head->next;
            h = head->head;
            t = head->tail;
        } else {
            return NULL;
        }
    }
    Order* o = head->slots;
    while (o[h].isTombStone()) {
        h = (h + 1) % CHUNK_SIZE;
        if (h == t) {
            if (head->next) {
                head = head->next;
                h = head->head;
                t = head->tail;
                o = head->slots;
            } else {
                return NULL;
            }
        } 
    }
    Order* order = &o[h];
    h = (h + 1) % CHUNK_SIZE;
    head->head = h;
    return order;
}