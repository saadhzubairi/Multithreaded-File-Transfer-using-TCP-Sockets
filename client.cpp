#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <cstdio>
#include <pthread.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

#define MAX 6400
#define PORT 8080
#define SA struct sockaddr

#define GREEN "\x1b[32m"
#define RED "\x1b[31m"
#define reset "\x1b[0m"

int THREAD_COUNT = 3;
char fnglob[20];

pthread_mutex_t lock;

int *COMPS;

void FTP(int _TID, char *ip_in);
void *startFTP(void *in_dat);
void combinefiles();

void printarr()
{

	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		if (i == 0)
			printf("[");
		printf("%d", COMPS[i]);
		if (i == (THREAD_COUNT - 1))
			printf("]\n");
		else
			printf(",");
	}
}

void multithread(char *ip)
{

	COMPS = (int *)calloc(sizeof(int), THREAD_COUNT);
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		COMPS[i] = 0;
	}

	struct client_dat
	{
		int tid;
		char ip[16];
	};

	struct client_dat dats[THREAD_COUNT];

	for (int i = 0; i < THREAD_COUNT; i++)
	{
		strcpy(dats[i].ip, ip);
		dats[i].tid = i;
	}

	pthread_t threads[THREAD_COUNT];

	pthread_mutex_init(&lock, NULL);
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		pthread_create(&(threads[i]), NULL, startFTP, &dats[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++)
	{
		pthread_join(threads[i], NULL);
	}

	pthread_mutex_destroy(&lock);
}

void write_file(int *sockfd, int *_TID)
{
	int n;
	// FILE *fp;
	char *filename;
	char *ext;
	char buffer[MAX];

	// recieving size
	read(*sockfd, buffer, sizeof(buffer));

	const char delim[] = ".";
	char *token;

	// tokenizing size of file:
	token = strtok(buffer, delim);
	long fsize = std::stoi(token);
	// tokenizing name of file:
	token = strtok(NULL, delim);
	filename = token;

	// tokenizing ext of file:
	token = strtok(NULL, delim);
	ext = token;

	// sending go signal
	char c[3] = "go";
	for (int i = 0; c[i] != '\0'; i++)
		buffer[i] = c[i];
	write(*sockfd, buffer, sizeof(buffer));

	// recieving file
	char d_buff[fsize];
	long red;
	//printf("[%d] buffer size:   %ld\n", *_TID, sizeof(d_buff));

	

	ssize_t ret;
	while (fsize != 0 && (ret = read(*sockfd, d_buff, sizeof(d_buff))) != 0)
	{
		if (ret == -1)
		{
			if (errno == EINTR)
				continue;
			perror("read");
			break;
		}
		fsize -= ret;
		//printf("[%d] read: %d, left: %d, buffsize: %d\n",*_TID,ret, fsize,sizeof(buffer));
	}

	printf("[%d] read bytes:    %ld\n", *_TID, red);
	// filename setting
	char fn[150] = "";

	strcat(fn, filename);
	strcat(fn, "-d.");
	strcat(fn, ext);

	strcpy(fnglob, fn);

	// printf("[.] creating file: [%s]\n", fn);
	//  writing file
	FILE *fp;
	fp = fopen(fn, "wb+");

	/* FILE *f = fopen("test.txt", "w");
	setbuf(f, NULL);

	while (fgets(d_buff, 100, stdin))
		fputs(s, f); */

	fwrite(&d_buff, sizeof(d_buff), 1, fp);
	bzero(d_buff, fsize);

	fclose(fp);

	// printf("[+] created file: [%s]\n", fn);
	// printf("[.] fetchind data\n");
	// char dat[] = d_buff;
	// strcpy(dat,d_buff);
	// printf("[.] fetchd data: \n%s\n", d_buff);
	// printf("sizeof da: %ld\n", fsize);
	return;
}

void *startFTP(void *in_dat)
{
	pthread_mutex_lock(&lock);
	struct client_dat
	{
		int tid;
		char ip[16];
	};
	struct client_dat *struct_ptr = (struct client_dat *)in_dat;
	FTP(struct_ptr->tid,
		struct_ptr->ip);

	printf("[+] THREAD [%d] RECIEVED DATA\n", struct_ptr->tid);
	COMPS[struct_ptr->tid] = 1;
	int should = 1;
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		if (COMPS[i] == 0)
		{
			should = 0;
			break;
		}
	}
	// printarr();
	if (should == 1)
		combinefiles();
	pthread_mutex_unlock(&lock);
	return NULL;
}

