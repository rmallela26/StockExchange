#include <vector>
#include <iostream>
#include <cassert>
#include "orderBook.h"
#include "util.h"
#include "matchingEngine.h"
#include <thread>

/*
Threads:
    - Matching engines (1 thread, 1 message queue per)
    - Thread pool for receiving orders and prepping them for matching engine (need message queue here)
    - Broadcasting thread 
    - 
    // Come back to networking stuff later 
*/

void runMatchingEngine() {
    MatchingEngine matchingEngine; 

    // Pop a message
    // Execute operation 
    while (true) {
        OrderMessage* message;
        while (message = matchingEngine.messageQueue.pop());
        
        switch (message->type) {
            case LIMIT:

                break;
            case MARKET:

                break;
            case CANCEL:

                break;
            case MODIFY:
    
                break;
        }
    }
}

// Function run by thread pool for parsing orders 
// and placing in message queue for ME
void parseOrder() {

}

int main() {
    
}