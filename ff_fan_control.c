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

enum sbc {
	CS_R1_3399JD4 = 0,
	CS_R2_3399JD4 = 1,
	ROC_RK3588S_PC = 2,
	ITX_3588J = 3,
	ROC_RK3588_PC = 4
};

enum sbc board;
char PID_fan[40]; // unused till now
void (*PID_fan_func)(int);
char PID_debug_buff[1024]; // unused till now
int ROC_RK3588S_PC_VERSION;
uint32_t uart_head = 2863267968; // 0x8000aaaa // unused till now
int fan_switch = 1; // 0x01000000
int global_pwm = 50; // 0x32000000

bool completed; // unused till now
// "local"
// __func__ might be needed
int temperature; // unused till now
int count; // unused till now
int tmp; // unused till now
// while tmp is a fine name for a temporary var
// for a global one that is a questionable choice

int global_debug; // unused till now
int uart_end = 8432298; // 0xaaaa8000 // unused till now
int uart_cmd = 838860800; // 0x320000000 // unused till now
char global_fan_speed[40]; // usused till now
int global_temperature;
int start = 1; // 0x01000000
int debug_buff_count; // unused till now
char firefly_fan[72];

void init_time() /* done */
{
	struct itimerval itv = {
		.it_interval = {
			.tv_sec = 0, // sp+0x30
			.tv_usec = PID_fan[0x20] // sp+0x28
		},
		.it_value = {
			.tv_sec = 5, // sp+0x20
			.tv_usec = 0 // sp+0x18
		}
	};
	setitimer(ITIMER_REAL, &itv, 0);
}

void init_sigaction() /* done */
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
	// TODO: set termios flags correctly
	if (x2 == 7)
		;
	else if (x2 == 8)
		;
	if (x3 == 3)
		;
	else {
		
	}
	tcflush(fd, 0);
	if (tcsetattr(fd, 0, &tio)) {
		perror("com set error");
		return -1;
	}
	return 0;
}

int sys_uart_close(const int fd) /* done */ /* UNUSED - thank god */
{
	close(fd);
	return 0;
	// is this really too complicated to remember?
	// also you return 0 even on failure
}

void fan_alarm(char *fan) /* done */
{
	printf("%s: speed error\n", fan + 16);
	fan[10] |=  1; // && and 0xff
	fan[10] |= 2; // && and 0xff
	// I don't get it, so I can't complain
}

int get_temperature(char *path, long something) /* done */ /* UNUSED */
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

int sys_uart_read(int fd, char *buf, int nbytes, int x3)
{
	fd_set tl_set; //char slt_str[64];
	int rbytes = 0;
	do {
		struct timeval tv = {
			.tv_sec = x3 / 1000,
			.tv_usec = (x3 % 1000) * 1000
		};
		int slt = select(fd + 1, &tl_set, NULL, NULL, &tv);
		if (slt < 0) {
			perror("sys_uart_read:select");
			break;
		}
		if (!slt) break;
		int tmp = read(fd, buf + rbytes, nbytes - rbytes);
		if (tmp < 0) {
			if (errno != EINTR) {
				perror("sys_uart_write:read");
				break;
			}
			tmp = 0;
		} else if (tmp == 0) {
			tcflush(fd, TCIFLUSH);
			return -3;
		}
		rbytes += tmp;
	} while (rbytes != nbytes);
	tcflush(fd, TCIFLUSH);
	return rbytes;
}

int sys_uart_write(int fd, char *buf, size_t bytes) /* done */
        //           long x3 /* unused */, long ch3 /* unused */, long h48 /* unused */)
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

