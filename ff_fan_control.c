#include <fcntl.h>
#include <signal.h>
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
char PID_fan[40];
void (*PID_fan_func)(void);
char PID_debug_buff[1024];
int ROC_RK3588S_PC_VERSION;
int uart_head = 0x8000aaaa;
int fan_switch;
char global_pwm[16];
char sth_pwm[16];

bool completed;
int temperature;
int count;
int tmp;
int global_debug;
int uart_end;
int uart_cmd;
char global_fan_speed[40];
int global_temperature;
int start;
int debug_buff_count;
char firefly_fan[72];

void init_time()
{
	struct itimerval itv = {
		.it_interval = {
			.tv_sec = 0,
			.tv_usec = 5
		},
		.it_value = {
			.tv_sec = 0, // should be some .got pointer, might be best checked at runtime
			.tv_usec = 0
		}
	};
	setitimer(0, &itv, 0);
}
void init_sigaction()
{
	//TODO
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
int init_uart(const char * tty_path) /* va_args - maybe / else done */
{
	const int fd = open(tty_path, 0x102);
	if (fd < 0) {
		printf("%s:open error!\n", tty_path);
		fprintf(stderr, "uart_open %s error\n", tty_path);
		perror("open:");
		return -3;
	}
	if (0 >= uart_set(fd, 1, 8, 0, 1)) {
		printf("%s:set error", tty_path);
		// missing \n
		fwrite("uart set failed!\n", 1, 0x11, stderr);
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
		return -1;
	}
	char volt_str[1000] = "";
	fgets(volt_str, 1000, iv);
	// this read should not return more than 32 bytes
	const size_t volt_len = strlen(volt_str);
	volt_str[volt_len - 1] = '\0';
	const int volt = (int) atof(volt_str);
	fclose(iv);
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
	const char pwm_p[] = "/sys/class/hwmon/hwmon1/pwm1";
	printf("set_PWM: %d\npwm: %d\n", 0, pwm);
	const int fd = open(pwm_p, O_WRONLY);
	if (fd < 1) {
		printf("set_ROC_RK3588S_PC_fan_pwm: Can not open %s file\n", pwm_p);
		// at this point you should be returning
		// instead you are writting and closing a invalid fd
	}
	char buf[0x18];
	sprintf(buf, "%d", pwm);
	write(fd, buf, strlen(buf));
	// good practice(and compiler warnings) would demand another fail check here
	// especially because this is a write which is likely to fail
	close(fd);
}
float roc_rk3588s_pc_average_temperature()
{
	int h14 = 0;
	float h18 = 0;
	FILE * temp_file = popen("cat /sys/class/thermal/thermal_zone*/temp", 0);
	if (temp_file == 0) {
		puts("no such file /sys/class/thermal/thermal_zone*/temp");
		return 50.0f; // 0x4248000 in IEEE-754
		// we don't have a read, either through a misconfigured kernel or handware issues
		// just returning 50 risks frying your board and leaves the issue undiscovered
	}
	char buf[1000];
	// this read will never return more than 32 chars
	while (1) {
		if (!fgets(buf, 1000, temp_file)) {
			if (h14 < 1) {
				// something
			}
			printf("sum = %f\n", h18);
			fclose(temp_file);
			return h18; // maybe
		}
		const int temp_l = strlen(buf);
		h18 = atof(buf); // some ops might still be here
		printf("%f\n", h18);
		// something more here
	}
}
void* roc_rk3588s_pc_fan_thread_daemon(void * arg)
{
	int x = 0;
	while (1) {
		while (x != 4) {
			usleep(50000);
			x++;
			// seams more practical to sleep 250000 ms once
			// although I might have missed something here
		}
		x = 0;
		const float temp = roc_rk3588s_pc_average_temperature() * 1000.0f; // 0x447a0000 in IEEE-754
		// potencially write to some thread shared pointer
	}
}
// ROC_RK3588_PC --------------------------------------------
void fan_ROC_RK3588_PC_init() /* done */
{
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", "r");
	// see comments on RK3588S init
}
// ----------------------------------------------------------
void fan_ITX_3588J_init() /* done */
{
	popen("echo 50 > /sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm1", "r");
	// see comments on RK3588S init
}
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
void fan_CS_R2_3399JD4_MAIN_init()
{
	int h4 = 0;
	while (h4 < 3) {
		//TODO
	}
	init_uart("/dev/ttyS0");
}
void set_CS_R1_3399JD4_MAIN_fan_pwm(char sth)
{
	// TODO:
}
void set_CS_R2_3399JD4_MAIN_fan_pwm(char * pwm, int sth) /* done */
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
void set_fan_ITX_3588J_fan_pwm(char pwm_ch)
{
	// TODO
}
void set_ROC_RK3588_PC_fan_pwm(char pwm_ch)
{
	// TODO
}
void* roc_rk3588_pc_fan_thread_daemon(void * arg)
{
	// TODO
}
void* cs_r1_3399jd4_main_fan_thread_daemon(void * arg)
{
	// TODO
}
void* fan_thread_rx(void * arg)
{
	// TODO
}
void* fan_thread_tx(void * arg)
{
	// TODO
}
void* itx_3588j_fan_thread_daemon(void * arg)
{
	// TODO
}
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
int fan_init()
{
	switch (board) {
		case CS_R2_3399JD4: // 1
			fan_CS_R2_3399JD4_MAIN_init();
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
	return 2;
}
void set_fan_pwm(char pwm_ch) /* done */
{
	global_pwm[0] = pwm_ch;
	switch (board) {
		case CS_R2_3399JD4: // 1
			set_CS_R2_3399JD4_MAIN_fan_pwm(sth_pwm, pwm_ch);
			break;
		case CS_R1_3399JD4: // 0
			set_CS_R1_3399JD4_MAIN_fan_pwm(pwm_ch);
			break;
		case ROC_RK3588S_PC: // 2
			set_ROC_RK3588S_PC_fan_pwm(pwm_ch);
			break;
		case ITX_3588J: // 3
			set_fan_ITX_3588J_fan_pwm(pwm_ch);
			break;
		case ROC_RK3588_PC: // 4
			set_ROC_RK3588_PC_fan_pwm(pwm_ch);
			break;
	}
}
int main(int argc/* 1ch */, char **argv /* str */)
{
	if (argc <= 1) {
		puts("./main CS-R1-3399JD4-MAIN 50");
		puts("./main CS-R2-3399JD4-MAIN --debug");
		puts("./main ROC-RK3588S-PC 50");
		puts("./main ITX_3588J 50");
		puts("./main ROC-RK3588-PC 50");
		// I know this might be more readable
		// but you are calling puts 5 times for what could be one write to stdout
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
	const int ch1 = fan_init();
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
			case CS_R1_3399JD4: // 0
				if (pthread_create(&t1, NULL, cs_r1_3399jd4_main_fan_thread_daemon, NULL) != 0) {
					puts("thread3 create error");
					return -1;
				}
				break;
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
	case CS_R1_3399JD4: // 0
		if (pthread_create(&t1, NULL, cs_r1_3399jd4_main_fan_thread_daemon, NULL) != 0) {
			puts("thread3 create error");
			return -1;
		}
		break;
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
	set_fan_pwm(0);
	init_sigaction();
	init_time();
	printf("pwm: %d\n", 0);
	// I guess here the actual fan control part is happening
}
