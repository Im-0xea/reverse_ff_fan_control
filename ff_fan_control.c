/*  +----------------+
 *  |       #        |
 *  |        #       |
 *  |        #       |
 *  |       ##       |
 *  |  ########    ##|
 *  |##    ########  |
 *  |       ##       |
 *  |       #        |
 *  |       #        |
 *  |        #       |
 *  +----------------+
 *  | ff_fan_control |
 *  +----------------+
 *       reversed
 *  firefly_fan_control
 *
 * "kill proprietary software"
 * - Me (Xea)
 *
 *
 * static bin anal by : Xea
 *
 * proprietary bin by : firefly "open source" team
 *
 */

// comment to disable fixes
#define FF_NG

// headers ---------------------------------------------------------------------
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <pthread.h>
#include <unistd.h>
// -----------------------------------------------------------------------------

// top level declarations ------------------------------------------------------
enum {
	CS_R1_3399JD4 = 0,
	CS_R2_3399JD4 = 1,
	ROC_RK3588S_PC = 2,
	ITX_3588J = 3,
	ROC_RK3588_PC = 4
} board;

extern float PID_fan[10];
void (*PID_fan_func)(int);
char PID_debug_buff[1024]; // unused till now
int ROC_RK3588S_PC_VERSION;
uint32_t uart_head = 2863267968; // 0x8000aaaa
int fan_switch = 1; // 0x01000000
int global_pwm = 50; // 0x32000000

int global_debug; // unused till now
int uart_end = 8432298; // 0xaaaa8000 // unused till now
int uart_cmd = 838860800; // 0x320000000 // unused till now
char global_fan_speed[40]; // usused till now
int global_temperature;
int start = 1; // 0x01000000
int debug_buff_count; // unused till now
char firefly_fan[72];
// -----------------------------------------------------------------------------

// local static ----------------------------------------------------------------
bool completed; // unused till now
// __func__ might be needed
int temperature; // unused till now
int count; // unused till now
int tmp; // unused till now
// -----------------------------------------------------------------------------

// utility functions -----------------------------------------------------------
void init_time() // done
{
	struct itimerval itv = {
		.it_interval = {
			.tv_sec = 0, // sp+0x30
			.tv_usec = PID_fan[32 / 4] // sp+0x28
		},
		.it_value = {
			.tv_sec = 5, // sp+0x20
			.tv_usec = 0 // sp+0x18
		}
	};
	setitimer(ITIMER_REAL, &itv, 0);
}

void init_sigaction() // done
{
	struct sigaction sa;
	sa.sa_handler = PID_fan_func; // sp+0x10
	sa.sa_restorer = NULL; // sp+0x98
	sigemptyset(&sa.sa_mask); // add 0x10 than sp+0x18
	sigaction(SIGALRM, &sa, NULL);
	// might want an error check here
}

int uart_set(int fd, int x1, int x2, int x3, int x4)
{
	struct termios tio;
	if (tcgetattr(fd, &tio)) {
		perror("Setupserial 1");
		return -1;
	}
	void * h38 = NULL;
	void * h10 = NULL;
	void * h20 = NULL;
	long h0 = 0;
	int h2  = 0;
	int h40; // uninitilized
	h40 |= 0x880;
	h40 &= 0xffffffcf;
	if (x2 == 7) {
		h40 |= 0x20;
	} else if (x2 == 8) {
		h40 |= 0x30;
	}
	if (x3 /* something something */); // you are here


	tcflush(fd, 0);
	if (tcsetattr(fd, 0, &tio)) {
		perror("com set error");
		return -1;
	}
	return 0;
}

void fan_alarm(char *fan) // done
{
	printf("%s: speed error\n", fan + 16);
	fan[10] |= 1; // & 0xff - only needed for char
	fan[10] |= 2; // & 0xff - only needed for char
	// I don't get it, so I can't complain
}

int get_temperature(char *path, long something) // done - unused
{
	const int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("%s:open error!\n", path);
		fprintf(stderr, "uart_open %s error\n", path);
		// failed to adjust error message copy pasted from uart_open
		perror("open:");
		return -3;
	}
	char buf[20]; // presumeably 4 bytes padding
	int ret;
	if (read(fd, buf, 20) == 0) {
		printf("read error: %s\n", path);
		// return uninitilized
	} else {
		ret = atoi(buf);
		printf("read temperature: %d\n", ret);
	}
	close(fd);
	return ret;
}

