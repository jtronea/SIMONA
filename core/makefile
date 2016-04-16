.PHONY: all clean user-clean
CXX=g++
CFLAGS=-g -Wall -lpthread

BINARY = $(patsubst %.c,%.o,$(wildcard *.c))
TARGET = simona 

all: $(TARGET)

$(TARGET): user-clean $(BINARY)
	$(CXX) $(BINARY)  -o $@ $(INCLUDE) $(CFLAGS) $(LIBS)
%.o:%.c
	$(CXX) -c $< -o $@ $(INCLUDE) $(CFLAGS)

clean:
	@rm -rf ${TARGET} $(BINARY) 2>/dev/null
