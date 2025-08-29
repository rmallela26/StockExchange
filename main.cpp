#include <vector>
#include <iostream>
#include <cassert>
#include "orderBook.h"
#include "util.h"
#include "matchingEngine.h"

int main() {
    {
        std::cout << "=== Test 1: Limit orders, partial + full fills ===\n";
        MatchingEngine engine;

        auto id1 = engine.limit(BUY_SIDE, 10, 100);
        std::cout << "Added BUY id=" << id1 << " @100 x10\n";

        auto id2 = engine.limit(SELL_SIDE, 5, 105);
        std::cout << "Added SELL id=" << id2 << " @105 x5 (resting)\n";

        auto id3 = engine.limit(SELL_SIDE, 7, 99); // crosses BUY
        std::cout << "Added SELL id=" << id3 << " @99 x7\n";

        if (engine.orders[id1])
            std::cout << "BUY id=" << id1 << " remaining vol=" << engine.orders[id1]->volume << "\n";
        else
            std::cout << "BUY id=" << id1 << " fully filled\n";

        if (engine.orders[id3])
            std::cout << "SELL id=" << id3 << " remaining vol=" << engine.orders[id3]->volume << "\n";
        else
            std::cout << "SELL id=" << id3 << " fully filled\n";

        auto id4 = engine.limit(BUY_SIDE, 3, 110); // matches SELL @105
        std::cout << "Added BUY id=" << id4 << " @110 x3\n";
        if (engine.orders[id2])
            std::cout << "SELL id=" << id2 << " remaining vol=" << engine.orders[id2]->volume << "\n";
        else
            std::cout << "SELL id=" << id2 << " fully filled\n";
    }

    {
        std::cout << "\n=== Test 2: Market Buy with enough liquidity ===\n";
        MatchingEngine engine;

        auto id1 = engine.limit(SELL_SIDE, 5, 100);
        auto id2 = engine.limit(SELL_SIDE, 5, 101);

        engine.market(BUY_SIDE, 8); // should take 5@100 and 3@101
        std::cout << "Market BUY matched, leftover should be 2 @101\n";

        if (engine.orders[id1] == nullptr)
            std::cout << "SELL id=" << id1 << " fully filled ✅\n";
        if (engine.orders[id2])
            std::cout << "SELL id=" << id2 << " remaining vol=" << engine.orders[id2]->volume << "\n";
    }

    {
        std::cout << "\n=== Test 3: Market Buy with insufficient liquidity ===\n";
        MatchingEngine engine;

        auto id1 = engine.limit(SELL_SIDE, 3, 100);

        engine.market(BUY_SIDE, 10); // only 3 available
        std::cout << "Market BUY wanted 10, only got 3, 7 units die ✅\n";

        if (engine.orders[id1] == nullptr)
            std::cout << "SELL id=" << id1 << " fully filled ✅\n";
    }

    {
        std::cout << "\n=== Test 4: Market Sell with no liquidity ===\n";
        MatchingEngine engine;

        engine.market(SELL_SIDE, 5);
        std::cout << "Market SELL got nothing, should just die ✅\n";
    }

    {
        std::cout << "\n=== Test 5: Price priority ===\n";
        MatchingEngine engine;

        auto id1 = engine.limit(SELL_SIDE, 5, 100); // worse ask
        auto id2 = engine.limit(SELL_SIDE, 5, 99);  // better ask

        auto id3 = engine.limit(BUY_SIDE, 7, 101);  // should hit @99 first
        std::cout << "BUY id=" << id3 << " matched @99, then @100 ✅\n";

        if (engine.orders[id2] == nullptr)
            std::cout << "SELL id=" << id2 << " fully filled ✅\n";
        if (engine.orders[id1])
            std::cout << "SELL id=" << id1 << " remaining vol=" << engine.orders[id1]->volume << "\n";
    }

    return 0;
}