int sys_uart_close(const int fd) // done - unused
{
	close(fd);
	return 0;
	// is this really too complicated to remember?
	// also you return 0 even on failure
}

int sys_uart_read(int fd, char *buf, int nbytes, int it) // done
{
	fd_set read_fds;
	int rbytes = 0;
	// char *bufp = buf
	// bogus pointer to buf, I just presume this is a compiler op

	do {
		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);
		// you are setting up a completly new fd_set every iteration

		struct timeval tv = {
			//.tv_sec =
			// ((0x10624dd3 * it) >> 0x26) -  (it >> 0x1f),
			.tv_sec = it / 1000,
			//.tv_usec = 
			// (it - ((((0x10624dd3 * it) >> 0x26) -
			// (it >> 0x1f)) * 0x3e8)) * 0x3e8
			.tv_usec = it * 100
		};

		int ret = select(fd + 1, &read_fds, NULL, NULL, &tv);
		if (ret < 0) {
			perror("sys_uart_read:select");
			break;
		}
		if (!ret)
			break;
		ret = read(fd, buf + rbytes, nbytes - rbytes);
		if (ret < 0) {
			if (errno != EINTR) {
				perror("sys_uart_write:read");
				break;
			}
			ret = 0;
		} else if (ret == 0) {
			tcflush(fd, TCIFLUSH);
			return -3;
		}
		rbytes += ret;
	} while (rbytes != nbytes);
	tcflush(fd, TCIFLUSH);

	return rbytes;
}

int sys_uart_write(int fd, char *buf, size_t bytes) // done
                 //long x3 /* unused */)
{
	size_t nbytes = bytes;
	while (nbytes != 0) {
		long wbytes = write(fd, buf, nbytes);
		if (wbytes < 0) {
			if (errno != EINTR) {
				perror("sys_uart_write:write");
				return -3;
			}
			wbytes = 0;
		}
		nbytes -= wbytes;
		buf += wbytes;
	} while (1);
	return bytes - nbytes;
}

int init_uart(const char *tty_path) // done
{
	int fd = open(
	    tty_path, O_RDWR & AT_SYMLINK_NOFOLLOW); // 0x102
	if (fd < 0) {
		printf("%s:open error!\n", tty_path);
		fprintf(stderr, "uart_open %s error\n", tty_path);
		perror("open:");
		// three error messages...
		return -3;
	}
	if (1 > uart_set(fd, 1, 8, 0, 1)) {
		printf("%s:set error", tty_path);
		// missing \n
		fwrite("uart set failed!\n", 1, 17, stderr);
		// fputs would be a bit more elagant
		// also two error messages
		return -3;
	}
	return fd;
}

void send_fan_cmd(char *cmd) // done
{
	char *format = cmd;
	long h0 = (long) format[12];
	// seams like you forgot to use it
	if (sys_uart_write(format[12], format,
	           4 /*, 0x2710 unused */) < 0)
		printf("%s: error\n", format + 16);
	if (sys_uart_write(format[12], format + 8
	             , 4 /*, 0x2710 unused */) < 0)
		printf("%s: error\n", format + 16);
	if (sys_uart_write(format[12], format + 4,
	               4 /*, 0x2710 unused */) < 0)
		printf("%s: error\n", format + 16);
}
// -----------------------------------------------------------------------------

// ROC_RK3588S_PC functions ----------------------------------------------------
int get_ROC_RK3588S_PC_version() // done
{
	FILE * iv = popen(
	    "cat /sys/bus/iio/devices/iio:device0/in_voltage5_raw", "r");
	// the proper C way would be using open() and read()
	if (!iv) {
		puts(
		  "can not open /sys/bus/iio:device0/in_voltage5_raw file");
		fclose(iv);
		// should be pclose()
		return -1;
	}
	char volt_str[1000] = "";
	fgets(volt_str, 1000, iv);
	// this read should not return more than 64 bytes
	const size_t volt_len = strlen(volt_str);
	volt_str[volt_len - 1] = '\0';
	const int volt = (int) atof(volt_str);
	fclose(iv);
	// should be pclose()
	if (volt >= 0) {
		if (volt <= 100)
			return 0;
		if (volt >= 1949 && volt <= 2150)
			return 1; 
	}
	return -1;
}

