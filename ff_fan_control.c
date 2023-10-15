/*  +----------------+
 *  |O /   *%     \ O|
 *  | /     *%     \ |
 *  |/      *%      \|
 *  |       @@     **|
 *  |  %%%%@@@@****%%|
 *  |%%****@@@@%%%%  |
 *  |**     @@       |
 *  |\      %*      /|
 *  | \     %*     / |
 *  |O \     %*   / O|
 *  +----------------+
 *  | ff_fan_control |
 *  +----------------+
 *       reversed
 *  firefly_fan_control
 *
 *  Copymiddle (CM) 2023 Xea. All wrongs rejected 
 *  see ./UNLICENSE file
 *
 *
 *  proprietary bin by: firefly "open source" team
 *
 */

/* define: FF_NG
 *
 * anything ifdef'ed by this define is not original
 * but added by me to make this proper software
 *
 * changes include:
 * - exploit mitigation
 * - proper and SAFE error handling
 * - proper writing to /sys and /proc files (without popen and shell commands)
 * - proper argument parsing
 * - disabling unused and unnecessary code
 * - basic common sense
 */

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
#if defined(FF_NG)
#include <getopt.h>
#endif
// -----------------------------------------------------------------------------

// top level declarations ------------------------------------------------------
enum {
	CS_R1_3399JD4  = 0,
	CS_R2_3399JD4  = 1,
	ROC_RK3588S_PC = 2,
	ITX_3588J      = 3,
	ROC_RK3588_PC  = 4
} board;
// no telling if this is what they actually wrote but I hope so

#if !defined(FF_NG)
extern float PID_fan[10];
// includes from pid_stub for accurate .got pointer
#else // defined(FF_NG)
float PID_fan[10];
#endif

char PID_debug_buff[1024];
int debug_buff_count;

int ROC_RK3588S_PC_VERSION;
uint32_t uart_head = 2863267968; // 0x8000aaaa
int fan_switch = 1; // 0x01000000
int global_pwm = 50; // 0x32000000

int global_debug;
int uart_end = 8432298; // 0xaaaa8000 // unused till now
int uart_cmd = 838860800; // 0x320000000 // unused till now
int global_fan_speed[10];
int global_temperature;
int start = 1; // 0x01000000
char firefly_fan[72];
// -----------------------------------------------------------------------------

// local static ----------------------------------------------------------------
bool completed; // unused till now
// __func__ might be needed
int tmp; // unused till now
// -----------------------------------------------------------------------------

// utility functions -----------------------------------------------------------
int uart_set(int fd, int x1, int x2, char x3, int x4)
{
	struct termios old_tio;
	if (tcgetattr(fd, &old_tio)) {
		perror("Setupserial 1");
		return -1;
	}
	struct termios new_tio = {};
	// strange manual zeroing

	if (x2 == 7) {
		// or 80h by 0x20
		new_tio.c_cflag |= 0x20;
	} else if (x2 == 8) {
		// or 80h by 0x30
		new_tio.c_cflag |= CSIZE;
	}
	switch (x3) {
	case 1:
		// and 80h by 0xfffffeff
		new_tio.c_cflag |= 0xfffffeff;
		break;
	case 2:
		// or 80h by 0x100
		new_tio.c_cflag |= 0x100;
		// or 88h by 0x30
		new_tio.c_iflag |= 0x30;
		// or 80h by 0x200
		new_tio.c_cflag |= 0x200;
		break;
	case 3:
		// or 80h by 0x100
		new_tio.c_cflag |= 0x100;
		// or 88h by 0x30
		new_tio.c_iflag |= 0x30;
		// and 80h by 0xfffffdff
		new_tio.c_cflag &= 0xfffffdff;
		break;
	}
	switch (x1) {
	case 0x960:
		cfsetispeed(&new_tio, 0xb);
		cfsetospeed(&new_tio, 0xb);
		break;
	case 0x12c0:
		cfsetispeed(&new_tio, 0xc);
		cfsetospeed(&new_tio, 0xc);
		break;
	case 0x2580:
		cfsetispeed(&new_tio, 0xd);
		cfsetospeed(&new_tio, 0xd);
	case 0x1c200:
		cfsetispeed(&new_tio, 0x1002);
		cfsetospeed(&new_tio, 0x1002);
	default:
		cfsetispeed(&new_tio, 0xd);
		cfsetospeed(&new_tio, 0xd);
		break;
	}
	if (x4 == 1) {
		// and 80h by 0xfffffbf
		new_tio.c_cflag &= 0xfffffbff;
	} else if (x4 == 2) {
		// or 80h by 0x40
		new_tio.c_cflag |= CSTOPB;
	}
	/// 80h + 0xe = 0
	/// 80h + 0xf = 1
	new_tio.c_lflag = 1;
	tcflush(fd, 0);
	if (tcsetattr(fd, 0, &new_tio)) {
		perror("com set error");
		return -1;
	}
	return 0;
}

