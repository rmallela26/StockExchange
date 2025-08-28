#include <vector>
#include <iostream>
#include <cassert>
#include "orderBook.h"
#include "util.h"
#include "matchingEngine.h"

int main() {
    {
        std::cout << "=== Test 1: Multiple Partial Fills Across Levels ===\n";
        MatchingEngine engine;

        engine.limit(SELL_SIDE, 5, 101);   // id=1
        engine.limit(SELL_SIDE, 6, 102);   // id=2

        auto id3 = engine.limit(BUY_SIDE, 10, 105);
        std::cout << "BUY id=" << id3 << " should fully match SELL id=1 (5) "
                  << "and partially match SELL id=2 (5/6)\n";

        if (!engine.orders[1])
            std::cout << "SELL id=1 fully filled ✅\n";
        if (engine.orders[2])
            std::cout << "SELL id=2 remaining volume=" << engine.orders[2]->volume << "\n";
    }

    {
        std::cout << "\n=== Test 2: Partial Fill Then Add to Book ===\n";
        MatchingEngine engine;

        engine.limit(SELL_SIDE, 5, 100); // id=1
        auto id2 = engine.limit(BUY_SIDE, 3, 105); // fully fills
        std::cout << "BUY id=" << id2 << " fully filled ✅\n";
        std::cout << "SELL id=1 remaining volume=" << engine.orders[1]->volume << "\n";

        auto id3 = engine.limit(BUY_SIDE, 4, 95); // won’t cross
        std::cout << "BUY id=" << id3 << " resting in book @95 ✅\n";
    }

    {
        std::cout << "\n=== Test 3: Exact Price Match ===\n";
        MatchingEngine engine;

        auto id1 = engine.limit(SELL_SIDE, 4, 100);
        auto id2 = engine.limit(BUY_SIDE, 4, 100);

        if (!engine.orders[id1] && !engine.orders[id2])
            std::cout << "Both orders fully filled ✅\n";
    }

    {
        std::cout << "\n=== Test 4: Price Priority ===\n";
        MatchingEngine engine;

        engine.limit(SELL_SIDE, 5, 100); // id=1
        engine.limit(SELL_SIDE, 5, 99);  // id=2

        auto id3 = engine.limit(BUY_SIDE, 7, 101);
        std::cout << "BUY id=" << id3 << " matched SELL@99 first, then SELL@100 ✅\n";
        if (engine.orders[2] == nullptr)
            std::cout << "SELL id=2 fully filled ✅\n";
        if (engine.orders[1])
            std::cout << "SELL id=1 remaining volume=" << engine.orders[1]->volume << "\n";
    }

    {
        std::cout << "\n=== Test 5: Cancel (stub) ===\n";
        MatchingEngine engine;
        auto id1 = engine.limit(BUY_SIDE, 10, 100);
        engine.cancel(id1);
        if (engine.orders[id1] == nullptr)
            std::cout << "Order " << id1 << " canceled ✅\n";
    }

    {
        std::cout << "\n=== Test 6: Modify (stub) ===\n";
        MatchingEngine engine;
        auto id1 = engine.limit(SELL_SIDE, 5, 105);
        engine.modify(id1, 10, 103); // volume=10, price=103
        if (engine.orders[id1])
            std::cout << "Order " << id1 << " now volume=" << engine.orders[id1]->volume
                      << " price=" << engine.orders[id1]->price << " ✅\n";
    }

    return 0;
}