void fan_ROC_RK3588S_PC_init() // done
{
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", "r");
	// if you don't want to do this with open() and write()
	// but incist on this, you should atleast use system()
	// popen() just leaked a fd
	// also you should probably check
	// if your write was actually successful
}

#define RK3588_PWM "/sys/class/hwmon/hwmon1/pwm1"
void set_ROC_RK3588S_PC_fan_pwm(uint8_t pwm) // done
{
	int rpwm = 0;
	char str[12] = "\0";
	int unused = 0;
	// while radare saw it as the 16th place in str being zeroed
	// I think its more likely that this is a var
	// they never used and therefore was interpreted as str + 16
	switch (ROC_RK3588S_PC_VERSION) {
	case 0:
		rpwm = pwm * (float) ((1 / 3) + 2);
		break;
	case 1:
		rpwm = pwm * (float) ((1 / 3) + 2);
		break;
	default:
		break;
	}
	// congratulations, your switch has a duplicate case
	// aaaand will not set rpwm if the version is unknown
	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	int fd = open(RK3588_PWM, O_RDWR & 0x900); // 0x902
	if (fd <= 0) {
		printf(
		  "set_ROC_RK3588S_PC_fan_pwm: Can not open %s file\n",
		  RK3588_PWM);
		// at this point you should be returning
		// instead you are writting and closing a invalid fd
	}
	sprintf(str, "%d", rpwm);
	int res = write(fd, str, strlen(str));
	// this variable seams to stay unused
	// also good practice(and compiler warnings)
	// would demand another fail check here
	// especially because this is a write which is likely to fail
	close(fd);
}

