#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

int get_ROC_RK3588S_PC_version()
{
	FILE * iv = popen("cat /sys/bus/iio/devices/iio:device0/in_voltage5_raw", 0);
	// the proper C way would be using open() and read()
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
	// if you don't want to do this with a fd and write()
	// which would have been the proper way
	// you should use system()
	// popen just leaked a fd
}
void set_ROC_RK3588S_PC_fan_pwm(int pwm)
{
	const char pwm_p[] = "/sys/class/hwmon/hwmon1/pwm1";
	printf("set_PWM: %d. pwm: %d.", 0, pwm);
	const int fd = open(pwm_p, O_WRONLY);
	if (fd < 1) {
		printf("set_ROC_RK3588S_PC_fan_pwm: Can not open %s file.", pwm_p);
		// at this point you should be returning
		// you are writting and closing a invalid fd
	}
	char buf[0x18];
	sprintf(buf, "%d", pwm);
	write(fd, buf, strlen(buf));
	// good practice would demand another fail check here
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
		// we don't know the temperature
		// handle the error!, don't just return 50 degrees
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
void* roc_rk3588s_pc_fan_thread_daemon(void * arg)
{
	int x = 0;
	while (1) {
		while (x != 4) {
			usleep(0x7a120);
			x++;
			// why didn't you just usleep 5 times longer?
		}
		x = 0;
		const float temp = roc_rk3588s_pc_average_temperature() * 1000.0f; // 0x447a0000 in IEEE-754
		// shouldn't you be setting the pwm now?
		// does this daemon even do anything besides outputing the temperature?
	}
}
void PID_init(int s3c, float x0[])
{
	switch (s3c) {
	case 0:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
	case 1:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
	case 2:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
	case 3:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
	case 4:
		x0[0]=2.0f;x0[1]=0.12f;x0[2]=1.0f;x0[3]=48.0f;x0[4]=0.0f;
		x0[5]=0.0f;x0[6]=0.0f; x0[7]=0.0f;x0[8]=0.0f; x0[9]=1.4f;
	}
	// these happen to all be the same...
	// also why did you functionise the initilization of a float array?
	// 0x3c449ba6 in IEEE-754 0.12f
	// 0x42400000 in IEEE-754 48.0f
	// 0x000186a0 in IEEE-754 1.4f
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
		// I know this is more readable
		// but you are calling puts 5 times
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
	// if you didn't change the MODEL format for no reason
	// you could have just made the model a string and saved yourselfs all of this
	float x0[10];
	PID_init(s3c, x0);
	fan_init(s3c);
	pthread_t t1, t2, t3, t4, t5, t6;
	if (argc > 2) {
		// this is not how professional argument parsing looks like
		if (strcmp("--debug", argv[2])) {
			const int pwm = atoi(argv[2]);
			// and w0, w0, 0xff
			set_fan_pwm(s3c, pwm);
			// those threadN create error messages don't make any sense
			// also you could have used 1 or 2 pthreads, you have 6 of which at max 2 are used
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
			return 0;
		}
	}
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
	set_fan_pwm(s3c, 0);
	//init_sigaction()
	//init_time()
	printf("pwm: %d.", 0);
	// I guess here the actual fan control part is happening
}
