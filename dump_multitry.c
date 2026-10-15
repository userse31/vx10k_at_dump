//Initially chopped up from https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

//Alright "tcdrain(fd)" is causing some problems. It seems to be speed related as the reading progress "jams" and stops progressing. I'm going to use some of that good ol' usleep().
#define USLEEP_DELAY 0

//#define DEBUG_COMMON_SORT 0

//#define TRIES 7
#define TRIES 7

//So we can have atleast SOME form of progress indication.
#define FLUSH_AFTER_AMOUNT 16

unsigned int read_serial_port();
unsigned int endian_convert();

int fd;
int wlen;

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */
    if (tcsetattr(fd, TCSANOW, &tty) < 0){
        printf("Error tcsetattr: %s\n", strerror(errno));
    }
}

void disable_echo(){
	wlen=write(fd, "ate0\r\n",6);
	//tcdrain(fd);
}

void read_address(){
	wlen=write(fd, "at+ilrr?\r\n",10);
	//tcdrain(fd);
}

void set_address(unsigned int address){
	char tmp[32];
	snprintf(tmp,32,"at+ilrr=%i\r\n", address);
	wlen=write(fd, tmp, 32);
	//tcdrain(fd);
}

unsigned int read_dword(){
	wlen=write(fd, "at+efcs?\r\n", 10);
	//tcdrain(fd);
	return endian_convert(read_serial_port());
}

unsigned int read_dword_no_endian_swap(){
	wlen=write(fd, "at+efcs?\r\n", 10);
	//tcdrain(fd);
	return read_serial_port();
}

/*
"Thinking in JavaScript is always faster then reloading." -Gaz

function is_in_array(array, value){
	var is_in_array=0;
	for(var i=0;i<array.length;i++){
		if(array[i]==value){
			is_in_array=1;
		}
	}
	return is_in_array;
}

function count_in_array(array,value){
	var count=0;
	for(var i=0;i<array.length;i++){
		if(array[i]==value){
			count++;
		}
	}
	return count;
}

var initial_values=[1,1,2,2,3];
var uniques=[];
uniques.push(initial_values[0]);

var is_unique;
for(var i=0;i<initial_values.length;i++){
	is_unique=1;
	if(is_in_array(uniques,initial_values[i])){
		0;
	}else{
		uniques.push(initial_values[i]);
	}
}

var uniques_freq=new Array(uniques.length);
for(var i=0;i<uniques_freq.length;i++){
	uniques_freq[i]=count_in_array(initial_values,uniques[i]);
}
console.log(initial_values);
console.log(uniques);
console.log(uniques_freq);

var largest_value_i;
var tmp=0;
for(var i=0;i<uniques_freq.length;i++){
	if(tmp<uniques_freq[i]){
		largest_value_i=i;
		tmp=uniques_freq[i];
	}
}
console.log(uniques[largest_value_i]);

*/

int is_in_array(unsigned int *array, unsigned int value,int array_length){
	int is_in_array=0;
	for(int i=0;i<array_length;i++){
		if(array[i]==value){
			is_in_array=1;
		}
	}
	return is_in_array;
}

int count_in_array(unsigned int *array, unsigned int value,int array_length){
	int count=0;
	for(int i=0;i<array_length;i++){
		if(array[i]==value){
			count++;
		}
	}
	return count;
}

unsigned int read_dword_no_endian_swap_multi(){
	unsigned int initial_values[TRIES];
	int initial_values_length=TRIES;
	unsigned int uniques[TRIES];
	int uniques_length;
	//Get our batch of reads.
	for(int i=0;i<TRIES;i++){
		initial_values[i]=read_dword_no_endian_swap();
	}
	uniques[0]=initial_values[0];
	uniques_length=1;
	//Remove duplicates.
	int is_unique;
	for(int i=0;i<initial_values_length;i++){
		is_unique=1;
		if(is_in_array(uniques,initial_values[i],uniques_length)){
			0;
		}else{
			uniques[uniques_length]=initial_values[i];
			uniques_length++;
		}
	}

	int uniques_freq[uniques_length];
	int uniques_freq_length=uniques_length;

	for(int i=0;i<uniques_freq_length;i++){
		uniques_freq[i]=count_in_array(initial_values,uniques[i],initial_values_length);
	}

	int largest_value_i;
	int tmp=0;
	for(int i=0;i<uniques_freq_length;i++){
		if(tmp<uniques_freq[i]){
			largest_value_i=i;
			tmp=uniques_freq[i];
		}
	}
	
#ifdef DEBUG_COMMON_SORT
	printf("initial_values: ");
	for(int i=0;i<initial_values_length;i++){
		printf("%08x,",initial_values[i]);
	}
	printf("\nuniques:");
	for(int i=0;i<uniques_length;i++){
		printf("%08x,",uniques[i]);
	}
	printf("\nuniques freq:");
	for(int i=0;i<uniques_freq_length;i++){
		printf("%i,",uniques_freq[i]);
	}
	printf("\nMost Frequent: $%08x\n",uniques[largest_value_i]);

#endif
	return uniques[largest_value_i];
}

