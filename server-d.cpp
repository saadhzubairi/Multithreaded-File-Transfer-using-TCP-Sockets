#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstdio>
#include <pthread.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <stddef.h>
#include <dirent.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#define MAX 6400
#define PORT 8080
#define SA struct sockaddr

#define GREEN "\x1b[32m"
#define RED "\x1b[31m"
#define reset "\x1b[0m"

int THREAD_COUNT = 10;
int PACKET_SIZE = 0;
long FILE_SIZE = 0;
int REMAINDER = 0;

void* startFTP(void *in_dat);

void multithread(char *filename, char *ip)
{
    FILE *fp;
    fp = fopen(filename, "rb");
    long size;
    fseek(fp, 0l, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0l, SEEK_SET);
    fclose(fp);

    long per_thread_load = size / THREAD_COUNT;
    PACKET_SIZE = per_thread_load;
    FILE_SIZE = size;
    REMAINDER = size%PACKET_SIZE;

    struct server_dat
    {
        int tid;
        int offset;
        char fn[50];
        char ip[16];
    };

    struct server_dat dats[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        strcpy(dats[i].ip, ip);
        strcpy(dats[i].fn, filename);
        dats[i].offset = i * per_thread_load;
        dats[i].tid = i;
    }

    pthread_t threads[THREAD_COUNT];

    for(int i=0; i<THREAD_COUNT; i++){
		pthread_create(&(threads[i]), NULL, startFTP, &dats[i]);
	}

    for (int i=0; i<THREAD_COUNT; i++){
		pthread_join(threads[i], NULL);
	}

    printf("\nFILE TRANSFER COMPLETE!\n\n");

}

void send_file(char *filename_in, int sockfd, long offset, int _TID)
{
    FILE *fp;
    char *filename = filename_in;
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        perror("[-] Error in reading file.");
        exit(1);
    }

    int size;

    if (_TID == THREAD_COUNT-1)
        size = PACKET_SIZE+REMAINDER;
    else
        size = PACKET_SIZE;

    // write(sockfd,size,sizeof(size));

    int n;
    char buff[MAX];

    // sending size
    sprintf(buff, "%d.[%d]%s", size, _TID, filename);
    //printf("%s\n", buff);
    write(sockfd, buff, sizeof(buff));
    bzero(buff, MAX);

    // recving go signal
    read(sockfd, buff, sizeof(buff));

    fseek(fp, offset, SEEK_SET);
    // sending file

	printf("\n[%d] sending file from offset [%d]\n",_TID,offset);
    if (strncmp("go", buff, 2) == 0)
    {
        char *data_send_buff[size];
        fread(&data_send_buff, sizeof(data_send_buff), 1, fp);
        //printf("[%d] data to be sent: %s from offset: %d and of size %d\n", _TID,data_send_buff,offset,size);
        write(sockfd, data_send_buff, sizeof(data_send_buff));
        bzero(data_send_buff, sizeof(data_send_buff));
    }
    fclose(fp);
}

struct server_dat
{
    int tid;
    int offset;
    char fn[50];
    char ip[16];
};

void FTP(int _TID, int _offs, char *filename, char *ip_in)
{
    char *ip = ip_in;
    int port = 5000 + _TID;
    int e;
    int sockfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    char buffer[MAX];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("[-] Error in socket");
        exit(1);
    }
    //printf("[%d] Server socket created successfully.\n",_TID);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    e = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e < 0)
    {
        perror("[-] Error in bind");
        exit(1);
    }
    //printf("[%d] Binding successfull.\n",_TID);

    if (listen(sockfd, 10) == 0)
    {
        printf("[+] THREAD [%d] LISTENING\n",_TID);
    }
    
    else
    {
        perror("[-] Error in listening");
        exit(1);
    }

    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr *)&new_addr, &addr_size);

    char *fn = filename;
    //calculating the point at which file is going to be read:
    long this_off = _offs;
    
    send_file(fn, new_sock, this_off, _TID);
    //printf("[+] Data written in the file successfully on client.\n");
    close(sockfd);
}

void* startFTP(void *in_dat)
{
    struct server_dat
    {
        int tid;
        int offset;
        char fn[50];
        char ip[16];
    };

    struct server_dat *struct_ptr = (struct server_dat *)in_dat;
    FTP(struct_ptr->tid,
        struct_ptr->offset,
        struct_ptr->fn,
        struct_ptr->ip);
    
    printf("\n[+] THREAD %d COMPLETED\n",struct_ptr->tid);
}

char *strstrip(char *s)
{
	size_t size;
	char *end;

	size = strlen(s);

	if (!size)
		return s;

	end = s + size - 1;
	while (end >= s && isspace(*end))
		end--;
	*(end + 1) = '\0';

	while (*s && isspace(*s))
		s++;

	return s;
}

