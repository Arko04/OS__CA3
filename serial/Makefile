# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11
LDFLAGS = -lsndfile -lpthread

# Source and executable
SRC = main.cpp
TARGET = main.out

# Default target
all: $(TARGET) run

# Compilation rule
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# Run the program
run: $(TARGET)
	./$(TARGET) ../input.wav

# Clean up
clean:
	rm -f $(TARGET)
