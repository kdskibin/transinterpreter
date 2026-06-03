CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
TARGET = translator
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/lexer.cpp $(SRCDIR)/parser.cpp \
          $(SRCDIR)/ops.cpp $(SRCDIR)/interpreter.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	python3 run_tests.py

.PHONY: all clean test