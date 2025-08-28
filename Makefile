CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = 

TARGET = main
OBJS = util.o main.o matchingEngine.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

util.o: util.cpp util.h orderBook.h
	$(CXX) $(CXXFLAGS) -c util.cpp

matchingEngine.o: util.cpp util.h orderBook.h

main.o: main.cpp util.h orderBook.h matchingEngine.h
	$(CXX) $(CXXFLAGS) -c main.cpp

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
