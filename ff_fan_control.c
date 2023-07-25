#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

int get_ROC_RK3588S_PC_version()
{
	FILE * iv = popen("cat /sys/bus/iio/devices/iio:device0/in_voltage5_raw", 0);
	// have you heard of read()?
	if (!iv) {
		puts("can not open /sys/bus/iio:device0/in_voltage5_raw");
		fclose(iv);
		return -1;
	}
	char volt_str[0x3e8];
	fgets(volt_str, 0x3e8, iv);
	const int volt = (int) atof(volt_str);
	fclose(iv);
	return volt <= 0x64 && volt >= 0 ? -1 :
	       volt < 0x79d || volt > 0x866 ? 0 :
	       1;
}
void fan_ROC_RK3588S_PC_init()
{
	popen("echo 50 > /sys/class/hwmon/hwmon1/pwm1", 0);
	// have you heard of write()?
	// note: closing FILE * is a good thing to do...
}
void set_ROC_RK3588S_PC_fan_pwm(int pwm)
{
	const char pwm_p[] = "/sys/class/hwmon/hwmon1/pwm1";
	printf("set_PWM: %d. pwm: %d.", 0, pwm);
	const int fd = open(pwm_p, O_WRONLY);
	if (fd < 1) {
		printf("set_ROC_RK3588S_PC_fan_pwm: Can not open %s file.", pwm_p);
		// at this point returning might be smart.....
	}
	char buf[0x18];
	sprintf(buf, "%d", pwm);
	write(fd, buf, strlen(buf));
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
	}
	char buf[0x3e8];
	while (1) {
		if (!fgets(buf, 0x3e8, temp_file)) {
			if (h14 < 1) {
				// something
			}
			printf("sum = %f.", h18);
			fclose(temp_file);
			return h18; // maybe
		}
		const int temp_l = strlen(buf);
		h18 = atof(buf); // some ops might still be here
		printf("%f.", h18);
		// something more here
	}
}
void roc_rk3588s_pc_fan_thread_daemon(void * arg)
{
	int x = 0;
	while (1) {
		// why again are we looping 4 times instead of just waiting 4 times as long?
		while (x != 4) {
			usleep(0x7a120);
			x++;
		}
		x = 0;
		const float temp = roc_rk3588s_pc_average_temperature() * 1000; // 0x447a0000 in IEEE-754
		// shouldn't you now be setting the pwm now?
	}
}
void PID_init(int s3c, float * x0)
{
	switch (s3c) {
	case 0: case 1: case 2: case 3: case 4:
		x0[0] = 2.0f;
		x0[1] = 0.12f; // 0x3c449ba6 in IEEE-754
		x0[2] = 1.0f;
		x0[3] = 48.0f; // 0x42400000 in IEEE-754
		x0[4] = 0.0f;
		x0[5] = 0.0f;
		x0[6] = 0.0f;
		x0[7] = 0.0f;
		x0[8] = 0.0f;
		x0[9] = 1.4f; //  0x000186a0 in IEEE-754
	}
}
void fan_init(int s3c)
{
	switch (s3c) {
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
	return;
}
void set_fan_pwm(int s3c, int pwm)
{
	switch (s3c) {
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
}
int main(int argc, char **argv)
{
	if (argc <= 1) {
		puts("./main CS-R1-3399JD4-MAIN --debug");
		puts("./main CS-R2-3399JD4-MAIN --debug");
		puts("./main ROC-RK3588S-PC 50");
		puts("./main ITX_3588J 50");
		puts("./main ROC-RK3588-PC 50 ");
		return 0;
	}
	
	int s3c;
	if (!strcmp("CS_R1-3399JD4-MAIN", argv[1])) {
		//puts("board CS_R1_3399JD4_MAIN");
		//s3c = 0;
	} else if (!strcmp("CS-R2-3399JD4-MAIN", argv[1])) {
		//puts("board CS_R2_3399JD4_MAIN");
		//s3c = 1;
	} else if (!strcmp("ROC-RK3588S-PC", argv[1])) {
		puts("board ROC-RK3588S-PC");
		s3c = 2;
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
		//s3c = 3;
	}
	float x0[10];
	PID_init(s3c, x0);
	fan_init(s3c);
	if (argc > 2) {
		if (!strcmp("--debug", argv[2])) {
			
		} else {
			const int pwm = atoi(argv[2]);
			// and w0, w0, 0xff
			set_fan_pwm(s3c, pwm);
			if (s3c) {
				// this seams fine.... don't mind the weird thread numbers
				// I didn't fake this
				pthread_t t1, t2, t3, t4, t5, t6;
				switch (s3c) {
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
	}
}
