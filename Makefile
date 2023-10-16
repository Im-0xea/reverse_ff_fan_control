# Copymiddle (CM) 2023 Xea. All wrongs rejected 
# see ./UNLICENSE file

CC = cc
CFLAGS = -Os -pipe
LDFLAGS = -flto -shared

OLDCFLAGS = -O0 -Wno-unused-result
OLDLDFLAGS = -Wl,--build-id

all: recon proper

recon:
	mkdir -p build
	$(CC) $(OLDCFLAGS) -c -o build/ff_fan_control.o ff_fan_control.c
	echo "float PID_fan[10];" | $(CC) $(OLDCFLAGS) -c -o build/ff_fan_control_stub_pid.o -xc -
	$(CC) $(OLDLDFLAGS) -o ff_fan_control_old build/ff_fan_control.o build/ff_fan_control_stub_pid.o

proper: build
	$(CC) $(CFLAGS) $(LDFLAGS) -o ff_fan_control_ng -DFF_NG ff_fan_control.c

clean:
	rm -rf ff_fan_control_old ff_fan_control_ng build