unsigned int read_serial_port(){
	//There is some weird god awful memory corruption fuckery going on around here and I do NOT like it!
     	unsigned char buf[80];
	char atoi_buffer[80];
        int rdlen;
	
	for(int i=0;i<80;i++){
		buf[i]=0;
		atoi_buffer[i]=0;
	}
	read(fd,buf,16);//First read clears the echo garbage we don't care for.
        rdlen = read(fd, buf, 32);
	if(rdlen>0){
		//printf("Read: >%s<\n",buf);
	}
	int j=0;
	for(int i=0;i<80;i++){
		if(buf[i]=='0'||buf[i]=='1'||buf[i]=='2'||buf[i]=='3'||buf[i]=='4'||buf[i]=='5'||buf[i]=='6'||buf[i]=='7'||buf[i]=='8'||buf[i]=='9'){
			atoi_buffer[j]=buf[i];
			j++;
		}
	}
	//printf("atoi_buffer: %s\n",atoi_buffer);
	//printf("atoi: %u\n",atoi(atoi_buffer));
	return atoi(atoi_buffer);
}

unsigned int endian_convert(unsigned int x){
	//We will convert your endianness!
	//Needed as the values being read back from the phone are being switched because _computer science stuff._
	unsigned int tmp=0;
	tmp=tmp|((x&0x000000FF)<<24);
	tmp=tmp|(((x&0x0000FF00)>>8)<<16);
	tmp=tmp|(((x&0x00FF0000)>>16)<<8);
	tmp=tmp|(((x&0xFF000000)>>24));
	//I accidentally forgot to add this return, yet this bit of code still worked. That's not horrifying!
	return tmp;
}

int main(int argc, char *argv[]){
    if(argc<5){
	printf("./dump </dev/ttyACM0> <starting address (hex)> <ending address (hex)> <output style>\n");
	printf("Output Styles:\n0: $<address>: $<4 bytes>\n1: Raw bytes output to stdout\n2: <bytes>\nAll formatted outputs are in hex.\n");
	exit(0);
    }

    fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", argv[1], strerror(errno));
        return -1;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
	set_interface_attribs(fd, B115200);
	
	unsigned int starting_address=(unsigned int) 0xFFFFFFFF&strtoul(argv[2],NULL,16);
	unsigned int ending_address=(unsigned int) 0xFFFFFFFF&strtoul(argv[3],NULL,16);

	if(ending_address<starting_address){
		printf("Ending address is below starting address. Please check your arguments.\n");
		exit(0);
	}
	if(((starting_address%4)!=0)||((ending_address%4)!=0)){
		printf("The starting and/or ending addresses you supplied are not aligned to even 4 byte values. Please check your arguments.\n");
		exit(0);
	}

	unsigned int current_address=0;
	unsigned int memory_read=0;
	int output_style=atoi(argv[4]);
	int j=0;

	if(output_style==0){
		for(unsigned int i=starting_address;i<ending_address;i+=4){
			set_address(i);
			read_serial_port();
			memory_read=read_dword_no_endian_swap_multi();
			memory_read=endian_convert(memory_read);
			printf("$%08x: $%08x\n", i, memory_read);
			usleep(USLEEP_DELAY);
			j++;
			if(j==FLUSH_AFTER_AMOUNT){
				fflush(stdout);
				j=0;
			}
		}
	}
	if(output_style==1){
		unsigned int tmp[TRIES];
		int tmp_freq[TRIES];
		for(unsigned int i=starting_address;i<ending_address;i+=4){
			set_address(i);
			read_serial_port();
			//Read the same address multiple times.
			tmp[0]=read_dword_no_endian_swap_multi();
#ifndef DEBUG_COMMON_SORT
			fwrite(tmp,sizeof(unsigned int),1,stdout);
#endif
			tcdrain(fd);
			j++;
			if(j==FLUSH_AFTER_AMOUNT){
				fflush(stdout);
				j=0;
			}
		}
	}
	if(output_style==2){
		for(unsigned int i=starting_address;i<ending_address;i+=4){
			set_address(i);
			read_serial_port();
			memory_read=read_dword_no_endian_swap_multi();
			memory_read=endian_convert(memory_read);
			printf("%08x", memory_read);
			usleep(USLEEP_DELAY);
			j++;
			if(j==FLUSH_AFTER_AMOUNT){
				fflush(stdout);
				j=0;
			}		}
	}

	/*unsigned int memory_read=0;
	set_address((unsigned int)0x00000000);
	read_serial_port();
	read_address();
	read_serial_port();
	memory_read=read_dword();
	printf("$00000000: $%08x\n", memory_read);
	set_address((unsigned int)0x00000004);
	read_serial_port();
	memory_read=read_dword();
	printf("$00000004: $%08x\n", memory_read);*/
}
