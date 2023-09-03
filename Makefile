CC = aarch64-unknown-linux-gnu-gcc
CFLAGS = -O0 -Wno-unused-result
LDFLAGS = -Wl,--build-id

all: recon proper

build:
	mkdir -p build

recon: build
	$(CC) -c -o build/ff_fan_control.o $(CFLAGS) ff_fan_control.c
	$(CC) -c -o build/ff_fan_control_stub_pid.o $(CFLAGS) ff_fan_control_stub_pid.c
	$(CC) -o ff_fan_control_old $(LDFLAGS) build/ff_fan_control.o build/ff_fan_control_stub_pid.o

proper: build
	$(CC) -o ff_fan_control_ng -DFF_NG $(LDFLAGS) $(CFLAGS) ff_fan_control.c

clean:
	rm ff_fan_control
