DEBUG = 1

CC = gcc
EXEC = ms2rms
COMMON = -I./libmseed/ -I.
CFLAGS =  -Wall
LDFLAGS = -L./libmseed
LDLIBS = -lmseed -lm

OBJS = main.o standard_deviation.o

ifeq ($(DEBUG), 1)
CFLAGS += -O0 -g -DDEBUG=1
endif

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(COMMON) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(COMMON) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(EXEC)
