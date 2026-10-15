////Initially chopped up from https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/


#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

unsigned int read_serial_port();
unsigned int endian_convert();

int do_little_endian;

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

void poke_memory(unsigned int address, unsigned int value){
	set_address(address);
	char tmp[32];
	if(do_little_endian){
		snprintf(tmp,32,"at+efcs=%li\r\n",endian_convert(value));
	}else{
		snprintf(tmp,32,"at+efcs=%li\r\n",value);
	}
	//printf("debug: %s",tmp);
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
	printf("./poke_vx10k </dev/ttyACM0> <address> <value> <endian>\n");
	printf("Writes the four byte word <value> into <address> inside the phone's memory.\n<endian> sets endianness. 0=little, 1=big.\n");
	exit(0);
    }

    fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", argv[1], strerror(errno));
        return -1;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
	set_interface_attribs(fd, B115200);
	
	unsigned int poke_address=(unsigned int) 0xFFFFFFFF&strtoul(argv[2],NULL,16);
	unsigned int poke_value=(unsigned int) 0xFFFFFFFF&strtoul(argv[3],NULL,16);
	//do_little_endian=atoi(argv[4]);

	/*if((poke_address%4)!=0){
		printf("The address you supplied is not aligned to an even 4 byte value. Please check your arguments.\n");
		exit(0);
	}*/
	poke_memory(poke_address,poke_value);
}
