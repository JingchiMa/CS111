#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <mraa.h>
#include <mraa/aio.h>

#define BUFSIZE 128

char* USAGE = "Usage: valid options are --threads and --iterations. Default values are 1\n";
const int B = 4275; // B value for thermistor value
const int R0 = 100 * 1000;
int period_flag;
int scale_flag;
int log_flag;
char * log_name;
FILE * log_file = NULL;
int period = 1;
char unit = 'F';
int paused = 0;
int dummy = 0;
mraa_aio_context temp_sensor;
mraa_gpio_context button;

void err_exit(char* err) {
    perror(err);
    exit(1);
}
void cleanup() {
    if (dummy) {
        return;
    }
    mraa_aio_close(temp_sensor);
    mraa_gpio_close(button);
    mraa_deinit();
}

int mraa_aio_read_wrapper() {
    if (dummy) {
        return 100;
    } else {
        return mraa_aio_read(temp_sensor);
    }
}
int mraa_gpio_read_wrapper() {
    if (dummy) {
        return 0;
    } else {
        return mraa_gpio_read(button);
    }
}
int mraa_aio_init_wrapper() {
    if (dummy) {
        return 1;
    } else {
        return mraa_aio_init(1);
    }
}
int mraa_gpio_init_wrapper() {
    if (dummy) {
        return 1;
    } else {
        return mraa_gpio_init(62);
    }
}
void mraa_gpio_dir_wrapper() {
    if (dummy) {
        return;
    } else {
        mraa_gpio_dir(button, MRAA_GPIO_IN);
    }
}


double get_temperature() {
    int raw = mraa_aio_read_wrapper();
    double temperature = 1023.0 / (raw * 1.0) - 1;
    temperature *= R0;
    double temp_C = 1.0 / (log(temperature / R0) / B + 1 / 298.15) - 273.15;
    return unit == 'C' ? temp_C : temp_C * 9 / 5 + 32;
}

void print_command(const char* cmd) {
    if (log_flag) {
        fprintf(log_file, "%s\n", cmd);
    }
}

void board_shutdown() {
    time_t time_data;
    struct tm* cur;
    time(&time_data);
    cur = localtime(&time_data);
    fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", cur->tm_hour, cur->tm_min, cur->tm_sec);
    if(log_flag) {
        fprintf(log_file, "%.2d:%.2d:%.2d SHUTDOWN\n", cur->tm_hour, cur->tm_min, cur->tm_sec);
    }
    exit(0);
}

void handle_log(const char* cmd) {
    if (log_flag) {
        fprintf(log_file, "%s\n", cmd);
    }
}

// returns if the set period function succeeds or not
int setPeriod(const char* cmd) {
    char str[] = "PERIOD=";
    int i = 0;
    while (cmd[i] != '\0' && str[i] != '\0') {
        if (cmd[i] != str[i])
            return 0; // different character
        i++;
    }
    if (i != 7) {
        return 0; // cmd is not long enough
    }
    // here we are sure that cmd has "PERIOD=" as prefix
    int input_period = 0;
    while (cmd[i] != '\0') {
        if (!isdigit(cmd[i])) {
            return 0;
        }
        input_period = input_period * 10 + (cmd[i] - '0');
    }
    period = input_period;
    return 1;
}

void handle_command(const char* cmd) {
    if (cmd == NULL) {
        err_exit("NULL command received");
    }
    if (strcmp(cmd, "OFF\n") == 0){
        board_shutdown();
    } else if (strcmp(cmd, "STOP\n") == 0) {
        paused = 1;
    } else if (strcmp(cmd, "START\n") == 0) {
        paused = 0;
    } else if (strcmp(cmd, "SCALE=F\n") == 0) {
        unit = 'F';
    } else if (strcmp(cmd, "SCALE=C\n") == 0) {
        unit = 'C';
    } else if ((strncmp(cmd, "LOG", sizeof(char) * 3) == 0)) {
        handle_log(cmd);
    } else {
        if (!setPeriod(cmd)) {
            print_command("error: invalid command\n");
            exit(1);
        }
    }
    print_command(cmd);
}

void init_devices() {
    temp_sensor = mraa_aio_init_wrapper();
    if (temp_sensor == NULL) {
        err_exit("temperature sensor not initialized");
    }
    button = mraa_gpio_init_wrapper();
    if (button == NULL) {
        err_exit("button not initialized");
    }
    mraa_gpio_dir_wrapper();
}

void log_temperature(double temp) {
    time_t time_data;
    struct tm* cur;
    time(&time_data);
    cur = localtime(&time_data);
    fprintf(stdout, "%.2d:%.2d:%.2d %.1f\n", cur->tm_hour, cur->tm_min, cur->tm_sec, temp);
    if(log_flag) {
        fprintf(log_file, "%.2d:%.2d:%.2d %.1f\n", cur->tm_hour, cur->tm_min, cur->tm_sec, temp);
    }
}

int main(int argc, char **argv) {
    atexit(cleanup);
    static struct option longopts[] = {
        {"period", required_argument, NULL, 'p'},
        {"scale",  required_argument, NULL, 's'},
        {"log",    required_argument, NULL, 'l'},
        {"dummy",  no_argument,       NULL, 'd'},
        {0,        0,           0,            0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "p:s:l:", longopts, NULL)) != -1) {
        switch(c) {
            case 'd':
                dummy = 1;
                break;
            case 'p':
                period = atoi(optarg);
                break;
            case 's':
                if (optarg[0] == 'c' || optarg[0] == 'C') {
                    unit = 'C';
                } else if (optarg[0] == 'f' || optarg[0] == 'F'){
                    unit = 'F';
                } else {
                    err_exit("Only C and F are allowed for temperature unit");
                }
                break;
            case 'l':
                log_flag = 1;
                log_name = optarg;
                log_file = fopen(log_name, "w");
                if (log_file == NULL){
                    err_exit("cannot open log file");
                }
                break;
            case '?':
                err_exit(USAGE);
                break;
        }
    }
   
    /* initializations */
    init_devices();
    struct pollfd pollfds[1];
    pollfds[0].fd = STDIN_FILENO;
    pollfds[0].events = POLLIN | POLLHUP | POLLERR;
    char buf[BUFSIZE];     // buffer input from poll
    char command[BUFSIZE]; // contains single command
    memset(command, 0, BUFSIZE);
    int cmdIdx = 0;        // index in command buffer
    time_t cur, next;
    time(&cur);
    next = cur + period;
    
    while (1) {
        if (mraa_gpio_read_wrapper()) {
            board_shutdown();
        }
        time(&cur);
        double temperature = get_temperature();
        if (!paused && cur >= next){
            next = cur + period;
            log_temperature(temperature);
        }
        int res = poll(pollfds, 1, 0);
        if (res < 0){
            err_exit("error in polling");
        }
        if (pollfds[0].revents & POLLIN){
            int num = (int) read(STDIN_FILENO, buf, BUFSIZE);
            if (num < 0) {
                err_exit("read error");
            }
            int i;
            // note that command length cannot exceed BUFSIZE.
            for (i = 0; i < num && cmdIdx < BUFSIZE; i++) {
                command[cmdIdx] = buf[i];
                if (command[i] == '\n') {
                    handle_command(command);
                    /* reset command buffer */
                    cmdIdx = 0;
                    memset(command, 0, BUFSIZE);
                } else {
                    cmdIdx++;
                }
            }
            if (cmdIdx >= BUFSIZE) {
                cmdIdx = 0; // discard input in command buf because command length exceeds BUFSIZE
            }
        }
    }
}