#if !defined(FF_NG)
void debug_print_buf(char *x0, char *x1, int x2)
{
	if (x0) {
		printf("\n****%s***len = %d*****************", x0, x2);
	}
	int f = 0;
	while (f < x2) {
		if (f & 0xf) {
			putchar('\n');
		}
		printf("%02x ", x1[f]);
		++f;
	}
	puts("\n");
}

int get_temperature(char *path, long something) // done - unused
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("%s:open error!\n", path);
		fprintf(stderr, "uart_open %s error\n", path);
		// failed to adjust error message copy pasted from uart_open
		perror("open:");
		// 3 error messages again
		// would fix but this is a redudant function
		return -3;
	}
	char buf[20]; // presumeably 4 bytes padding
	int ret;
	if (read(fd, buf, 20) == 0) {
		printf("read error: %s\n", path);
		// return uninitilized
		// would fix with ifdef but this function is redundant anyhow
	} else {
		ret = atoi(buf);
		printf("read temperature: %d\n", ret);
	}
	close(fd);
	return ret;
}

int sys_uart_open(char *path, int h14, int h10)
{
	int fd = open(path, 0x102);
	if (fd < 0) {
		fprintf(stderr, "uart_open %s error\n", path);
		perror("open:");
		return -3;
	}
	if (uart_set(fd, h14, 8, h10, 1) >= 0) {
		fwrite("uart set failed!\n", 1, 0x11, stderr);
		return -4;
	}
	printf("%s[%d]: fd = %d\n", "sys_uart_open", 0x102, fd);
	return fd;
}

