#include <vector>
#include <iostream>
#include <cassert>
#include "orderBook.h"
#include "util.h"
#include "matchingEngine.h"
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <sys/_endian.h>

#include <libkern/OSByteOrder.h>

#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)

#include <netinet/in.h>  
#include <arpa/inet.h> 

#include <random>
#include <chrono>

constexpr int PORT = 9000;
constexpr int BACKLOG = 10;
constexpr size_t MSG_SIZE = 24;

constexpr int NUM_SHARDS = 4;

MatchingEngine* shards[NUM_SHARDS];

/*
Threads:
    - Matching engines (1 thread, 1 message queue per)
    - Thread pool for receiving orders and prepping them for matching engine (need message queue here)
    - Broadcasting thread 
    - 
    // Come back to networking stuff later 
*/

void runMatchingEngine(int shard) {
    MatchingEngine* matchingEngine = shards[shard];

    // Pop a message
    // Execute operation 
    while (true) {
        OrderMessage* message;
        while ((message = matchingEngine->messageQueue.pop()) == nullptr) {
            std::this_thread::yield(); // spin until not nullptr
        }
        
        switch (message->type) {
            case LIMIT:
                matchingEngine->limit(message->side, message->volume, message->price);
                break;
            case MARKET:
                matchingEngine->market(message->side, message->volume);
                break;
            case CANCEL:
                matchingEngine->cancel(message->orderId);
                break;
            case MODIFY:
                matchingEngine->modify(message->orderId, message->volume, message->price);
                break;
        }
    }
}

// Function run by thread pool for parsing orders 
// and placing in message queue for ME
/*
Protocol: 
First 3 bits is stock index (which matching engine)
Next 2 are for order type 
Next 1 is side
Next 32 are volume
Next 32 are price
Next 64 is order id
*/
OrderMessage* parseOrder(const uint8_t *buf, uint8_t* stockId) {
    OrderMessage* msg = new (orderMessageAllocator.getSlab()) OrderMessage();
    // First byte: 3 bits stock, 2 bits type, 1 bit side => 6 bits used
    uint8_t first = buf[0];
    *stockId = (first >> 5) & 0b111;      // top 3 bits
    msg->type = (first >> 3) & 0b11;        // next 2 bits
    msg->side = (first >> 2) & 0b1;               // next 1 bit

    // Copy aligned fields
    memcpy(&msg->volume, buf + 1, sizeof(uint32_t));
    memcpy(&msg->price, buf + 5, sizeof(uint32_t));
    memcpy(&msg->orderId, buf + 9, sizeof(uint64_t));

    msg->volume = ntohl(msg->volume);
    msg->price = ntohl(msg->price);
    msg->orderId = be64toh(msg->orderId); // big-endian to host

    return msg;
}

void handleClient(int client_fd) {
    uint8_t buf[MSG_SIZE];
    while (true) {
        ssize_t n = recv(client_fd, buf, MSG_SIZE, MSG_WAITALL);
        if (n <= 0) {
            break;  // client closed or error
        }

        uint8_t stockId;
        OrderMessage* msg = parseOrder(buf, &stockId);

        shards[stockId]->messageQueue.push(msg);

        // Figure out way to send id back when needed

        // // Example: reply with order_id only if order_type == 1
        // if (msg->type == 1) {
        //     uint64_t resp = htobe64(msg->orderId);
        //     send(client_fd, &resp, sizeof(resp), 0);
        // }
    }
    close(client_fd);
}


void acceptConnections() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("socket");
    }

    int opt = 1; 
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);


    if (bind(serverFd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("bind");
    }

    if (listen(serverFd, BACKLOG) < 0) {
        throw std::runtime_error("listen");
    }

    std::cout << "Server listening on port " << PORT << "\n";

    while (true) {
        int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd >= 0) {
            std::thread t(handleClient, clientFd);
            t.detach(); 
        }
    }

    close(serverFd);

}


///////////////////////
//////TESTING/////////
/////////////////////

