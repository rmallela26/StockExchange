#include <vector>
#include <iostream>
#include <cassert>
#include "orderBook.h"
#include "util.h"

int main() {
    Queue q;

    // --- Test 1: basic push/pop ---
    Order a{static_cast<uint32_t>(111), BUY_SIDE,
            static_cast<uint32_t>(100), static_cast<uint8_t>(5)};
    Order b{static_cast<uint32_t>(222), SELL_SIDE,
            static_cast<uint32_t>(200), static_cast<uint8_t>(51)};

    q.push(a);
    q.push(b);

    Order* o1 = q.pop();
        std::cout << "hello " << o1->id << " " << o1->side << " " << o1->volume << " " << static_cast<int>(o1->price) << std::endl;
    assert(o1 && o1->id == a.id && o1->side == a.side &&
           o1->volume == a.volume && o1->price == a.price);
    std::cout << "Pop a OK\n";

    Order* o2 = q.pop();
    if (o2)
        std::cout << "hello " << o2->id << " " << o2->side << " " << o2->volume << " " << static_cast<int>(o2->price) << std::endl;
    assert(o2 && o2->id == b.id && o2->side == b.side &&
           o2->volume == b.volume && o2->price == b.price);
    std::cout << "Pop b OK\n";

    assert(q.pop() == nullptr);
    std::cout << "Empty pop OK\n";

    // --- Test 2: rollover ---
    Order c{static_cast<uint32_t>(333), BUY_SIDE,
            static_cast<uint32_t>(10), static_cast<uint8_t>(42)};
    Order d{static_cast<uint32_t>(444), SELL_SIDE,
            static_cast<uint32_t>(20), static_cast<uint8_t>(43)};
    Order e{static_cast<uint32_t>(555), BUY_SIDE,
            static_cast<uint32_t>(30), static_cast<uint8_t>(44)};

    q.push(c);
    q.push(d);
    q.push(e); // rollover

    Order* o3 = q.pop();
    assert(o3 && o3->id == c.id);
    Order* o4 = q.pop();
    assert(o4 && o4->id == d.id);
    Order* o5 = q.pop();
    assert(o5 && o5->id == e.id);
    std::cout << "Rollover OK\n";

    // --- Test 3: tombstone skip ---
    Order f{static_cast<uint32_t>(666), BUY_SIDE,
            static_cast<uint32_t>(77), static_cast<uint8_t>(55)};
    Order g{static_cast<uint32_t>(777), SELL_SIDE,
            static_cast<uint32_t>(88), static_cast<uint8_t>(56)};

    q.push(f);
    q.push(g);

    // mark first as tombstone
    q.head->slots[q.head->head] = Order::tombstone();

    Order* o6 = q.pop();
    if (o6) {
        std::cout << o6->id << std::endl;
    }
    assert(o6 && o6->id == g.id);
    assert(q.pop() == nullptr);
    std::cout << "Tombstone skip OK\n";

    // --- Test 4: multiple rollovers ---
    Order arr[10];
    for (int i = 0; i < 10; i++) {
        arr[i] = {
            static_cast<uint32_t>(1000 + i),
            BUY_SIDE,
            static_cast<uint32_t>(10 + i),
            static_cast<uint8_t>(50 + i)
        };
        q.push(arr[i]);
    }
    for (int i = 0; i < 10; i++) {
        Order* o = q.pop();
        assert(o && o->id == arr[i].id &&
               o->volume == arr[i].volume &&
               o->price == arr[i].price);
    }
    assert(q.pop() == nullptr);
    std::cout << "Multiple rollovers OK\n";

    std::cout << "All tests passed!\n";
    return 0;
}