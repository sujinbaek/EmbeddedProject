#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "lcd.c"

#define GPIO_PATH "/sys/class/gpio"
#define GPIO_LENGTH 10
#define BUFF_SIZE 100

void nomal_mode();
void tmp_mode();
void alarm_mode();
void ring_alarm();

int h1,m1;
char am[10];

int main(void)
{
//                       RS  E   D0  D1  D2  D3
        char gpio[10] = {47, 46, 23, 26, 45, 44,
//                               D4  D5  D6  D7
                                 69, 68, 66, 67};
        FILE *gpio_fp[10];
        FILE *tmp_fp;
        FILE *tmp; //temperature

        char buz = 7; //buzzer
        int buz_fd; //file descriptor
//                   mode
        char btn[3] = {65, 27, 22}; //push button
        FILE *btn_fp[3];

        char tmp_path[255];
        int i;

        struct tm *t;
        time_t timer;

        int flag[3]={0,};
        char val[3];
        for (i = 0; i < GPIO_LENGTH; i++)
       {
                tmp_fp = fopen(GPIO_PATH "/export", "w");
                fprintf(tmp_fp, "%d", gpio[i]);
                fclose(tmp_fp);

                sprintf(tmp_path, GPIO_PATH "/gpio%d/direction", gpio[i]);
                tmp_fp = fopen(tmp_path, "w");
                fprintf(tmp_fp, "out");
                fclose(tmp_fp);

                sprintf(tmp_path, GPIO_PATH "/gpio%d/value", gpio[i]);
                gpio_fp[i] = fopen(tmp_path, "w");
        }

        tmp_fp = fopen(GPIO_PATH "/export", "w");
        fprintf(tmp_fp, "%d", buz);
        fclose(tmp_fp);

        sprintf(tmp_path, GPIO_PATH "/gpio%d/direction", buz);
        tmp_fp = fopen(tmp_path, "w");
        fprintf(tmp_fp, "out");
        fclose(tmp_fp);

        sprintf(tmp_path, GPIO_PATH "/gpio%d/value", buz);
        buz_fd = open(tmp_path, O_WRONLY);

        for(i=0; i<3; i++) {
                tmp_fp = fopen(GPIO_PATH "/export", "w");
                fprintf(tmp_fp, "%d", btn[i]);
                fclose(tmp_fp);

                sprintf(tmp_path, GPIO_PATH "/gpio%d/direction", btn[i]);
                tmp_fp = fopen(tmp_path, "w");
                fprintf(tmp_fp, "in");
                fclose(tmp_fp);
        }

        tmp = fopen("/sys/devices/bone_capemgr.9/slots", "w");
        fprintf(tmp, "%s", "BB-1WIRE-P9-22");
        fclose(tmp);

        // LCD INITIALIZATION
        lcd_init(gpio_fp);
        // BUZZER INITIALIZATION
        write(buz_fd, "1", 1);
        // ALARM INITIALIZATION
        timer = time(NULL);
        t = localtime(&timer);
        h1 = t->tm_hour-1;
        m1 = t->tm_min+2;
        if (h1>12){
                strcpy(am,"PM");
                h1 -= 12;
        }
        else
                strcpy(am,"AM");

        while (1) {
                timer = time(NULL);
                t = localtime(&timer);

                for(i=0; i<3; i++) {
                        sprintf(tmp_path, GPIO_PATH "/gpio%d/value", btn[i]);
                        btn_fp[i] = fopen(tmp_path, "r");
                        val[i] = fgetc(btn_fp[i]);
                        usleep(50000);
                        if (val[i] == '1') {
                                if(i==2) flag[i]=1;
                                else {
                                        flag[i] = (flag[i]+1)%3;
                                        lcd_init(gpio_fp);
                                }
                        }
                        else {if(i==2) flag[i]=0;}
                        fclose(btn_fp[i]);
                }

                switch(flag[0]) {
                case 0:
                        nomal_mode(t);
                        break;
                case 1:
                        tmp_mode();
                        break;
                case 2:
                        alarm_mode(t, flag[1], flag[2]);
                        break;
                }

                ring_alarm(t, buz_fd, h1, m1);
        }

        tmp_fp = fopen(GPIO_PATH "/unexport", "w");
        for (i = 0; i < GPIO_LENGTH; i++)
        {
                fclose(gpio_fp[i]);
                fprintf(tmp_fp, "%d", gpio[i]);
        }
        for (i=0; i<3; i++) {
                fclose(btn_fp[i]);
                fprintf(tmp_fp, "%d", btn[i]);
        }
        close(buz_fd);
        fprintf(tmp_fp, "%d", buz);
        fclose(tmp_fp);
}