void FTP(int _TID, char *ip_in)
{
	char *ip = ip_in;
	int port = 5000 + _TID;
	int e;
	int sockfd;
	struct sockaddr_in server_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("[%d] Error in socket");
		exit(1);
	}
	// printf("[%d] Server socket created successfully.\n",_TID);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = port;
	server_addr.sin_addr.s_addr = inet_addr(ip);

	e = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (e == -1)
	{
		perror("[-]Error in socket");
		exit(1);
	}
	// printf("[+] THREAD [%d] CONNECTED TO SERVER.\n", _TID);

	write_file(&sockfd, &_TID);

	close(sockfd);
}

void combinefiles()
{
	sleep(1);
	FILE *mainfp;
	char filecon_temp[50] = "";
	char filecon[90] = "";

	//Extracting name of that file
	for (int i = 3; i < sizeof(fnglob); i++)
	{
		sprintf(filecon, "%s%c", filecon_temp, fnglob[i]);
		strcpy(filecon_temp, filecon);
		if (fnglob[i] == '\0')
		{
			break;
		}
	}

	// main file is open
	mainfp = fopen(filecon, "wb+");

	for (int i = 0; i < THREAD_COUNT; i++)
	{
		char fn_temp[130];
		sprintf(fn_temp, "[%d]%s", i, filecon);
		FILE *fptemp;
		fptemp = fopen(fn_temp, "rb");

		long size;
		fseek(fptemp, 0l, SEEK_END);
		size = ftell(fptemp);
		fseek(fptemp, 0l, SEEK_SET);

		char combine_buffer[size];
		fread(&combine_buffer, sizeof(combine_buffer), 1, fptemp);

		// writing into main fp
		fwrite(&combine_buffer, sizeof(combine_buffer), 1, mainfp);
		fclose(fptemp);
		remove(fn_temp);
	}

	fclose(mainfp);
	printf("\nCOMBINING...\n");
	printf("\nDOWNLOADED!!\n\n");
}

void func(int g_sockfd, char *ip)
{
	char buff[MAX];
	int n;

	for (;;)
	{
		bzero(buff, MAX);
		n = 0;

		printf(reset "[->] SEND\t: ");
		while ((buff[n++] = getchar()) != '\n')
			;

		write(g_sockfd, buff, sizeof(buff));

		if (strncmp("exit", buff, 4) == 0)
		{
			printf("\n[ENDING CHAT...]\n");
			break;
		}

		if (strncmp("cp", buff, 2) == 0)
		{

			bzero(buff, MAX);
			read(g_sockfd, buff, sizeof(buff));

			if (strncmp("[+]", buff, 3) == 0)
			{

				printf("[..]     \t: RECIEVING FILES...\n");
				sleep(1);
				multithread(ip);

				bzero(buff, MAX);
				read(g_sockfd, buff, sizeof(buff));
				printf("[<-] RECV\t: %s", buff);
			}
			else
			{
				printf("[<-] RECV\t: %s", buff);
			}
			// write_file(g_sockfd);
		}
		else
		{
			bzero(buff, MAX);
			read(g_sockfd, buff, sizeof(buff));

			if (strncmp("exit", buff, 4) == 0)
			{
				printf("[<-] RECV\t: exit");
				printf("\n[ENDING CHAT...]\n");
				break;
			}
			printf("[<-] RECV\t: ");
			printf("%s", buff);
		}
	}
}

int main()
{
	char ip[16] = "127.0.0.1";
	int port = 8080;
	int e;
	int g_sockfd;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	g_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (g_sockfd < 0)
	{
		printf(RED "[-] socket creation failed...\n");
		exit(1);
	}
	printf(GREEN "[+] Socket successfully created..\n");

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = port;
	servaddr.sin_addr.s_addr = inet_addr(ip);

	e = connect(g_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (e == -1)
	{
		printf(RED "[-] connection with the server failed...\n");
		exit(0);
	}

	printf(GREEN "[+] connected to the server..\n");

	// function for chat
	func(g_sockfd, ip);

	close(g_sockfd);

	return 0;
}