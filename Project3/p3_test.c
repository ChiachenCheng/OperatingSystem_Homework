#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#define Concurrency 20 
#define Blocksize 65536
#define filesize (200 * 1024 * 1024)
#define maxline (2 * 1024 * 1024)
#define S_TO_MUS 1000000 

char examtext[maxline] = "abcdefghijklmnopqrstuvwxyz";
struct timeval starttime, endtime, spendtimeSpec;
char testtext[maxline];
char filepath[100];
char x[5]; 

void write_file(int blocksize, bool isrand, char *filepath, int fs)
{
	int fd;
  
	fd=open(filepath,O_WRONLY|O_CREAT|O_SYNC,0666);
	if(fd<0){
		perror("\nWrite Error: cannot open file! \n");
		return;
	}
    
    for(int i=1;i<=fs/blocksize;i++){
    	int z=((rand()*RAND_MAX)+rand())%fs;
		if(isrand){
			lseek(fd, z, SEEK_SET);
		}
		
		int rsize=write(fd,examtext,blocksize);
		if(rsize<0){
			perror("\nWrite Error: return value of write < 0 ! \n");
			return;
		}
		else if(rsize==0){
			rsize=write(fd,examtext,blocksize);
			if(rsize!=blocksize){
				perror("\nWrite Error: write returns 0, cannot write enough bytes! \n");
				return;
			}
		}
		else if(rsize<blocksize){
			rsize+=write(fd,examtext,blocksize-rsize);
			if(rsize!=blocksize){
				perror("\nWrite Error: cannot write enough bytes! \n");
				return;
			}
		}
	} 
}

void read_file(int blocksize, bool isrand, char *filepath, int fs)
{
	int fd;
  
	fd=open(filepath,O_RDONLY|O_SYNC,0666);
	if(fd<0){
		perror("\nRead Error: cannot open file! \n");
		return;
	}
    
	for(int i=1;i<=fs/blocksize;i++){
		int z=((rand()*RAND_MAX)+rand())%fs; 
		if(isrand){
			lseek(fd, z, SEEK_SET);
		}
		
		int rsize=read(fd,testtext,blocksize);
		if(rsize<0){
			perror("\nRead Error: return value of read < 0 ! \n");
			return;
		}
		else if(rsize==0){
			lseek(fd, 0, SEEK_SET);
			rsize=read(fd,examtext,blocksize);
			if(rsize!=blocksize){
				perror("\nRead Error: read returns 0, cannot read enough bytes! \n");
				return;
			}
		}
		else if(rsize<blocksize){
			lseek(fd, 0, SEEK_SET);
			rsize+=read(fd,examtext,blocksize-rsize);
			if(rsize!=blocksize){
				perror("\nWrite Error: cannot read enough bytes! \n");
				return;
			}
		}
	} 
}

struct timeval get_time_left(struct timeval starttime, struct timeval endtime)
{
	struct timeval spendtime;
	spendtime.tv_sec=endtime.tv_sec-starttime.tv_sec;
	spendtime.tv_usec=endtime.tv_usec-starttime.tv_usec;
	if(spendtime.tv_usec<0){
		spendtime.tv_sec--;
		spendtime.tv_usec+=S_TO_MUS;
	}
    return spendtime;
}

int main()
{
	printf("Please input path:\n");
	scanf("%s",filepath);
	for(int concurrency=Concurrency;concurrency<(Concurrency+1);concurrency++){
		printf("Blocksize|Random Write|Sequence Write|Random Read|Sequence Read(MB/s) concurrency=%d;\n",concurrency);
		for(int blocksize=Blocksize;blocksize>63;blocksize/=2){
			printf("%dB\t",blocksize);
			fflush(stdout);
			for(int flag=1;flag<5;flag++){
				//if(flag==1) continue;
				//if(flag==2) continue;
				//if(flag==3) continue;
				//if(flag==4) continue;
				
				gettimeofday(&starttime,NULL);
				for(int i=0;i<concurrency;i++){
					if(fork()==0){
						sprintf(x,"%d",i);
						strcat(filepath,x);
						switch(flag){
							case 1:
								//Ëæ»úÐ´ 
								write_file(blocksize, true, &filepath, filesize/concurrency);
								break;
							case 2:
								//Ë³ÐòÐ´ 
								write_file(blocksize, false, &filepath, filesize/concurrency);
								break;
							case 3:
								//Ëæ»ú¶Á 
								read_file(blocksize, true, &filepath, filesize/concurrency);
								break;
							case 4:
								//Ë³Ðò¶Á 
								read_file(blocksize, false, &filepath, filesize/concurrency);
								break;	
						};
						exit(0);
					}
				}
				while(wait(NULL)!=-1);
				gettimeofday(&endtime,NULL);
				spendtimeSpec=get_time_left(starttime,endtime);
				
				long us_ms = (S_TO_MUS * spendtimeSpec.tv_sec + spendtimeSpec.tv_usec);         
		        double timeuse = (us_ms / 1000.0) / 1000.0;    
		        double throughput=filesize/timeuse/1024/1024;
		        printf("%lf\t",throughput);
		        fflush(stdout);
		    }
		    printf("\n");
		}
	}
  return 0;
}



