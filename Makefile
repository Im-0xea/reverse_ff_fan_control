# Copymiddle (CM) 2023 Xea. All wrongs rejected 
# see ./UNLICENSE file

CC = cc
CFLAGS = -Os -pipe
LDFLAGS = -flto -shared

all: recon proper

build:
	mkdir -p build

recon: build
	$(CC) -c -o build/ff_fan_control.o -O0 -Wno-unused-result ff_fan_control.c
	echo "float PID_fan[10];" | $(CC) -c -o build/ff_fan_control_stub_pid.o -O0 -Wno-unused-result -xc -
	$(CC) -o ff_fan_control_old -Wl,--build-id build/ff_fan_control.o build/ff_fan_control_stub_pid.o

proper: build
	$(CC) -o ff_fan_control_ng -DFF_NG $(LDFLAGS) $(CFLAGS) -Wno-unused-result ff_fan_control.c

clean:
	rm -rf ff_fan_control_old ff_fan_control_ng build
