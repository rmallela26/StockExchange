CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = 

TARGET = test_queue
OBJS = util.o main.o   # removed orderBook.o, since it has no implementations

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

util.o: util.cpp util.h orderBook.h
	$(CXX) $(CXXFLAGS) -c util.cpp

main.o: main.cpp util.h orderBook.h
	$(CXX) $(CXXFLAGS) -c main.cpp

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
