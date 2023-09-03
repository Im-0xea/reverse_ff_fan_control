CC = aarch64-unknown-linux-gnu-gcc
CFLAGS = -O0 -Wno-unused-result
LDFLAGS = -Wl,--build-id

all: reversed

build:
	mkdir -p build

reversed: build
	$(CC) -c -o build/ff_fan_control.o $(CFLAGS) ff_fan_control.c
	$(CC) -c -o build/ff_fan_control_stub_pid.o $(CFLAGS) ff_fan_control_stub_pid.c
	$(CC) -o ff_fan_control $(LDFLAGS) build/ff_fan_control.o build/ff_fan_control_stub_pid.o

clean:
	rm ff_fan_control
