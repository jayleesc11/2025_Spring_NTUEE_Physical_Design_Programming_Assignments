CC=g++
LDFLAGS=-std=c++11 -O3 -lm
SOURCES=src/floorplanner.cpp src/tm_usage.cpp src/config.cpp src/main.cpp
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=bin/fp
INCLUDES=src/floorplanner.h src/module.h src/tm_usage.h

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c ${INCLUDES}
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)