int init_uart(const char *tty_path) /* done */
{
	const int fd = open(tty_path, O_RDWR & AT_SYMLINK_NOFOLLOW); // 0x102
	if (fd < 0) {
		printf("%s:open error!\n", tty_path);
		fprintf(stderr, "uart_open %s error\n", tty_path);
		perror("open:");
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

// ROC_RK3588S_PC -----------------------------------------
int get_ROC_RK3588S_PC_version() /* done */
{
	FILE * iv = popen("cat /sys/bus/iio/devices/iio:device0/in_voltage5_raw", "r");
	// the proper C way would be using open() and read()
	if (!iv) {
		puts("can not open /sys/bus/iio:device0/in_voltage5_raw file");
		fclose(iv);
		// should be pclose()
		return -1;
	}
	char volt_str[1000] = "";
	fgets(volt_str, 1000, iv);
	// this read should not return more than 32 bytes
	const size_t volt_len = strlen(volt_str);
	volt_str[volt_len - 1] = '\0';
	const int volt = (int) atof(volt_str);
	fclose(iv);
	// should be pclose()
	if (volt >= 0) {
		if (volt <= 0x64)
			return 0;
		if (volt >= 0x79d && volt <= 0x866)
			return 1; 
	}
	return -1;
}

void fan_ROC_RK3588S_PC_init() /* done */
{
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", "r");
	// if you don't want to do this with open() and write()
	// you should use system()
	// popen() just leaked a fd
	// also you should check if your write was actually successful
}

void set_ROC_RK3588S_PC_fan_pwm(char pwm)
{
	int rpwm = 0;
	char *nbytes = NULL;
	// var_0h zeroed
	switch (ROC_RK3588S_PC_VERSION) {
	case 0:
		rpwm = (((((pwm << 8) - pwm) * 0x51eb851f >> 20) >> 5) >> 0x1f);
		break;
	case 1:
		rpwm = (pwm * 0x100  - pwm) / 100;
		break;
	default:
		break;
	}
	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	const int fd = open("/sys/class/hwmon/hwmon1/pwm1", O_RDWR & 0x900); // 0x902
	if (fd < 1) {
		printf("set_ROC_RK3588S_PC_fan_pwm: Can not open %s file\n", "/sys/class/hwmon/hwmon1/pwm1");
		// at this point you should be returning
		// instead you are writting and closing a invalid fd
	}
	char buf[0x18];
	sprintf(buf, "%d", rpwm);
	write(fd, buf, strlen(buf));
	// good practice(and compiler warnings) would demand another fail check here
	// especially because this is a write which is likely to fail
	close(fd);
}

float roc_rk3588s_pc_average_temperature()
{
	float miau[74];
	FILE * temp_file = popen("cat /sys/class/thermal/thermal_zone*/temp", "r");
	if (temp_file == 0) {
		puts("no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f; // 0x4248000 in IEEE-754
		// we don't have a read, either through a misconfigured kernel or handware issues
		// just returning 50 risks frying your board and leaves the issue undiscovered
	}
	char buf[1000];
	int count = 0;
	float ret;
	// this read will never return more than 32 chars
	while (!fgets(buf, 1000, temp_file)) {
		const int temp_l = strlen(buf);
		buf[temp_l - 1] = 0;
		float temp = atof(buf); // some ops might still be here
		miau[count] = temp;
		printf("%f\n", miau[count]);
		miau[count] /= 1000.0f;
		ret += miau[count];
		++count;
	}
	if (1 < count) {
		ret = (ret  - miau[count]) / count;
	}
	printf("sum = %f\n", *miau);
	fclose(temp_file);
	// should be pclose()
	return ret;
}

void *roc_rk3588s_pc_fan_thread_daemon(void *arg) /* done */
{
	int x = 0;
	do {
		do {
			usleep(50000);
			// seams more practical to sleep 250000 ms once
		} while (++x != 4);
		x = 0;
		global_temperature = roc_rk3588s_pc_average_temperature() * 1000.0f; // 0x447a0000 in IEEE-754
	} while (1);
}

// ROC_RK3588_PC --------------------------------------------
void fan_ROC_RK3588_PC_init() /* done */
{
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", "r");
	// see comments on RK3588S init
}

float roc_rk3588_pc_average_temperature()
{
	int h14 = 0;
	float h18 = 0;
	FILE * temp_file = popen("cat /sys/class/thermal/thermal_zone*/temp", "r");
	if (temp_file == 0) {
		puts("no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f; // 0x4248000 in IEEE-754
		// we don't have a read, either through a misconfigured kernel or handware issues
		// just returning 50 risks frying your board and leaves the issue undiscovered
	}
	char buf[1000];
	// this read will never return more than 32 chars
	while (!fgets(buf, 1000, temp_file)) {
		const int temp_l = strlen(buf);
		buf[temp_l - 1] = 0;
		h18 = atof(buf); // some ops might still be here
		printf("%f\n", h18);
		// something more here
	}
	if (h14 < 1) {
		// something
	}
	printf("sum = %f\n", h18);
	fclose(temp_file);
	// should be pclose()
	return h18;
}

void *roc_rk3588_pc_fan_thread_daemon(void *arg) /* done */
{
	int x = 0;
	do {
		do {
			usleep(50000);
			// see comment rk3588s thread daemon
		} while (++x != 4);
		x = 0;
		global_temperature = roc_rk3588_pc_average_temperature() * 1000.0f; // 0x447a0000 in IEEE-754
	} while (1);
}

void set_ROC_RK3588_PC_fan_pwm(char pwm) 
{
	const char pwm_p[] = "/sys/class/hwmon/hwmon1/pwm1";
	const int rpwm = (pwm * 0x100 - pwm) / 100;
	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	const int fd = open(pwm_p, O_RDWR & 0x900); // 0x902
	if (fd < 1) {
		printf("set_ROC_RK3588_PC_fan_pwm: Can not open %s file\n", pwm_p);
		// read comments set_ROC_RK3588S_PC_fan_pwm
	}
	char buf[0x18];
	sprintf(buf, "%d", rpwm);
	write(fd, buf, strlen(buf));
	// read comments set_ROC_RK3588S_PC_fan_pwm
	close(fd);
}

// ITX_3588J ------------------------------------------------
void fan_ITX_3588J_init() /* done */
{
	popen("echo 50 > /sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm1", "r");
	// see comments on RK3588S init
}

float itx_3588j_average_temperature()
{
	FILE * tr_file = popen("cat /sys/class/thermal/thermal_zone*/temp", "r");
	if (!tr_file) {
		puts("no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f;
		// see comment at roc_rk3588s_pc_average_temperature()
	}
	char buf[1000];
	while (fgets(buf, 1000, tr_file) != 0) {
		
	}
	printf("sum = %f\n", 0.0f);
	fclose(tr_file);
	// should be pclose()
	return 0.0f;
}

void* itx_3588j_fan_thread_daemon(void *arg) /* done */
{
	int x = 0;
	do {
		do {
			usleep(50000);
			// see comment rk3588s thread daemon
		} while (++x != 4);
		x = 0;
		global_temperature = itx_3588j_average_temperature() * 1000.0f; // 0x447a0000 in IEEE-754
	} while (1);
}

void set_ITX_3588J_fan_pwm(char pwm) 
{
	const char pwm_p[] = "/sys/class/hwmon/hwmon0/pwm1";
	const int rpwm = (pwm * 0x100 - pwm) / 100;
	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	const int fd = open(pwm_p, O_RDWR & 0x900); // 0x902
	if (fd < 1) {
		printf("set_ITX_3588J_fan_pwm: Can not open %s file\n", pwm_p);
		// read comments set_ROC_RK3588S_PC_fan_pwm
	}
	char buf[0x18];
	sprintf(buf, "%d", rpwm);
	write(fd, buf, strlen(buf));
	// read comments set_ROC_RK3588S_PC_fan_pwm
	close(fd);
}

// ----------------------------------------
void fan_CS_R1_3399JD4_MAIN_init() /* done */
{
	popen("echo 0 > /sys/class/pwm/pwmchip0/export", "r");
	sleep(1);
	popen("echo 100000 > /sys/class/pwm/pwmchip0/pwm0/period", "r");
	popen("echo 60000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle", "r");
	popen("echo inversed > /sys/class/pwm/pwmchip0/pwm0/polarity", "r");
	popen("echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable", "r");
	// 5 leaked file * 
}

void* cs_r1_3399jd4_main_fan_thread_daemon(void *arg)
{
	// TODO
	do {
	
	} while (1);
}

void set_CS_R1_3399JD4_MAIN_fan_pwm(char pwm) 
{
	const char pwm_p[] = "/sys/class/pwm/pwmchip0/pwm0/duty_cycle";
	const int rpwm = (pwm - 0x32) * 800 + 59000;
	printf("set_PWM: %d\npwm: %d\n", rpwm, pwm);
	const int fd = open(pwm_p, O_RDWR & 0x900); // 0x902
	if (fd < 1) {
		printf("set_CS_R1_3399JD4_MAIN_fan_pwm: Can not open %s file\n", pwm_p);
		// read comments set_ROC_RK3588S_PC_fan_pwm
	}
	char buf[0x18];
	sprintf(buf, "%d", rpwm);
	write(fd, buf, strlen(buf));
	// read comments set_ROC_RK3588S_PC_fan_pwm
	close(fd);
}

// -----------------------------------------
void send_fan_cmd(char *cmd) /* done */
{
	char *format = cmd;
	long h0 = (long) format[0xc];
	// seams like you forgot to use it
	if (sys_uart_write(format[0xc], format, 4 /*, 0x2710 unused */) < 0)
		printf("%s: error\n", format + 16);
	if (sys_uart_write(format[0xc], format + 8, 4 /*, 0x2710 unused */) < 0)
		printf("%s: error\n", format + 16);
	if (sys_uart_write(format[0xc], format + 4, 4 /*, 0x2710 unused */) < 0)
		printf("%s: error\n", format + 16);
}

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

void *fan_thread_tx(void *arg) /* done */
{
	// there is a bunch of stack protector/guard schnick here
	// I don't think im missing anything though
	do {
		send_fan_cmd((char *) arg);
		usleep(500000);
		send_fan_cmd((char *) arg + 36);
	} while (1);
}

int fan_CS_R2_3399JD4_MAIN_init(char *sth)
{
	int h4 = 0;
	while (h4 <= 3) {
		sth[h4] = (unsigned char) (&uart_head)[h4];
		sth[h4 + 4] = (unsigned char) (&uart_end)[h4];
		sth[h4 + 8] = (unsigned char) (&uart_cmd)[h4];
		sth[h4 + 0x24] = (unsigned char) (&uart_head)[h4];
		sth[h4 + 0x28] = (unsigned char) (&uart_end)[h4];
		sth[h4 + 0x2c] = (unsigned char) (&uart_cmd)[h4];
	}
	sth[0xc] = init_uart("/dev/ttyS0");
	init_uart("/dev/ttyS4");
	return 0;
}

void set_CS_R2_3399JD4_MAIN_fan_pwm(char *pwm, int sth) /* done */
{
	char h1 = 0;
	char ch;
	if (sth <= 0x13) {
		if (sth <= 0x9)
			h1 = fan_switch <= 0 ? 0x2a : 0x15;
		ch = '0';
	} else if (sth <= 0x27) {
		if (sth <= 0x1d)
			h1 = fan_switch <= 0 ? 0x2a : 0x15;
		ch = '1';
	} else if (sth <= 0x3b) {
		if (sth <= 0x31)
			h1 = fan_switch <= 0 ? 0x2a : 0x15;
		ch = '2';
	} else if (sth <= 0x4f) {
		if (sth <= 0x45)
			h1 = fan_switch <= 0 ? 0x2a : 0x15;
		ch = '3';
	} else {
		if (sth <= 0x59)
			h1 = fan_switch <= 0 ? 0x2a : 0x15;
		ch = '4';
	}
	pwm[9] = h1;
	pwm[11] = ch;
	pwm[45] = h1;
	pwm[47] = ch;
}

// ------------------------------
void PID_init(float x0[])
{
	// 0x3c449ba6 in IEEE-754 0.12f
	// 0x42400000 in IEEE-754 48.0f
	// 0x000186a0 in IEEE-754 1.4f
	switch (board) {
	case CS_R2_3399JD4: // 1
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	case CS_R1_3399JD4: // 0
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	case ROC_RK3588S_PC: // 2
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	case ITX_3588J: // 3
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	case ROC_RK3588_PC: // 4
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	}
	// FIXME: var / arg is not the same, this is not correct
	// these happen to all be the same 
	// also making a function out of the initilization of a float array
	// seams a bit off
}

void fan_init() /* done */
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

void set_fan_pwm(char pwm_ch) /* done */
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

int main(int argc, char **argv)
{
	if (argc < 2) {
		puts("./main CS-R1-3399JD4-MAIN 50");
		puts("./main CS-R2-3399JD4-MAIN --debug");
		puts("./main ROC-RK3588S-PC 50");
		puts("./main ITX_3588J 50");
		puts("./main ROC-RK3588-PC 50");
		// I know this might be more readable
		// but you are calling puts 5 times for what could be one write to stdout
		// also this is not proper usage
		return 0;
	}

	if (!strcmp(argv[1], "CS_R1-3399JD4-MAIN")) {
		puts("board CS_R1_3399JD4_MAIN");
		board = CS_R1_3399JD4;
	} else if (!strcmp(argv[1], "CS-R2-3399JD4-MAIN")) {
		puts("board CS_R2_3399JD4_MAIN");
		board = CS_R2_3399JD4;
	} else if (!strcmp(argv[1], "ROC-RK3588S-PC")) {
		puts("board ROC-RK3588S-PC");
		board = ROC_RK3588S_PC;
		const int RK3588S_V = get_ROC_RK3588S_PC_version();
		if (RK3588S_V == -1) {
			puts("can not judge ROC-RK3588S-PC version");
			return -1;
		} else if (!RK3588S_V) {
			ROC_RK3588S_PC_VERSION = 0;
			puts("board ROC-RK3588 S-PC VERSION v0.1");
		} else if (RK3588S_V == 1) {
			ROC_RK3588S_PC_VERSION = 1;
			puts("board ROC-RK3588 S-PC VERSION v1.X");
		}
	} else if (!strcmp(argv[1], "ITX_3588J 50")) {
		puts("board ITX-3588J");
		board = ITX_3588J;
	} else if (!strcmp(argv[1], "ROC-RK3588-PC")) {
		puts("board ROC-RK3588-PC");
		board = ROC_RK3588_PC;
	}
	// if you didn't change the model formats
	// you could have just made the models to strings
	// and you could have done most of this in one place
	// it would also be smart to error out if no valid model is specified
	//-------------------------------------------------------------------------------

	float x0[4];
	PID_init(x0);
	fan_init();
	pthread_t t1, t2, t3, t4, t5, t6;
	if (argc > 2) {
		// this is not proper argument parsing
		// it is better to either use a plain loop or getopts
		if (strcmp("--debug", argv[2])) {
			const int in = atoi(argv[2]);
			// and w0, w0, 0xff
			set_fan_pwm(5);
			// those "threadN create error" messages don't make any sense
			// and don't give any indication were the issue is
			// also you could have used 1 or 2 pthreads
			// you have 6 of which at max 2 are used
			switch (board) {
			case CS_R2_3399JD4: // 1
				if (pthread_create(&t2, NULL, fan_thread_tx, NULL) != 0) {
					puts("thread1 create error");
					return -1;
				}
				if (pthread_create(&t3, NULL, fan_thread_rx, NULL) != 0) {
					puts("thread2 create error");
					return -1;
				}
				break;
			case CS_R1_3399JD4: // 0
				if (pthread_create(&t1, NULL, cs_r1_3399jd4_main_fan_thread_daemon, NULL) != 0) {
					puts("thread3 create error");
					return -1;
				}
				break;
			case ROC_RK3588S_PC: // 2
				if (pthread_create(&t4, NULL, roc_rk3588s_pc_fan_thread_daemon, NULL) != 0) {
					puts("thread4 create error");
					return -1;
				}
				break;
			case ITX_3588J: // 3
				if (pthread_create(&t5, NULL, itx_3588j_fan_thread_daemon, NULL) != 0) {
					puts("thread5 create error");
					return -1;
				}
				break;
			case ROC_RK3588_PC: // 4
				if (pthread_create(&t6, NULL, roc_rk3588_pc_fan_thread_daemon, NULL) != 0) {
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
		if (pthread_create(&t2, NULL, fan_thread_tx, NULL) != 0) {
			puts("thread1 create error");
			return -1;
		}
		if (pthread_create(&t3, NULL, fan_thread_rx, NULL) != 0) {
			puts("thread2 create error");
			return -1;
		}
		break;
	case CS_R1_3399JD4: // 0
		if (pthread_create(&t1, NULL, cs_r1_3399jd4_main_fan_thread_daemon, NULL) != 0) {
			puts("thread3 create error");
			return -1;
		}
		break;
	case ROC_RK3588S_PC: // 2
		if (pthread_create(&t4, NULL, roc_rk3588s_pc_fan_thread_daemon, NULL) != 0) {
			puts("thread4 create error");
			return -1;
		}
		break;
	case ITX_3588J: // 3
		if (pthread_create(&t5, NULL, itx_3588j_fan_thread_daemon, NULL) != 0) {
			puts("thread5 create error");
			return -1;
		}
		break;
	case ROC_RK3588_PC: // 4
		if (pthread_create(&t6, NULL, roc_rk3588_pc_fan_thread_daemon, NULL) != 0) {
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
		if (pthread_create(&t2, NULL, fan_thread_tx, NULL) != 0) {
			puts("thread1 create error");
			return -1;
		}
		if (pthread_create(&t3, NULL, fan_thread_rx, NULL) != 0) {
			puts("thread2 create error");
			return -1;
		}
	}
	while (start) sleep(1);
	set_fan_pwm(0);
	return 0;
}