// --------------------------------------------------
// Helper: build binary order message
// --------------------------------------------------
std::vector<uint8_t> buildMsg(uint8_t stockId, uint8_t type,
                              bool side, uint32_t volume,
                              uint32_t price, uint64_t orderId) {
    std::vector<uint8_t> buf(MSG_SIZE, 0);
    buf[0] = ((stockId & 0b111) << 5) |
             ((type & 0b11) << 3) |
             ((side & 0b1) << 2);

    uint32_t v = htonl(volume);
    uint32_t p = htonl(price);
    uint64_t id = htobe64(orderId);

    memcpy(buf.data() + 1, &v, 4);
    memcpy(buf.data() + 5, &p, 4);
    memcpy(buf.data() + 9, &id, 8);

    return buf;
}



// --------------------------------------------------
// Test client
// --------------------------------------------------
void clientTest(uint8_t stockId, uint8_t type, bool side,
                uint32_t volume, uint32_t price, uint64_t orderId) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return;
    }

    auto msg = buildMsg(stockId, type, side, volume, price, orderId);
    send(fd, msg.data(), msg.size(), 0);

    close(fd);
}


void speedTestOrderBook(size_t numOrders) {
    std::mt19937 rng(12345); // deterministic seed
    std::uniform_int_distribution<int> symDist(0, 3);     // 4 symbols
    std::uniform_int_distribution<int> typeDist(0, 3);    // LIMIT/MARKET/CANCEL/MODIFY
    std::uniform_int_distribution<int> sideDist(0, 1);    // buy/sell
    std::uniform_int_distribution<int> volDist(1, 1000);  // volumes
    std::normal_distribution<double> priceDist(1000, 5);  // clustered around 1000
    std::uniform_int_distribution<uint64_t> idDist(1, 1e9);

    std::vector<uint64_t> liveOrders; // track live IDs for cancel/modify
    liveOrders.reserve(numOrders);

    double overhead = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < numOrders; i++) {
        auto s = std::chrono::high_resolution_clock::now();
        uint8_t stockId = symDist(rng);
        uint8_t type    = typeDist(rng);
        bool side       = sideDist(rng);
        uint32_t vol    = volDist(rng);
        uint32_t price  = std::max(1, (int)priceDist(rng));
        uint64_t oid    = idDist(rng);

        // Ensure cancel/modify only happen if there are live orders
        if ((type == CANCEL || type == MODIFY) && liveOrders.empty()) {
            type = LIMIT; // fallback
        }

        if (type == CANCEL || type == MODIFY) {
            oid = liveOrders[rng() % liveOrders.size()];
        } else if (type == LIMIT) {
            liveOrders.push_back(oid);
        }

        // Build message
        auto buf = buildMsg(stockId, type, side, vol, price, oid);
        uint8_t stock;
        OrderMessage* msg = parseOrder(buf.data(), &stock);

        auto e = std::chrono::high_resolution_clock::now();
        overhead += std::chrono::duration<double>(e - s).count();

        // Push to engine queue
        shards[stock]->messageQueue.push(msg);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    double engineOnly = elapsed - overhead;

    double throughput = numOrders / engineOnly;
    std::cout << "Processed " << numOrders << " orders in "
              << engineOnly << " sec = " << throughput << " orders/sec\n";
}

int main() {
    // spawn all threads
    // matching engines first, then accept connections

    for (int i = 0; i < NUM_SHARDS; i++) {
        shards[i] = new MatchingEngine();
        std::thread t(runMatchingEngine, i);
        t.detach();
    }

    std::thread acceptThread(acceptConnections);

    // // -------- TESTS --------
    // std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // // Fire some test clients
    // std::thread c1(clientTest, 0, LIMIT, 1, 100, 50, 1111);
    // std::thread c2(clientTest, 1, MARKET, 0, 200, 0, 2222);
    // std::thread c3(clientTest, 2, CANCEL, 1, 0, 0, 3333);
    // std::thread c4(clientTest, 3, MODIFY, 0, 150, 60, 4444);

    // c1.join();
    // c2.join();
    // c3.join();
    // c4.join();

    // printf("here\n");
    // // let server process
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    // // -----------------------
    // printf("here\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // warmup

    speedTestOrderBook(500'000);

    acceptThread.join();
}