void nomal_mode(struct tm *t) {
        char line1[BUFF_SIZE], line2[BUFF_SIZE];
       char* week[7]={"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

        sprintf(line1, "%6d %d %d %s", t->tm_year+1900, t->tm_mon+1, t->tm_mday, week[t->tm_wday]);
        // WRITE FIRST LINE
        lcd_send(FIRST_LINE, CMD_MODE);
        lcd_write(line1);

        strftime(line2, BUFF_SIZE, "%4p %2I:%2M:%2S", t);
        // WRITE SECOND LINE
        lcd_send(SECOND_LINE, CMD_MODE);
        lcd_write(line2);
}

void tmp_mode() {
        char buff[BUFF_SIZE];
        char *token = NULL;
        int tmp_fd; //temperature fd
        int i;

        lcd_send(FIRST_LINE, CMD_MODE);
        lcd_write("Current");
        if((tmp_fd=open("/sys/devices/w1_bus_master1/28-01145447f7ff/w1_slave", O_RDONLY)) > 0) {
                read(tmp_fd, buff, 1024);
                read(tmp_fd, buff, 1024);

                token = strtok(buff, "\n");

                token = strtok(NULL, "=");
                token = strtok(NULL, "\n");

                for (i=strlen(token)+1; i>strlen(token)-4; i--)
                        token[i] = token[i-1];
                token[i] = '.';

                strcat(token, " degrees.");
                lcd_send(SECOND_LINE, CMD_MODE);
                lcd_write(token);
        }
        close(tmp_fd);
}

void alarm_mode(struct tm *t, int flag1, int flag2) {
        char line1[BUFF_SIZE], line2[BUFF_SIZE];
        char* week[7]={"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

        sprintf(line1, "%6d %d %d %s", t->tm_year+1900, t->tm_mon+1, t->tm_mday, week[t->tm_wday]);
        // WRITE FIRST LINE
        lcd_send(FIRST_LINE, CMD_MODE);
        lcd_write(line1);

        sprintf(line2, "%4s %2d:%2d    ", am, h1, m1);
        // WRITE SECOND LINE
        lcd_send(SECOND_LINE, CMD_MODE);
        lcd_write(line2);

        usleep(250000);

        switch(flag1) {
                case 0:
                        sprintf(line2, "     %2d:%2d    ", h1, m1);
                        break;
                case 1:
                       h1 += flag2;
                        sprintf(line2, "%4s   :%2d   ", am, m1);
                        break;
                case 2:
                        m1 += flag2;
                        m1 = m1%60;
                        sprintf(line2, "%4s %2d:      ", am, h1);
                        break;
        }
        // WRITE SECOND LINE
        lcd_send(SECOND_LINE, CMD_MODE);
        lcd_write(line2);

        usleep(250000);
}

void ring_alarm(struct tm *t, int fd, int h, int m) {
        t->tm_hour -= (t->tm_hour > 12)?12:0;
        if (t->tm_hour == h && t->tm_min == m) {
                write(fd, "0", 1);
                usleep(250000);
                write(fd, "1", 1);
                usleep(100000);
                write(fd, "0", 1);
                usleep(250000);
                write(fd, "1", 1);
                usleep(500000);
        }
}
