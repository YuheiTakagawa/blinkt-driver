#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char *argv[]){
	char path[256];
	char num;
	char count;
	int fd;
	char rc;
	char wc;
	int ret;
	char wc2;
	if(argc < 4){
		printf("usage: ./ledsw <device file> <num> <count>\n");
		return 1;
	}
	strncpy(path,  argv[1], 256);
	num = atoi(argv[2]);
	count = atoi(argv[3]);
//	printf("device: %s\nnum: %d\ncount: %d\n", path, num, count);

	fd = open(path, O_RDWR);
	if (fd < 0){
		perror("open");
		return 1;
	}


	ret = read(fd, &rc, sizeof(rc));
	if(ret < 0) {
		perror("read");
		return 1;
	}

	if (count == 0) {
		printf("0x%x\n", rc);
		return 0;
	}


	/*
	for(int i = 0; i < count; i++){
		flock(fd, LOCK_EX);
		ret = read(fd, &rc, sizeof(rc));
		if(ret < 0) {
			perror("read");
			return 1;
		}
		wc = 1 << (num-1);
		wc = rc ^ wc;
		write(fd, &wc, sizeof(wc));
		flock(fd, LOCK_UN);
		usleep(80000);
	}
	*/

	for(int i = 0; i < count; i++){
		flock(fd, LOCK_EX);
		wc = num & 1;
		write(fd, &wc, sizeof(wc));
		flock(fd, LOCK_UN);
		usleep(80000);
	}

	close(fd);

	
	return 0;
}
