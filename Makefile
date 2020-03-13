DEBUG = 1

CC = gcc
EXEC = ms2rms
COMMON = -I./libmseed/ -I.
CFLAGS =  -Wall
LDFLAGS = -L./libmseed -Wl,-rpath,./libmseed
LDLIBS = -Wl,-Bstatic -lmseed -Wl,-Bdynamic -lm

OBJS = main.o standard_deviation.o min_max.o traverse.o

ifeq ($(DEBUG), 1)
CFLAGS += -O0 -g -DDEBUG=1
endif

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(MAKE) -C libmseed/ static
	$(CC) $(COMMON) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(COMMON) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C libmseed/ clean
	rm -rf $(OBJS) $(EXEC)