float roc_rk3588s_pc_average_temperature() // done
{
	float s[73];
	memset(s, 0, 73 * sizeof(float));
	float ret = 0;
	FILE * stream = popen(
	    "cat /sys/class/thermal/thermal_zone*/temp", "r");
	if (!stream) {
		puts("no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f; // 0x4248000 in IEEE-754
		// we don't have a read
		// either through a misconfigured kernel or handware issues
		// just returning 50 risks frying your board
	}
	int count = 0;
	char str[1000];
	// this read should not return more than 64 bytes
	while (fgets(str, 1000, stream)) {
		int temp_l = strlen(str);
		str[temp_l - 1] = 0;
		s[count] = atof(str);
		printf("%f\n", s[count]);
		s[count] /= 1000.0f; // 0x44fa0000
		ret += s[count];
		++count;
	}
	if (count > 1) {
		ret -= s[count];
		ret /= (float) count;
	}
	printf("sum = %f\n", ret);
	fclose(stream);
	// should be pclose()
	return ret;
}

void *roc_rk3588s_pc_fan_thread_daemon(void *arg) // done
{
	int x = 0;
	do {
		do {
			usleep(50000);
			// seams more practical to sleep 250000 ms once
		} while (++x != 4);
		x = 0;
		global_temperature =
		    roc_rk3588s_pc_average_temperature() * 1000.0f;
		// 0x447a0000 in IEEE-754
	} while (1);
}
// -----------------------------------------------------------------------------

// ROC_RK3588_PC functions -----------------------------------------------------
void fan_ROC_RK3588_PC_init() // done
{
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", "r");
	// see comments on RK3588S init
}

float roc_rk3588_pc_average_temperature() // done
{
	float s[73];
	memset(s, 0, 73 * sizeof(float));
	float ret = 0;
	FILE * stream = popen(
	    "cat /sys/class/thermal/thermal_zone*/temp", "r");
	if (!stream) {
		puts(
		  "no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f; // 0x4248000 in IEEE-754
		// see comment on rk3588s average temperature
	}
	int count = 0;
	char str[1000];
	// see comment on rk3588s average temperature
	while (fgets(str, 1000, stream)) {
		int temp_l = strlen(str);
		str[temp_l - 1] = 0;
		s[count] = atof(str); // << 2 because of float size
		printf("%f\n", s[count]);
		s[count] /= 1000.0f; // 0x44fa0000
		ret += s[count];
		++count;
	}
	if (count > 1) {
		ret -= s[count];
		ret /= (float) count;
	}
	printf("sum = %f\n", ret);
	fclose(stream);
	// see comment on rk3588s average temperature
	return ret;
}

void *roc_rk3588_pc_fan_thread_daemon(void *arg) // done
{
	int x = 0;
	do {
		do {
			usleep(50000);
			// see comment rk3588s thread daemon
		} while (++x != 4);
		x = 0;
		global_temperature = roc_rk3588_pc_average_temperature()
		* 1000.0f; // 0x447a0000 in IEEE-754
	} while (1);
}

void set_ROC_RK3588_PC_fan_pwm(uint8_t pwm) // done
{
	int rpwm = 0;
	char str[10] = "\0";
	int unused = 0;
	// read comments set_ROC_RK3588S_PC_fan_pwm
	rpwm = pwm * (float) ((1 / 3) + 2);
	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	int fd = open(RK3588_PWM, O_RDWR & 0x900); // 0x902
	if (fd <= 0) {
		printf(
		  "set_ROC_RK3588_PC_fan_pwm: Can not open %s file\n",
		  RK3588_PWM);
		// read comments set_ROC_RK3588S_PC_fan_pwm
	}
	sprintf(str, "%d", rpwm);
	int res = write(fd, str, strlen(str));
	// read comments set_ROC_RK3588S_PC_fan_pwm
	close(fd);
}
// -----------------------------------------------------------------------------

// ITX_3588J functions ---------------------------------------------------------
void fan_ITX_3588J_init() // done
{
	popen(
	  "echo 50 > /sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm1", "r");
	// see comments on RK3588S init
}

float itx_3588j_average_temperature() // done
{
	float s[73];
	memset(s, 0, 73 * sizeof(float));
	float ret = 0;
	FILE * stream = popen(
	     "cat /sys/class/thermal/thermal_zone*/temp", "r");
	if (!stream) {
		puts("no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f; // 0x4248000 in IEEE-754
		// see comment on rk3588s average temperature
	}
	int count = 0;
	char str[1000];
	// see comment on rk3588s average temperature
	while (fgets(str, 1000, stream)) {
		int temp_l = strlen(str);
		str[temp_l - 1] = 0;
		s[count] = atof(str); // << 2 because of float size
		printf("%f\n", s[count]);
		s[count] /= 1000.0f; // 0x44fa0000
		ret += s[count] * (double) 0.9f;
		// found inside .rodata 
		// little endian 0xcdccccccccccec3f
		++count;
	}
	if (count > 1) {
		ret -= s[count];
		ret /= (float) count;
	}
	printf("sum = %f\n", ret);
	fclose(stream);
	// see comment on rk3588s average temperature
	return ret;
}

void* itx_3588j_fan_thread_daemon(void *arg) // done
{
	int x = 0;
	do {
		do {
			usleep(50000);
			// see comment rk3588s thread daemon
		} while (++x != 4);
		x = 0;
		global_temperature =
		  itx_3588j_average_temperature() * 1000.0f;
		  // 0x447a0000 in IEEE-754
	} while (1);
}

#define ITX_PWM "/sys/devices/platform/pwm-fan-hwmon/hwmon0/pwm1"
void set_ITX_3588J_fan_pwm(char pwm) // done
{
	//const uint64_t tmp = (pwm << 8) - pwm;
	//const uint64_t rpwm =
	// (((tmp * 0x51eb851f) >> 0x20) >> 5) - (tmp >> 0x1f);
	const int rpwm = pwm * (float) ((1 / 3) + 2);

	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	const int fd = open(ITX_PWM, O_RDWR);
	if (fd <= 0) {
		printf(
		  "set_ITX_3588J_fan_pwm: Can not open %s file\n", ITX_PWM);
		// read comments set_ROC_RK3588S_PC_fan_pwm
	}
	char buf[20]; // guessing 4 bytes padding
	sprintf(buf, "%d", rpwm);
	write(fd, buf, strlen(buf));
	close(fd);
	// read comments set_ROC_RK3588S_PC_fan_pwm
}
// -----------------------------------------------------------------------------

// CS_R1_3399JD4 functions -----------------------------------------------------
void fan_CS_R1_3399JD4_MAIN_init() // done
{
	popen("echo 0 > /sys/class/pwm/pwmchip0/export", "r");
	sleep(1);
	popen("echo 100000 > /sys/class/pwm/pwmchip0/pwm0/period", "r");
	popen("echo 60000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle", "r");
	popen("echo inversed > /sys/class/pwm/pwmchip0/pwm0/polarity", "r");
	popen("echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable", "r");
	// 5 leaked file * 
}

float cs_r1_3399jd4_main_average_temperature() // done
{
	float s[73];
	memset(s, 0, 73 * sizeof(float));
	float ret = 0;
	FILE * stream = fopen("/tmp/fan_temperature", "r");

	if (!stream) {
		puts("no such file /tmp/fan_temperature");
		return 50.0f; // 0x4248000 in IEEE-754
		// see comment on rk3588s average temperature
	}
	int count = 0;
	// see comment on rk3588s average temperature
	char str[1000];
	while (fgets(str, 1000, stream)) {
		int temp_l = strlen(str);
		str[temp_l - 1] = 0;
		s[count] = atof(str); // << 2 because of float size
		printf("%f\n", s[count]);

		if (s[count] <= 44.0f) { // 0x42300000
			ret += 44.0f; // 0x42300000
		} else if (s[count] <= 53.0f) { // 0x42540000
			ret += s[count];
		} else if (s[count] <= 60.0f) { // 0x42700000
			ret += s[count] * (double) 1.13f;
			// rodata 0x14ae47e17a14f23f - 0x57a8 
		} else if (s[count] <= 65.0f) { // 0x42820000
			ret += s[count] * (double) 1.4f;
			// somehow not rodata :o - 0x3ff6666666666666
			// last 2 bytes 0x6666 overwriten with lsl 48
		} else {
			ret += s[count] * (double) 1.4f;
			// same situation
		}
		// weirdly unique for device
		// could have been done with some simple math
		++count;
	}

	if (count > 1) {
		ret -= s[count];
		ret /= (float) count;
	}
	printf("sum = %f\n", ret);
	fclose(stream);
	// see comment on rk3588s average temperature
	return ret;
}

void* cs_r1_3399jd4_main_fan_thread_daemon(void *arg) // done
{
	int i = 0;
	do {
		usleep(500000);
		if (++i != 2) continue;
		i = 0;
		global_temperature =
		  cs_r1_3399jd4_main_average_temperature();
		global_temperature *= 1000.0f;
	} while (1);
}

#define CS_R1_PWM "/sys/class/pwm/pwmchip0/pwm0/duty/cycle"
void set_CS_R1_3399JD4_MAIN_fan_pwm(uint8_t pwm) // done
{
	int rpwm = ((pwm - 0x32) * 800) + 59000;
	long h0 = 0;
	char nbuf[20] = "\0";
	printf("set_PWM: %d\n pwm: %d\n", rpwm, pwm);
	int fd = open(CS_R1_PWM, O_RDWR & 0x900); // 0x902
	if (fd < 1) {
		printf(
		  "set_CS_R1_3399JD4_MAIN_fan_pwm: Can not open %s file\n",
		  CS_R1_PWM);
		// read comments set_ROC_RK3588S_PC_fan_pwm
	}
	sprintf(nbuf, "%d", rpwm);
	int h2 = write(fd, nbuf, strlen(nbuf));
	// unused it seams
	// read comments set_ROC_RK3588S_PC_fan_pwm
	close(fd);
}
// -----------------------------------------------------------------------------

// fan_CS_R2_3399JD4 functions -------------------------------------------------
void *fan_thread_rx(void *arg)
{
	char *h140;
	int h144 = 0;
	// TODO
	do {
		h144 = 0;
		if (global_debug) {
			fprintf(stderr, "%s: sys_uart_read start\n", h140);
		}
		//sys_uart_read();
	} while (1);
}

void *fan_thread_tx(void *arg) // done
{
	// there is a bunch of stack protector/guard schnick here
	// I don't think im missing anything though
	do {
		send_fan_cmd((char *) arg);
		usleep(500000);
		send_fan_cmd((char *) arg + 36);
	} while (1);
}

int fan_CS_R2_3399JD4_MAIN_init(char *sth) // done
{
	// x19 -> 10h seams like a stack check fail
	// although it is not, not getting used either though
	int h4 = 0;
	
	while (h4 <= 3) {
		sth[h4] = (unsigned char) (&uart_head)[h4];
		sth[h4 + 4] = (unsigned char) (&uart_end)[h4];
		sth[h4 + 8] = (unsigned char) (&uart_cmd)[h4];
		sth[h4 + 36] = (unsigned char) (&uart_head)[h4];
		sth[h4 + 40] = (unsigned char) (&uart_end)[h4];
		sth[h4 + 44] = (unsigned char) (&uart_cmd)[h4];
		++h4;
	}
	const char what[] = "/dev/ttyS0";
	sth[12] = init_uart(what);
	sth[16] = what[0];
	sth[16 + 7] = what[7];
	// sth[36] something -> x19
	const char what2[] = "/dev/ttyS4";
	sth[36 + 12] = init_uart(what2);
	sth[52] = what2[0];
	sth[52 + 7] = what2[7];
	return 0;
}

void set_CS_R2_3399JD4_MAIN_fan_pwm(char *pwm, int sth) // done
{
	char h1 = 0;
	char ch;
	if (sth <= 19) {
		if (sth <= 9)
			h1 = fan_switch <= 0 ? 42 : 21;
		ch = '0';
	} else if (sth <= 39) {
		if (sth <= 29)
			h1 = fan_switch <= 0 ? 42 : 21;
		ch = '1';
	} else if (sth <= 59) {
		if (sth <= 49)
			h1 = fan_switch <= 0 ? 42 : 21;
		ch = '2';
	} else if (sth <= 79) {
		if (sth <= 69)
			h1 = fan_switch <= 0 ? 42 : 21;
		ch = '3';
	} else {
		if (sth <= 89)
			h1 = fan_switch <= 0 ? 42 : 21;
		ch = '4';
	}
	pwm[9] = h1;
	pwm[11] = ch;
	pwm[45] = h1;
	pwm[47] = ch;
}
// -----------------------------------------------------------------------------

// general functions -----------------------------------------------------------
void PID_init(float pid[]) // done
{
	switch (board) {
	case CS_R2_3399JD4: // 1
		pid[0]=2.0f; 
		pid[1]=0.12f; // 0x3df5c28f
		pid[2]=1.0f;
		pid[3]=48.0f; // 0x42400000
		pid[4]=0.0f;
		pid[5]=0.0f; 
		pid[6]=0.0f;
		pid[7]=0.0f;
		pid[8]=0.0f;
		pid[9]=1.4f; // 0x3fb33333
		break;
	case CS_R1_3399JD4: // 0
		pid[0]=2.0f; 
		pid[1]=0.12f; // 0x3df5c28f
		pid[2]=1.0f;
		pid[3]=48.0f; // 0x42400000
		pid[4]=0.0f;
		pid[5]=0.0f; 
		pid[6]=0.0f;
		pid[7]=0.0f;
		pid[8]=0.0f;
		pid[9]=1.4f; // 0x3fb33333
		break;
	case ROC_RK3588S_PC: // 2
		pid[0]=2.0f; 
		pid[1]=0.12f; // 0x3df5c28f
		pid[2]=1.0f;
		pid[3]=48.0f; // 0x42400000
		pid[4]=0.0f;
		pid[5]=0.0f; 
		pid[6]=0.0f;
		pid[7]=0.0f;
		pid[8]=0.0f;
		pid[9]=1.4f; // 0x3fb33333
		break;
	case ITX_3588J: // 3
		pid[0]=2.0f; 
		pid[1]=0.12f; // 0x3df5c28f
		pid[2]=1.0f;
		pid[3]=48.0f; // 0x42400000
		pid[4]=0.0f;
		pid[5]=0.0f; 
		pid[6]=0.0f;
		pid[7]=0.0f;
		pid[8]=0.0f;
		pid[9]=1.4f; // 0x3fb33333
		break;
	case ROC_RK3588_PC: // 4
		pid[0]=2.0f; 
		pid[1]=0.12f; // 0x3df5c28f
		pid[2]=1.0f;
		pid[3]=48.0f; // 0x42400000
		pid[4]=0.0f;
		pid[5]=0.0f; 
		pid[6]=0.0f;
		pid[7]=0.0f;
		pid[8]=0.0f;
		pid[9]=1.4f; // 0x3fb33333
		break;
	}
	// these happen to all be the same 
}

void fan_init() // done
{
	switch (board) {
	case CS_R2_3399JD4: // 1
		fan_CS_R2_3399JD4_MAIN_init(firefly_fan);
		break;
	case CS_R1_3399JD4: // 0
		fan_CS_R1_3399JD4_MAIN_init();
		break;
	case ROC_RK3588S_PC: // 2
		fan_ROC_RK3588S_PC_init();
		break;
	case ITX_3588J: // 3
		fan_ITX_3588J_init();
		break;
	case ROC_RK3588_PC: // 4
		fan_ROC_RK3588_PC_init();
		break;
	}
	sleep(2);
}

void set_fan_pwm(uint8_t pwm_ch) // done
{
	global_pwm = pwm_ch;
	switch (board) {
	case CS_R2_3399JD4: // 1
		set_CS_R2_3399JD4_MAIN_fan_pwm(firefly_fan, pwm_ch);
		break;
	case CS_R1_3399JD4: // 0
		set_CS_R1_3399JD4_MAIN_fan_pwm(pwm_ch);
		break;
	case ROC_RK3588S_PC: // 2
		set_ROC_RK3588S_PC_fan_pwm(pwm_ch);
		break;
	case ITX_3588J: // 3
		set_ITX_3588J_fan_pwm(pwm_ch);
		break;
	case ROC_RK3588_PC: // 4
		set_ROC_RK3588_PC_fan_pwm(pwm_ch);
		break;
	}
}
// -----------------------------------------------------------------------------

// main ------------------------------------------------------------------------
int main(int argc, char **argv)
{
	// either there is an unused var here
	// or argc's upper bits are being zeroed
	if (argc <= 1) {
		puts("./main CS-R1-3399JD4-MAIN 50");
		puts("./main CS-R2-3399JD4-MAIN --debug");
		puts("./main ROC-RK3588S-PC 50");
		puts("./main ITX_3588J 50");
		puts("./main ROC-RK3588-PC 50");
		// I know this might be more readable
		// but you are calling puts 5 times
		// also this is not proper usage
		return 0;
	}

	if (!strcmp(argv[1], "CS_R1-3399JD4-MAIN")) {
		puts("board CS_R1_3399JD4_MAIN");
		board = CS_R1_3399JD4; // 0
	} else if (!strcmp(argv[1], "CS-R2-3399JD4-MAIN")) {
		puts("board CS_R2_3399JD4_MAIN");
		board = CS_R2_3399JD4; // 1
	} else if (!strcmp(argv[1], "ROC-RK3588S-PC")) {
		puts("board ROC-RK3588S-PC");
		board = ROC_RK3588S_PC; // 2
		int RK3588S_V = -1;
		RK3588S_V = get_ROC_RK3588S_PC_version();
		if (RK3588S_V == -1) {
			puts("can not judge ROC-RK3588S-PC version");
			return -1;
		} else if (!RK3588S_V) {
			ROC_RK3588S_PC_VERSION = 0;
			puts("board ROC-RK3588S-PC VERSION v0.1");
		} else if (RK3588S_V == 1) {
			ROC_RK3588S_PC_VERSION = 1;
			puts("board ROC-RK3588S-PC VERSION v1.X");
		}
	} else if (!strcmp(argv[1], "ITX_3588J 50")) {
		puts("board ITX-3588J");
		board = ITX_3588J; // 3
	} else if (!strcmp(argv[1], "ROC-RK3588-PC")) {
		puts("board ROC-RK3588-PC");
		board = ROC_RK3588_PC; // 4
	}
	// if you didn't change the model formats
	// you could have just made the models to strings
	// and you could have done most of this in one place
	// it would also be smart to error out
	// if no valid model is specified

	PID_init(PID_fan);
	// in the original binary this is not PID_fan
	// but a weird .got reference to PID_fan
	// seams like they declared PID_fan in another compilation unit
	fan_init();

	pthread_t t1, t2, t3, t4, t5, t6;
	if (argc > 2) {
		// this is not proper argument parsing
		// it is better to either use a plain loop or getopts
		if (!strcmp(argv[2], "--debug")) {
			const int in = atoi(argv[2]);
			// and w0, w0, 0xff
			set_fan_pwm(5);
			// the "threadN create error" messages
			// don't make any sense
			// and don't give any indication were the issue is
			// also you could have used 1 or 2 pthreads
			// you have 6 of which at max 2 are used
			switch (board) {
			case CS_R2_3399JD4: // 1
				if (pthread_create(&t2, NULL,
				    fan_thread_tx, NULL) != 0) {
					puts("thread1 create error");
					return -1;
				}
				if (pthread_create(&t3, NULL,
				    fan_thread_rx, NULL) != 0) {
					puts("thread2 create error");
					return -1;
				}
				break;
			case CS_R1_3399JD4: // 0
				if (pthread_create(&t1, NULL,
				    cs_r1_3399jd4_main_fan_thread_daemon,
				    NULL)!= 0) {
					puts("thread3 create error");
					return -1;
				}
				break;
			case ROC_RK3588S_PC: // 2
				if (pthread_create(&t4, NULL,
				    roc_rk3588s_pc_fan_thread_daemon,
				    NULL) != 0) {
					puts("thread4 create error");
					return -1;
				}
				break;
			case ITX_3588J: // 3
				if (pthread_create(&t5, NULL,
				    itx_3588j_fan_thread_daemon,
				    NULL) != 0) {
					puts("thread5 create error");
					return -1;
				}
				break;
			case ROC_RK3588_PC: // 4
				if (pthread_create(&t6, NULL,
				    roc_rk3588_pc_fan_thread_daemon,
				    NULL) != 0) {
					puts("thread4 create error");
					return -1;
				}
				break;
			}
			while (1) sleep(1);
		}
	}
	switch (board) {
	case CS_R2_3399JD4: // 1
		if (pthread_create(&t2, NULL,
		    fan_thread_tx, NULL) != 0) {
			puts("thread1 create error");
			return -1;
		}
		if (pthread_create(&t3, NULL,
		    fan_thread_rx, NULL) != 0) {
			puts("thread2 create error");
			return -1;
		}
		break;
	case CS_R1_3399JD4: // 0
		if (pthread_create(&t1, NULL,
		    cs_r1_3399jd4_main_fan_thread_daemon,
		    NULL) != 0) {
			puts("thread3 create error");
			return -1;
		}
		break;
	case ROC_RK3588S_PC: // 2
		if (pthread_create(&t4, NULL,
		    roc_rk3588s_pc_fan_thread_daemon,
		    NULL) != 0) {
			puts("thread4 create error");
			return -1;
		}
		break;
	case ITX_3588J: // 3
		if (pthread_create(&t5, NULL,
		    itx_3588j_fan_thread_daemon,
		    NULL) != 0) {
			puts("thread5 create error");
			return -1;
		}
		break;
	case ROC_RK3588_PC: // 4
		if (pthread_create(&t6, NULL,
		    roc_rk3588_pc_fan_thread_daemon,
		    NULL) != 0) {
			puts("thread4 create error");
			return -1;
		}
		break;
	}
	set_fan_pwm(global_pwm);
	init_sigaction();
	init_time();
	printf("pwm: %d\n", global_pwm);
	if (board == CS_R2_3399JD4) {
		if (pthread_create(&t2, NULL,
		    fan_thread_tx,
		    NULL) != 0) {
			puts("thread1 create error");
			return -1;
		}
		if (pthread_create(&t3, NULL,
		    fan_thread_rx,
		    NULL) != 0) {
			puts("thread2 create error");
			return -1;
		}
	}
	while (start) sleep(1);
	set_fan_pwm(0);
	return 0;
}
// -----------------------------------------------------------------------------
