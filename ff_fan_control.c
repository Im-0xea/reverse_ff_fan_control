#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


int board;
int PID_fan;

int get_ROC_RK3588S_PC_version()
{
	FILE * iv = popen("cat /sys/bus/iio/devices/iio:device0/in_voltage5_raw", 0);
	// the proper C way would be using open() and read()
	if (!iv) {
		puts("can not open /sys/bus/iio:device0/in_voltage5_raw");
		fclose(iv);
		return -1;
	}
	char volt_str[1000];
	fgets(volt_str, 1000, iv);
	// this read should not return more than 32 bytes
	const int volt = (int) atof(volt_str);
	fclose(iv);
	return volt <= 0x64 && volt >= 0 ? -1 :
	       volt < 0x79d || volt > 0x866 ? 0 :
	       1;
}
void fan_ROC_RK3588S_PC_init()
{
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", 0);
	// if you don't want to do this with open() and write()
	// you should use system()
	// popen() just leaked a fd
}
void set_ROC_RK3588S_PC_fan_pwm(int pwm)
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
void PID_init(int PID_fan /* x0 */, float x0[])
{
	// 0x3c449ba6 in IEEE-754 0.12f
	// 0x42400000 in IEEE-754 48.0f
	// 0x000186a0 in IEEE-754 1.4f
	switch (board) {
	case 0:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	case 1:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	case 2:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	case 3:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	case 4:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
		break;
	}
	// these happen to all be the same
	// also making a function out of the initilization of a float array
	// seams a bit off
}
int fan_init()
{
	switch (board) {
		case 0:
			//fan_CS_R1_3399JD4_MAIN_init();
			break;
		case 1:
			//fan_CS_R2_3399JD4_MAIN_init();
			break;
		case 2:
			fan_ROC_RK3588S_PC_init();
			break;
		case 3:
			//fan_ITX_3588J_init();
			break;
		case 4:
			//fan_ROC_RK3588_PC_init();
			break;
	}
	sleep(2);
	return 2;
}
int set_fan_pwm(int pwm)
{
	switch (board) {
		case 0:
			//set_CS_R1_3399JD4_MAIN_fan_pwm(pwm);
			break;
		case 1:
			//set_CS_R2_3399JD4_MAIN_fan_pwm(pwm);
			break;
		case 2:
			set_ROC_RK3588S_PC_fan_pwm(pwm);
			break;
		case 3:
			//set_fan_ITX_3588J_fan_pwm(pwm);
			break;
		case 4:
			//set_ROC_RK3588_PC_fan_pwm(pwm);
			break;
	}
	return 2;
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
	
	if (!strcmp("CS_R1-3399JD4-MAIN", argv[1])) {
		//puts("board CS_R1_3399JD4_MAIN");
		//board = 0;
	} else if (!strcmp("CS-R2-3399JD4-MAIN", argv[1])) {
		//puts("board CS_R2_3399JD4_MAIN");
		//board = 1;
	} else if (!strcmp("ROC-RK3588S-PC", argv[1])) {
		puts("board ROC-RK3588S-PC");
		board = 2;
		const int RK3588S_V = get_ROC_RK3588S_PC_version();
		if (RK3588S_V == -1) {
			puts("can not judge ROC-RK3588S-PC version");
			return -1;
		} else if (!RK3588S_V) {
			puts("board ROC-RK3588 S-PC VERSION v0.1");
		} else if (RK3588S_V == 1) {
			puts("board ROC-RK3588 S-PC VERSION v1.X");
		}
	} else if (!strcmp("ITX_3588J 50", argv[1])) {
		//puts("board ITX-3588J");
		//board = 3;
	}
	// if you didn't change the model formats
	// you could have just made the models to strings
	// and you could have done most of this in one place
	// it would also be smart to error out if no model is specified
	float x0[10];
	PID_init(PID_fan, x0);
	const int ch1 = fan_init();
	pthread_t t1, t2, t3, t4, t5, t6;
	if (argc > 2) {
		// this is not proper argument parsing
		// it is better to either use a plain loop or getopts
		if (strcmp("--debug", argv[2])) {
			const int pwm = atoi(argv[2]);
			// and w0, w0, 0xff
			set_fan_pwm(pwm);
			// those "threadN create error" messages don't make any sense
			// and don't give any indication were the issue is
			// also you could have used 1 or 2 pthreads
			// you have 6 of which at max 2 are used
			switch (board) {
			case 0:
				//if (pthread_create(&t1, NULL, cs_r1_3399jd4_main_fan_thread_daemon, NULL) != 0) {
				//	puts("thread3 create error");
				//	return -1;
				//}
				break;
			case 1:
				//if (pthread_create(&t2, NULL, fan_thread_tx, NULL) != 0) {
				//	puts("thread1 create error");
				//	return -1;
				//}
				//if (pthread_create(&t3, NULL, fan_thread_rx, NULL) != 0) {
				//	puts("thread2 create error");
				//	return -1;
				//}
				break;
			case 2:
				if (pthread_create(&t4, NULL, roc_rk3588s_pc_fan_thread_daemon, NULL) != 0) {
					puts("thread4 create error");
					return -1;
				}
				break;
			case 3:
				//if (pthread_create(&t5, NULL, itx_3588j_fan_thread_daemon, NULL) != 0) {
				//	puts("thread5 create error");
				//	return -1;
				//}
				//break;
			case 4:
				//if (pthread_create(&t6, NULL, roc_rk3588_pc_fan_thread_daemon, NULL) != 0) {
				//	puts("thread4 create error");
				//	return -1;
				//}
				//break;
			}
			while (1) sleep(1);
		}
	}
	switch (board) {
	case 0:
		//if (pthread_create(&t1, NULL, cs_r1_3399jd4_main_fan_thread_daemon, NULL) != 0) {
		//	puts("thread3 create error");
		//	return -1;
		//}
		break;
	case 1:
		//if (pthread_create(&t2, NULL, fan_thread_tx, NULL) != 0) {
		//	puts("thread1 create error");
		//	return -1;
		//}
		//if (pthread_create(&t3, NULL, fan_thread_rx, NULL) != 0) {
		//	puts("thread2 create error");
		//	return -1;
		//}
		break;
	case 2:
		if (pthread_create(&t4, NULL, roc_rk3588s_pc_fan_thread_daemon, NULL) != 0) {
			puts("thread4 create error");
			return -1;
		}
		break;
	case 3:
		//if (pthread_create(&t5, NULL, itx_3588j_fan_thread_daemon, NULL) != 0) {
		//	puts("thread5 create error");
		//	return -1;
		//}
		//break;
	case 4:
		//if (pthread_create(&t6, NULL, roc_rk3588_pc_fan_thread_daemon, NULL) != 0) {
		//	puts("thread4 create error");
		//	return -1;
		//}
		//break;
	}
	set_fan_pwm(0);
	//init_sigaction()
	//init_time()
	printf("pwm: %d\n", 0);
	// I guess here the actual fan control part is happening
}
