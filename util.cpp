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
    totalSize(bytes), slabSize(slab) {
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
SlabAllocator orderMessageAllocator(1000000ULL, sizeof(OrderMessage)); // 1 MB



Queue::Queue(uint32_t pl) {
    head = new (queueSlabAllocator.getSlab()) Chunk();
    tail = head;
    priceLevel = pl;
}

// returns the address where the element was pushed
Order* Queue::push(Order order) {
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
        return &tail->slots[0];
    } else {
        tail->slots[t] = order;
        tail->tail = next;
        return &tail->slots[t];
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

Order* Queue::peek() {
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
    return &o[h];
}

MessageQueue::MessageQueue() { // constructor call
    head.store(0);
    tail.store(0);
    for (int i = 0; i < MESSAGE_QUEUE_SIZE; i++) {
        buffer[i].seq.store(i, std::memory_order_relaxed);
    }
}

void MessageQueue::push(OrderMessage* m) {
    size_t pos = tail.fetch_add(1, std::memory_order_relaxed); 
    Slot& slot = buffer[pos % MESSAGE_QUEUE_SIZE];
    
    while(slot.seq.load(std::memory_order_acquire) != pos);

    slot.message = m; 
    slot.seq.store(pos + 1, std::memory_order_release);
}

OrderMessage* MessageQueue::pop() {
    size_t pos = head.load(std::memory_order_relaxed);
    Slot& slot = buffer[pos % MESSAGE_QUEUE_SIZE];

    size_t expected = pos + 1;
    if (slot.seq.load(std::memory_order_acquire) != expected) return nullptr; 

    OrderMessage* ord = slot.message; 
    slot.seq.store(pos + MESSAGE_QUEUE_SIZE, std::memory_order_release);
    head.store(pos + 1, std::memory_order_relaxed);
    return ord; 
}