char *ls()
{

	static char retval[] = "\t\t  ";

	DIR *folder;
	struct dirent *entry;
	int files = 0;
	char *path = ".";

	folder = opendir(path);

	if (folder == NULL)
	{
		puts("Unable to read directory");
	}
	else
	{
		while (entry = readdir(folder))
		{
			if (entry->d_type == 8)
			{
				strcat(strcat(retval, "\n\t\t  "), entry->d_name);
			}
		}
	}
	strcat(retval, "\n");
	closedir(folder);
	return retval;
}


int find(char *str_in)
{
	DIR *folder;
	struct dirent *entry;
	int files = 0;

	char *path = ".";
	folder = opendir(path);

	if (folder == NULL)
	{
		return 0;
	}

	else
	{
		while (entry = readdir(folder))
		{
			files++;
			char *m = entry->d_name;
			if (strcmp(m, str_in) == 0)
			{
				return 1;
			}
		}
		return 0;
	}
	closedir(folder);
}

void func(int n_sockfd, int g_sockfd, char* ip)
{
	char l_buff[MAX];
	int n;

	for (;;)
	{
		printf(reset "[<-] RECV\t: ");
		bzero(l_buff, MAX);
		read(n_sockfd, l_buff, sizeof(l_buff));
		printf("%s", l_buff);

		if (strncmp("exit", l_buff, 4) == 0)
		{
			printf("\n[ENDING CHAT...]\n");
			break;
		}

		// SEND LS WITH LS

		if (strncmp("ls", l_buff, 2) == 0)
		{
			bzero(l_buff, MAX);
			n = 0;
			char *c;
			c = ls();
			c = strstrip(c);
			strcat(c, "\n");
			for (int i = 0; c[i] != '\0'; i++)
				l_buff[i] = c[i];

			printf("[->] SEND\t: SENDING LS OF CURRENT DIRECTORY\n");
			write(n_sockfd, l_buff, sizeof(l_buff));

			bzero(l_buff, MAX);
		}

		// SEND FILE WITH CP

		else if (strncmp("cp", l_buff, 2) == 0)
		{
			const char s[2] = " ";
			char *token;
			token = strtok(l_buff, s);
			token = strtok(NULL, s);
			token = strstrip(token);

			n = 0;
			printf("[->] ....\t: CLIENT REQUESTING FILE -> [%s]\n", token);
			int m = find(token);
			char c[50];
			strcpy(c,token);
			printf("m: %d \tc: %s\n",m,c);
			if (m)
			{
				bzero(l_buff, MAX);
				printf("sending file %s\n",c);
				
				char signal[] = "[+]";
				for (int i = 0; i < 3 ; i++)
					l_buff[i] = signal[i];
				write(n_sockfd, l_buff, sizeof(l_buff));
				
				multithread(c,ip);
				
				strcpy(c,"FILE_SENT\n");
				for (int i = 0; c[i] != '\0'; i++)
					l_buff[i] = c[i];
				write(n_sockfd, l_buff, sizeof(l_buff));
				printf("[->] SEND\t: %s\n",c);
			}
			else
			{
				printf("[->] SEND\t: SENDING ERROR\n");
				strcpy(c,"404 - File not found\n");
				for (int i = 0; c[i] != '\0'; i++)
					l_buff[i] = c[i];
				write(n_sockfd, l_buff, sizeof(l_buff));
			}



		}


		else
		{
			bzero(l_buff, MAX);
			n = 0;
			printf("[->] SEND\t: ");
			while ((l_buff[n++] = getchar()) != '\n')
				;
			write(n_sockfd, l_buff, sizeof(l_buff));
			if (strncmp("exit", l_buff, 4) == 0)
			{
				printf("[ENDING CHAT...]\n");
				break;
			}
		}

		if (strncmp("exit", l_buff, 4) == 0)
		{
			printf("\n[ENDING CHAT...]\n");
			break;
		}
	}
}

int main()
{
	char ip[16] = "127.0.0.1";
	int port = 8080;
	int e;

	int g_sockfd;
	int n_sockfd;
	struct sockaddr_in servaddr, cli;
	socklen_t addr_size;
	// socket create and verification
	g_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (g_sockfd < 0)
	{
		printf(RED "[-] socket creation failed...\n");
		exit(1);
	}

	printf(GREEN "[+] Socket successfully created..\n");

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip);
	servaddr.sin_port = port;

	e = bind(g_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	if (e < 0)
	{
		printf(RED "[-] Socket bind failed...\n");
		exit(1);
	}
	printf(GREEN "[+] Socket successfully binded..\n");

	if ((listen(g_sockfd, 10)) != 0)
	{
		printf(RED "[-] Listen failed...\n");
		exit(0);
	}
	printf(GREEN "[+] Server listening..\n");

	addr_size = sizeof(cli);

	n_sockfd = accept(g_sockfd, (struct sockaddr *)&cli, &addr_size);

	if (n_sockfd < 0)
	{
		printf(RED "[-] Server accept failed...\n");
		exit(1);
	}
	printf(GREEN "[+] Server accept the client...\n");

	/* char *fn = "blahh.txt";
	printf("putting file %s", fn);
	 */
	/* send_file(fn, g_sockfd,servaddr); */

	
	func(n_sockfd, g_sockfd,ip);
	
	close(g_sockfd);


}