int sys_uart_close(int fd) // done - unused
{
	close(fd);
	return 0;
	// is this really too complicated to remember?
	// also you return 0 even on failure
}
#else // defined(FF_NG)
int sys_write_num(long num, char *path)
{
	char buf[64];
	if (sprintf(buf, "%ld", num) <= 0) {
		return 1;
	}
	int fd = open(path, O_WRONLY);
	if (fd < 0)
		return 1;
	if (write(fd, buf, strlen(buf)) <= 0) {
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}

#define WRITE_NUM_FATAL(NUM, PATH) \
	do { \
		if (sys_write_num(NUM, PATH)) { \
			fprintf(stderr, \
			        "failed to write: %ld to: %s\n", (long) NUM, \
			        PATH); \
			exit(1); \
		} \
	} while(0)

int sys_write_str(char *str, char *path)
{
	int fd = open(path, O_WRONLY);
	if (fd < 0)
		return 1;
	if (write(fd, str, strlen(str)) <= 0) {
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}

#define WRITE_STR_FATAL(STR, PATH) \
	do { \
		if (sys_write_str(STR, PATH)) { \
			fprintf(stderr, \
			        "failed to write: %s to: %s\n", STR, \
			        PATH); \
			exit(1); \
		} \
	} while(0)

int sys_cat_file(char *buf, size_t count, char *path)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return 1;
	if (read(fd, buf, count) < 0) {
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}

#define CAT_FILE_FATAL(BUF, SIZE, PATH) \
	do { \
		if (sys_cat_file(BUF, SIZE, PATH)) { \
			fprintf(stderr, \
			        "failed to read: %s\n", PATH); \
			exit(1); \
		} \
	} while(0)
#endif // defined(FF_NG)

// knowingly reimplemented a libc function...
char *local_strstr(char *x1, char *x2, int x3) // done
{
	char *n = x1;
	int h20 = 0;
	int h24 = 0;
	if (x1 || x2) {
		return x1;
	}
	while (x3--) {
		char *h28 = x1;
		char *h30 = x2;
		
		while (h28++ == h30++) {
			if (++x3 > 2) {
				return x1;
			}
		}
		h24 = 0;
		++h20;
		x1 = n + h20;
	}
	return NULL;
}

void fan_alarm(char *fan) // done
{
	printf("%s: speed error\n", fan + 16);
	fan[10] |= 1; // & 0xff - only needed for char
	fan[10] |= 2; // & 0xff - only needed for char
	// I don't get it, so I can't complain
}

int sys_uart_read(int fd, char *buf, int nbytes, int it) // done
{
	fd_set read_fds;
	int rbytes = 0;
	// char *bufp = buf
	// bogus pointer to buf, I just presume this is a compiler op

	#if !defined(FF_NG)
	do {
		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);
		// you are setting up a completly new fd_set every iteration
	#else // defined(FF_NG)
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);
	do {
	#endif // defined(FF_NG)
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
		} else if (!ret) {
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
	while (nbytes) {
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
		#if !defined(FF_NG)
		printf("%s:open error!\n", tty_path);
		fprintf(stderr, "uart_open %s error\n", tty_path);
		perror("open:");
		// three error messages...
		#else // defined(FF_NG)
		fprintf(stderr,
		    "init_uart %s: %s\n",
		    tty_path, strerror(errno));
		#endif // defined(FF_NG)
		return -3;
	}
	if (1 > uart_set(fd, 1, 8, 0, 1)) {
		#if !defined(FF_NG)
		printf("%s:set error", tty_path);
		// missing \n
		fwrite("uart set failed!\n", 1, 17, stderr);
		// fputs would be a bit more elagant
		// also two error messages
		#else // defined(FF_NG)
		fprintf(stderr, "uart set error: %s\n", tty_path);
		#endif // defined(FF_NG)
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
	char volt_str[1000] = "";
	#if !defined(FF_NG)
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
	fgets(volt_str, 1000, iv);
	#else // defined(FF_NG)
	CAT_FILE_FATAL(volt_str, 1000, "/sys/bus/iio/devices/iio:device0/in_voltage5_raw");
	#endif // defined(FF_NG)
	// this read should not return more than 64 bytes
	size_t volt_len = strlen(volt_str);
	volt_str[volt_len - 1] = '\0';
	int volt = (int) atof(volt_str);
	#if !defined(FF_NG)
	fclose(iv);
	#endif // !defined(FF_NG)
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
	#if !defined(FF_NG)
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", "r");
	// if you don't want to do this with open() and write()
	// but incist on this, you should atleast use system()
	// popen() just leaked a fd
	// also you should probably check
	// if your write was actually successful
	#else // defined(FF_NG)
	WRITE_NUM_FATAL(50, "/sys/class/hwmon/hwmon1/pwm1");
	#endif // defined(FF_NG)
}

#define RK3588_PWM "/sys/class/hwmon/hwmon1/pwm1"
void set_ROC_RK3588S_PC_fan_pwm(uint8_t pwm) // done
{
	int rpwm = 0;
	char str[12] = "\0";
	#if !defined(FF_NG)
	int unused = 0;
	// while radare saw it as the 16th place in str being zeroed
	// I think its more likely that this is a var
	// they never used and therefore was interpreted as str + 16
	#endif // !defined(FF_NG)
	#if !defined(FF_NG)
	switch (ROC_RK3588S_PC_VERSION) {
	case 0:
		rpwm = pwm * (float) ((1 / 3) + 2);
		break;
	case 1:
		rpwm = pwm * (float) ((1 / 3) + 2);
		break;
	// congratulations, your switch has a duplicate case
	default:
		// if version check fails, the fan will be zerod
		// THIS IS DANGEROUS!!!
		break;
	}
	#else // defined(FF_NG)
		if (ROC_RK3588S_PC_VERSION == 0 ||
		    ROC_RK3588S_PC_VERSION == 1) {
			rpwm = pwm * (float) ((1 / 3) + 2);
		} else {
			fprintf(stderr, "invalid ROC_RK3588S_PC_VERSION: %d\n",
			    ROC_RK3588S_PC_VERSION);
			exit(1);
		}
	#endif // defined(FF_NG)

	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	int fd = open(RK3588_PWM, O_RDWR & 0x900); // 0x902
	if (fd <= 0) {
		printf(
		  "set_ROC_RK3588S_PC_fan_pwm: Can not open %s file\n",
		  RK3588_PWM);
		// at this point you should be returning
		// instead you are writting and closing a invalid fd
		#if defined(FF_NG)
		return;
		#endif
	}
	sprintf(str, "%d", rpwm);
	int res = write(fd, str, strlen(str));
	// this variable seams to stay unused
	// also good practice(and compiler warnings)
	// would demand another fail check here
	// especially because this is a write which is likely to fail
	#if defined(FF_NG)
	if (res <= 0) {
		fprintf(stderr, "failed to set pwm on : %s\n", RK3588_PWM);
		exit(1);
	}
	#endif // defined(FF_NG)
	close(fd);
}

float roc_rk3588s_pc_average_temperature() // done
{
	float s[73];
	memset(s, 0, 73 * sizeof(float));
	float ret = 0;
	char str[1000];
	int count = 0;
	#if !defined(FF_NG)
	FILE * stream = popen(
	    "cat /sys/class/thermal/thermal_zone*/temp", "r");
	if (!stream) {
		puts("no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f; // 0x4248000 in IEEE-754
		// we don't have a read
		// either through a misconfigured kernel or handware issues
		// just returning 50 risks frying your board
	}
	// this read should not return more than 64 bytes
	while (fgets(str, 1000, stream)) {
	#else // defined(FF_NG)
	// IMPORTANT!!! THIS SHOULD BE A WILDCARD AT THE END OF THERMAL_ZONE
	// I need to make a readdir loop and cat each of them
	CAT_FILE_FATAL(str, 1000, "/sys/class/thermal/thermal_zone/temp");
	{
	#endif // defined(FF_NG)
		int temp_l = strlen(str);
		str[temp_l - 1] = 0;
		s[count] = atof(str);
		printf("%f\n", s[count]);
		s[count] /= 1000.0f; // 0x44fa0000
		ret += s[count];
		++count;
	}
	if (count > 1) {
		ret -= s[count]; // has to be zero
		ret /= (float) count; // also has to be zero
	}
	printf("sum = %f\n", ret);
	#if !defined(FF_NG)
	fclose(stream);
	// should be pclose()
	#endif // !defined(FF_NG)
	return ret;
}

void *roc_rk3588s_pc_fan_thread_daemon(void *arg) // done
{
	#if !defined(FF_NG)
	int x = 0;
	#endif
	do {
		#if !defined(FF_NG)
		do {
			usleep(50000);
			// seams more practical to sleep 250000 ms once...
		} while (++x != 4);
		x = 0;
		#else // defined(FF_NG)
		sleep(250);
		// I don't know why this would not work
		// if it does fail blame me
		#endif // defined(FF_NG)
		global_temperature =
		    roc_rk3588s_pc_average_temperature() * 1000.0f;
		// 0x447a0000 in IEEE-754
	} while (1);
}
// -----------------------------------------------------------------------------

// ROC_RK3588_PC functions -----------------------------------------------------
void fan_ROC_RK3588_PC_init() // done
{
	#if !defined(FF_NG)
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", "r");
	// see comments on RK3588S init
	#else // defined(FF_NG)
	WRITE_NUM_FATAL(50, "sys/class/hwmon/hwmon1/pwm1");
	#endif // defined(FF_NG)
}

float roc_rk3588_pc_average_temperature() // done
{
	float s[73];
	memset(s, 0, 73 * sizeof(float));
	float ret = 0;
	FILE * stream = popen(
	    "cat /sys/class/thermal/thermal_zone*/temp", "r");
	if (!stream) {
		#if !defined(FF_NG)
		puts(
		  "no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f; // 0x4248000 in IEEE-754
		// see comment on rk3588s average temperature
		#else // defined(FF_NG)
		fputs(
		  "failed to read from /sys/class/thermal/thermal_zone*/temp",
		  stderr);
		exit(1);
		#endif // defined(FF_NG)
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
		ret -= s[count]; // has to be zero
		ret /= (float) count; // also has to be zero
	}
	printf("sum = %f\n", ret);
	//#if !defined(FF_NG)
	fclose(stream);
	// see comment on rk3588s average temperature
	//#endif // !defined(FF_NG)
	return ret;
}

void *roc_rk3588_pc_fan_thread_daemon(void *arg) // done
{
	int x = 0;
	do {
		#if !defined(FF_NG)
		do {
			usleep(50000);
			// see comment rk3588s thread daemon
		} while (++x != 4);
		#else // defined(FF_NG)
		sleep(250);
		// see comment rk3588s thread daemon
		#endif // defined(FF_NG)
		x = 0;
		global_temperature = roc_rk3588_pc_average_temperature()
		* 1000.0f; // 0x447a0000 in IEEE-754
	} while (1);
}

void set_ROC_RK3588_PC_fan_pwm(uint8_t pwm) // done
{
	int rpwm = 0;
	char str[10] = "\0";
	#if !defined(FF_NG)
	int unused = 0;
	// read comments set_ROC_RK3588S_PC_fan_pwm
	#endif // !defined(FF_NG)
	rpwm = pwm * (float) ((1 / 3) + 2);
	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	int fd = open(RK3588_PWM, O_RDWR & 0x900); // 0x902
	if (fd <= 0) {
		printf(
		  "set_ROC_RK3588_PC_fan_pwm: Can not open %s file\n",
		  RK3588_PWM);
		// read comments set_ROC_RK3588S_PC_fan_pwm
		#if defined(FF_NG)
		return;
		#endif
	}
	sprintf(str, "%d", rpwm);
	int res = write(fd, str, strlen(str));
	// read comments set_ROC_RK3588S_PC_fan_pwm
	#if defined(FF_NG)
	if (res <= 0) {
		fprintf(stderr, "failed to set pwm on : %s\n", RK3588_PWM);
		exit(1);
	}
	#endif // defined(FF_NG)
	close(fd);
}
// -----------------------------------------------------------------------------

// ITX_3588J functions ---------------------------------------------------------
void fan_ITX_3588J_init() // done
{
	#if !defined(FF_NG)
	popen(
	  "echo 50 > /sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm1", "r");
	// see comments on RK3588S init
	#else // defined(FF_NG)
	WRITE_NUM_FATAL(50, "/sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm1");
	#endif // defined(FF_NG)
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
		ret -= s[count]; // has to be zero
		ret /= (float) count; // also has to be zero
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
		#if !defined(FF_NG)
		do {
			usleep(50000);
			// see comment rk3588s thread daemon
		} while (++x != 4);
		#else // defined(FF_NG)
		sleep(250);
		// see comment rk3588s thread daemon
		#endif // defined(FF_NG)
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
	int rpwm = pwm * (float) ((1 / 3) + 2);

	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	int fd = open(ITX_PWM, O_RDWR);
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
	#if !defined(FF_NG)
	popen("echo 0 > /sys/class/pwm/pwmchip0/export", "r");
	sleep(1);
	popen("echo 100000 > /sys/class/pwm/pwmchip0/pwm0/period", "r");
	popen("echo 60000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle", "r");
	popen("echo inversed > /sys/class/pwm/pwmchip0/pwm0/polarity", "r");
	popen("echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable", "r");
	// 5 leaked FPs and ever more popen bs and again no error checking!!!
	#else // defined(FF_NG)
	WRITE_NUM_FATAL(0,          "/sys/class/pwm/pwmchip0/export");
	sleep(1);
	WRITE_NUM_FATAL(100000,     "/sys/class/pwm/pwmchip0/pwm0/period");
	WRITE_NUM_FATAL(60000,      "/sys/class/pwm/pwmchip0/pwm0/duty_cycle");
	WRITE_STR_FATAL("inversed", "/sys/class/pwm/pwmchip0/pwm0/polarity");
	WRITE_NUM_FATAL(1,          "/sys/class/pwm/pwmchip0/pwm0/enable");
	#endif // defined(FF_NG)
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
	#if !defined(FF_NG)
	char str[1000];
	while (fgets(str, 1000, stream)) {
	#else //defined(FF_NG)
	char str[64];
	while (fgets(str, 64, stream) && count != 73) {
	#endif // defined(FF_NG)
		int temp_l = strlen(str);
		str[temp_l - 1] = 0;
		s[count] = atof(str); // << 2 because of float size
		// THIS IS AN EXPLOIT!!!!
		//
		// /tmp files with static names are not safe
		// which ever daemon is supposed to write to it
		// this not only allows you to set the temperature to 0
		// and fry the board with alot of load
		// but the fgets() loop above parses and stores each string
		// into a value in a stack allocated float array

		// the float array is never used again, making me wonder why it even exists...
		// maybe only to make the buffer overflow exploit work?
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
		ret -= s[count]; // has to be 0
		ret /= (float) count; // also has to be 0
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
		// guess you did the sensible thing here hihi
		if (++i != 2)
			continue;
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
		#if defined(FF_NG)
		return;
		#endif
	}
	sprintf(nbuf, "%d", rpwm);
	int res = write(fd, nbuf, strlen(nbuf));
	// unused it seams
	// read comments set_ROC_RK3588S_PC_fan_pwm
	#if defined(FF_NG)
	if (res <= 0) {
		fprintf(stderr,
		    "failed to set pwm on : %s\n", CS_R1_PWM);
		exit(1);
	}
	#endif // defined(FF_NG)
	close(fd);
}
// -----------------------------------------------------------------------------

// fan_CS_R2_3399JD4 functions -------------------------------------------------
void *fan_thread_rx(void *arg)
{
	char s[200]; // this size is a guess
	char *ptr = (char*) arg;
	char buf[36]; // 8 * 4 + 4 // alot of zero
	int h50 = 0; // somehow this zero is .rodata
	// alot of zero
	do {
		char *arg_buf = ptr;
		int ch2 = 0;
		if (global_debug) {
			fprintf(stderr,
			// stderr is a guess, but a quite likely one
				"%s: sys_uart_read start\n", arg_buf + 16);
		}
		ch2 = sys_uart_read(arg_buf[12], buf, 50, 100);
		usleep(500000);
		int format;
		if (ch2) {
			arg_buf[10] = 0;
			if (global_debug) {
				fprintf(stderr,
				// stderr is a guess, but a quite likely one
					"ret: %d\n", ch2);
			}
			char *h40 = local_strstr(buf, (char *)&uart_head, ch2);
			if (h40) {
				if (global_debug) {
					fprintf(stderr,
					// stderr is a guess, but a quite likely one
						"%s: success %d\t", arg_buf + 16, ch2);
				}
				format = 0;
				while (format <= 5) {
					if (format * 2 > 44) { // << 1
						global_fan_speed[format] =
							((*((format * 2) + 4 + h40)) << 8) | *(((format * 2) + 5) + h40);
					}
					if (global_fan_speed[format] <= 1299) {
						fan_alarm(arg_buf);
					}
					if (global_debug) {
						fprintf(stderr,
						// stderr is a guess, but a quite likely one
							"speed %d: %d ", format + 1, global_fan_speed[format]);
					}
					++format;
				}
				if (global_debug) {
					fputc('\n', stderr);
					// stderr is a guess, but a quite likely one
				}
			}
		} else {
			fan_alarm(arg_buf);
			format = 0;
			while (format <= 3) {
				global_fan_speed[format + 6] = 0;
				++format;
			}
		}
		int h24 = 0;
		format = 0;
		while (format <= 9) {
			h24 += sprintf(s + h24,
				"node_cluster_fan_speed{fan=\"fan%d\"} %d.0\n",
				format + 1,
				global_fan_speed[format]);
			++format;
		}
		FILE *fan_prom = fopen("/var/netrecovery/collector/fan.prom.xxx", "r");
		if (!fan_prom) {
			puts("open error!");
		} else {
			fputs(s, fan_prom);
			fclose(fan_prom);
			rename("/var/netrecovery/collector/fan.prom.xxx",
			       "/var/netrecovery/collector/fan.prom");
		}
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

float PID_control(float s0, float s1, float *s) // done
{
	// remember times 4
	s[4] = s0 - s1;
	if ((int) s[4] >= 999 && (int) s[4] <= 13492) {
		s[4] = 0;
		s[9] = 1;
	}
	s[7] += s[4];
	if (debug_buff_count <= 1023) {
		debug_buff_count +=
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "SetValue: %f\n", s0);
	}
	if (debug_buff_count <= 1023) {
		debug_buff_count +=
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "ActualValue: %f\n", s1);
	}
	if (debug_buff_count <= 1023) {
		debug_buff_count +=
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "PID->Ek: %f\n", s[4]);
	}
	if (debug_buff_count <= 1023) {
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "PID->LocSum: %f\n", s[7]);
	}
	float s2 = s[3] + (s[2] * (s[5] - s[4])) + (s[0] * s[4]) + (s[1] * s[7]);
	s2 = -s2;
	if (s[7] > 6000000.0f) {
	// 0x4ab71b00 - 6000000
		s[7] = 6000000.0f;
	} else if (s[7] < -6000000.0f) { // assumed but reasonable
	// 0xcab71b00 - -6000000
		s[7] = -6000000.0f;
	}
	if (debug_buff_count <= 1023) {
		debug_buff_count +=
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "PIDLoc: %f\n", s2);
	}
	s[5] = s[4];
	if (s[3] + (s2 / 1000.0f) < 50.0f) {
	// 0x447a0000 - 1000
	// 0x42480000 - 50
		s[7] -= s[4];
		return 50.0f;
		// 0x42480034 - 50.0001983643
	} else {
		return (s2 / 1000.0f) + s[3];
		// 0x447a0000
	}
}

void PID_fan_func(int x) // done
{
	static float temperature;
	temperature = global_temperature;
	float h14 = PID_control(53000.0f, temperature, PID_fan);
	// 0x474f0800 - 53000
	if (h14 > 100.0f) {
	// 0x42c80000 - 100
		h14 = 100.0f;
	} else if (h14 < 0) {
		h14 = 0.0f;
	}
	if (debug_buff_count <= 1023) {
		debug_buff_count +=
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "pwm:%f \n", h14);
	}
	set_fan_pwm((char) h14);
	// guess of what, fcvtzu and umov did
	static int count;
	if (debug_buff_count <= 1023) {
		debug_buff_count +=
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "count is %d\n", count++);
	}
	if (debug_buff_count <= 1023) {
		debug_buff_count +=
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "fan_switch is %d\n", fan_switch);
	}
	//if (count - (((((count * 0x057619f1) >> 0x20) >> 7) -
	// (count >> 0x1f)) * 0x1770)) {
	if (count % 6000 == 0) {
		fan_switch = !fan_switch;
		if (count == 30000) {
			count = 0;
		}
	}
	if (debug_buff_count <= 1023) {
		debug_buff_count +=
		    sprintf(&PID_debug_buff[debug_buff_count],
		            "PID_fan_func exit\n");
	}
	FILE *stream = fopen("/tmp/fan_ctl_debug.xxx", "w");
	if (!stream) {
		puts("open error!");
		return;
	}
	fputs(PID_debug_buff, stream);
	debug_buff_count = 0;
	printf("%s", PID_debug_buff);
	fclose(stream);
}

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
	#if !defined(FF_NG)
	sigaction(SIGALRM, &sa, NULL);
	// might want an error check here
	#else // defined(FF_NG)
	if (sigaction(SIGALRM, &sa, NULL)) {
		fputs("failed to setup signal action on SIGALARM",
		      stderr);
		exit(1);
	}
	#endif // defined(FF_NG)
}

