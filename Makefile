CC = gcc
CFLAGS = -O0 -Wno-unused-result
LDFLAGS =

all: reversed

reversed:
	$(CC) ff_fan_control.c -o ff_fan_control $(CFLAGS) $(LDFLAGS)

nextgen:
	@echo maybe soon
