#ifndef packet_h
#define packet_h

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <fstream>
#include <iostream>
#include <fcntl.h>

using namespace std;

#define WINDOW_SIZE 10

#define DEBUG printf("%d\n", __LINE__)

typedef struct {
	char data;
}Packet;

typedef enum {
	fr_data, ack
}frame_type;

typedef struct {
	frame_type fr_type = fr_data;
	uint8_t seq = WINDOW_SIZE;
	Packet info;
	bool valid = false;
	bool last = false;
}Frame;


const char * clin = "channel_l_in";
const char * clout = "channel_l_out";
const char * crin = "channel_r_in";
const char * crout = "channel_r_out";

#endif