void PID_init(float pid[]) // done
{
	#if !defined(FF_NG)
	switch (board) {
	case CS_R2_3399JD4: // 1
		pid[0]=2.0f; pid[1]=0.12f; /* 0x3df5c28f */
		pid[2]=1.0f; pid[3]=48.0f; /* 0x42400000 */
		pid[4]=0.0f; pid[5]=0.0f;
		pid[6]=0.0f; pid[7]=0.0f;
		pid[8]=0.0f; pid[9]=1.4f; /* 0x3fb33333 */
		break;
	case CS_R1_3399JD4: // 0
		pid[0]=2.0f; pid[1]=0.12f; /* 0x3df5c28f */
		pid[2]=1.0f; pid[3]=48.0f; /* 0x42400000 */
		pid[4]=0.0f; pid[5]=0.0f;
		pid[6]=0.0f; pid[7]=0.0f;
		pid[8]=0.0f; pid[9]=1.4f; /* 0x3fb33333 */
		break;
	case ROC_RK3588S_PC: // 2
		pid[0]=2.0f; pid[1]=0.12f; /* 0x3df5c28f */
		pid[2]=1.0f; pid[3]=48.0f; /* 0x42400000 */
		pid[4]=0.0f; pid[5]=0.0f;
		pid[6]=0.0f; pid[7]=0.0f;
		pid[8]=0.0f; pid[9]=1.4f; /* 0x3fb33333 */
		break;
	case ITX_3588J: // 3
		pid[0]=2.0f; pid[1]=0.12f; /* 0x3df5c28f */
		pid[2]=1.0f; pid[3]=48.0f; /* 0x42400000 */
		pid[4]=0.0f; pid[5]=0.0f;
		pid[6]=0.0f; pid[7]=0.0f;
		pid[8]=0.0f; pid[9]=1.4f; /* 0x3fb33333 */
		break;
	case ROC_RK3588_PC: // 4
		pid[0]=2.0f; pid[1]=0.12f; /* 0x3df5c28f */
		pid[2]=1.0f; pid[3]=48.0f; /* 0x42400000 */
		pid[4]=0.0f; pid[5]=0.0f;
		pid[6]=0.0f; pid[7]=0.0f;
		pid[8]=0.0f; pid[9]=1.4f; /* 0x3fb33333 */
		break;
	}
	// these cases happen to all be the same piles
	#else // defined(FF_NG)
	pid[0]=2.0f; pid[1]=0.12f; /* 0x3df5c28f */
	pid[2]=1.0f; pid[3]=48.0f; /* 0x42400000 */
	pid[4]=0.0f; pid[5]=0.0f;
	pid[6]=0.0f; pid[7]=0.0f;
	pid[8]=0.0f; pid[9]=1.4f; /* 0x3fb33333 */
	#endif // defined(FF_NG)
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
// -----------------------------------------------------------------------------

#if defined(FF_NG)
static void usage(char * name)
{
	printf(
	     "Usage: %s [BOARD] [PWM]\n"
	     "Regulate fan speed for some firefly SBCs\n"
	     "\n"
	     "Supported Boards:\n"
	     "  - CS_R1-3399JD4\n"
	     "  - CS_R2-3399JD4\n"
	     "  - ROC-RK3588S-PC\n"
	     "  - ITX_3588J\n"
	     "  - ROC-RK3588-PC\n"
	     "\n"
	     "Flags:\n"
	     "  -h --help     print usage\n"
	     "  -d --debug    enable debug mode\n",
	     name);
}
#endif // defined(FF_NG)

// main ------------------------------------------------------------------------
int main(int argc, char **argv)
{
	// either there is an unused var here
	// or argc's upper bits are being zeroed
	if (argc <= 1) {
		#if !defined(FF_NG)
		puts("./main CS-R1-3399JD4-MAIN 50");
		puts("./main CS-R2-3399JD4-MAIN --debug");
		puts("./main ROC-RK3588S-PC 50");
		puts("./main ITX_3588J 50");
		puts("./main ROC-RK3588-PC 50");
		// I know this might be more readable
		// but you are calling puts 5 times
		// also this is not proper usage
		// AND the CS_R*-3399JD4 boards probably don't have
		// -MAIN in their name
		// also the CS_R*-3399JD4 boards are writen wrong here
		#else // defined(FF_NG)
			usage(argv[0]);
		#endif // defined(FF_NG)
		return 0;
	}

	#if defined(FF_NG)
	const struct option long_opt[] = {
		{"help", no_argument, 0, 'h'},
		{"debug", no_argument, 0, 'd'},
		{0,0,0,0}
	};
	bool debug_mode = false;
	do {
		int opt = getopt_long(argc, argv,
		    "hd", long_opt, NULL);

		if (opt == -1)
			break;

		switch (opt) {
		default:
		case '?':
			return 1;
		case 'h':
			usage(argv[0]);
			exit(1);
		case 'd':
			debug_mode = true;
			break;
		}
	} while (1);
	
	if (optind >= argc) {
		fprintf(stderr,
		    "no arguments provided\n");
		usage(argv[0]);
		return 1;
	}
	#endif // defined(FF_NG)

	#if !defined(FF_NG)
	if (!strcmp(argv[1], "CS_R1-3399JD4-MAIN")) {
		puts("board CS_R1_3399JD4_MAIN");
		board = CS_R1_3399JD4; // 0
	} else if (!strcmp(argv[1], "CS-R2-3399JD4-MAIN")) {
		puts("board CS_R2_3399JD4_MAIN");
		board = CS_R2_3399JD4; // 1
	// once again, I don't think -MAIN belongs in the name
	#else // defined(FF_NG)
	if (!strcmp(argv[optind], "CS_R1-3399JD4")) {
		puts("board CS_R1_3399JD4");
		board = CS_R1_3399JD4; // 0
	} else if (!strcmp(argv[optind], "CS-R2-3399JD4")) {
		puts("board CS_R2_3399JD4");
		board = CS_R2_3399JD4; // 1
	#endif // defined(FF_NG)
	#if !defined(FF_NG)
	} else if (!strcmp(argv[1], "ROC-RK3588S-PC")) {
	#else // defined(FF_NG)
	} else if (!strcmp(argv[optind], "ROC-RK3588S-PC")) {
	#endif // defined(FF_NG)
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
	#if !defined(FF_NG)
	} else if (!strcmp(argv[1], "ITX_3588J 50")) {
	#else // defined(FF_NG)
	} else if (!strcmp(argv[optind], "ITX_3588J 50")) {
	#endif // defined(FF_NG)
		puts("board ITX-3588J");
		board = ITX_3588J; // 3
	#if !defined(FF_NG)
	} else if (!strcmp(argv[1], "ROC-RK3588-PC")) {
	#else // defined(FF_NG)
	} else if (!strcmp(argv[optind], "ROC-RK3588-PC")) {
	#endif // defined(FF_NG)
		puts("board ROC-RK3588-PC");
		board = ROC_RK3588_PC; // 4
	#if defined(FF_NG)
	} else {
		fprintf(stderr, "invalid board: %s\n", argv[optind]);
		return 1;
	#endif // defined(FF_NG)
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
	#if !defined(FF_NG)
	if (argc > 2) {
	#endif
		// this is not proper argument parsing
		// it is better to either use a plain loop or getopts
		#if !defined(FF_NG)
		if (!strcmp(argv[2], "--debug")) {
			int in = atoi(argv[2]);
		#else // defined(FF_NG)
		if (debug_mode) {
			int in = atoi(argv[optind + 1]);
		#endif // defined(FF_NG)
			// and w0, w0, 0xff
			set_fan_pwm(5);
			// the "threadN create error" messages
			// don't make any sense
			// and don't give any indication were the issue is
			// also you could have used 1 or 2 pthreads
			// you have 6 of which at max 2 are used

			// even worse, Im certain there is no need for pthreads
			// besides for uart on CS_R2_3399JD4
			switch (board) {
			case CS_R2_3399JD4: // 1
				if (pthread_create(&t2, NULL,
				    fan_thread_tx,
				    firefly_fan) != 0) {
					puts("thread1 create error");
					return -1;
				}
				if (pthread_create(&t3, NULL,
				    fan_thread_rx,
				    firefly_fan) != 0) {
					puts("thread2 create error");
					return -1;
				}
				break;
			case CS_R1_3399JD4: // 0
				if (pthread_create(&t1, NULL,
				    cs_r1_3399jd4_main_fan_thread_daemon,
				    firefly_fan)!= 0) {
					puts("thread3 create error");
					return -1;
				}
				break;
			case ROC_RK3588S_PC: // 2
				if (pthread_create(&t4, NULL,
				    roc_rk3588s_pc_fan_thread_daemon,
				    firefly_fan) != 0) {
					puts("thread4 create error");
					return -1;
				}
				break;
			case ITX_3588J: // 3
				if (pthread_create(&t5, NULL,
				    itx_3588j_fan_thread_daemon,
				    firefly_fan) != 0) {
					puts("thread5 create error");
					return -1;
				}
				break;
			case ROC_RK3588_PC: // 4
				if (pthread_create(&t6, NULL,
				    roc_rk3588_pc_fan_thread_daemon,
				    firefly_fan) != 0) {
					puts("thread4 create error");
					return -1;
				}
				break;
			}
			while (1) sleep(1);
		}
	#if !defined(FF_NG)
	} // if argc end
	#endif
	switch (board) {
	case CS_R2_3399JD4: // 1
		if (pthread_create(&t2, NULL,
		    fan_thread_tx,
		    firefly_fan) != 0) {
			puts("thread1 create error");
			return -1;
		}
		if (pthread_create(&t3, NULL,
		    fan_thread_rx,
		    firefly_fan) != 0) {
			puts("thread2 create error");
			return -1;
		}
		break;
	case CS_R1_3399JD4: // 0
		if (pthread_create(&t1, NULL,
		    cs_r1_3399jd4_main_fan_thread_daemon,
		    firefly_fan) != 0) {
			puts("thread3 create error");
			return -1;
		}
		break;
	case ROC_RK3588S_PC: // 2
		if (pthread_create(&t4, NULL,
		    roc_rk3588s_pc_fan_thread_daemon,
		    firefly_fan) != 0) {
			puts("thread4 create error");
			return -1;
		}
		break;
	case ITX_3588J: // 3
		if (pthread_create(&t5, NULL,
		    itx_3588j_fan_thread_daemon,
		    firefly_fan) != 0) {
			puts("thread5 create error");
			return -1;
		}
		break;
	case ROC_RK3588_PC: // 4
		if (pthread_create(&t6, NULL,
		    roc_rk3588_pc_fan_thread_daemon,
		    firefly_fan) != 0) {
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
		    firefly_fan) != 0) {
			puts("thread1 create error");
			return -1;
		}
		if (pthread_create(&t3, NULL,
		    fan_thread_rx,
		    firefly_fan) != 0) {
			puts("thread2 create error");
			return -1;
		}
	}
	while (start)
		sleep(1);
	set_fan_pwm(0);
	return 0;
}
// -----------------------------------------------------------------------------
