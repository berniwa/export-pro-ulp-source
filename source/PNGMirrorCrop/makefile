CC=g++

ifdef OS
	CFLAGS=-c -Wall -I./libpng/include/ -I./libzlib/include -O3
	LDFLAGS= -L./libpng/lib/ -L./libzlib/lib -O3
	LIBS= -lpng -lzlibstatic -static
	EXECUTABLE=PNGMirrorCrop.exe
else
	CFLAGS=-c -Wall
	LDFLAGS= -L/usr/lib/	
	LIBS= -lpng -lz 
	EXECUTABLE=LibPNGImage
endif

SOURCES=main.cpp LibPNGImage.cpp
OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC)  $(LDFLAGS)  $(OBJECTS) -o $@ $(LIBS)
	rm -rf *o

.cpp.o:
	$(CC) $(CFLAGS) $(LIBS) $< -o $@

clean: 
	rm -rf *o $(EXECUTABLE)