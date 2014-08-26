CC=g++
CFLAGS=-c -Wall
LDFLAGS= -L/usr/lib/
SOURCES=main.cpp LibPNGImage.cpp
LIBS= -lpng -lz 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=LibPNGImage

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC)  $(LDFLAGS)  $(OBJECTS) -o $@ $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) $(LIBS) $< -o $@

clean: 
	rm -rf *o $(EXECUTABLE)
