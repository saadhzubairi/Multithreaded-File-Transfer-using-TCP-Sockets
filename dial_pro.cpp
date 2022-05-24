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
#define MAGENTA "\x1b[35m"
#define reset "\x1b[0m"

int THREAD_COUNT = 3;
int PACKET_SIZE = 0;
long FILE_SIZE = 0;
int REMAINDER = 0;
char fnglob[20];
pthread_mutex_t lock;
pthread_t server_thread;
pthread_t client_thread;
int *COMPS;

void start_server(char *ip_add);
void start_client(char *ip_add);
void *send_cs(void *a);
void *recv_cs(void *a);
void send_cf(int n_sockfd, int g_sockfd, char *ip);
void recv_cf(int n_sockfd, int g_sockfd, char *ip);
void multithread_SEND(char *filename, char *ip);
void multithread_RECV(char *ip);
void *startFTP_SEND(void *in_dat);
void *startFTP_RECV(void *in_dat);
void FTP_SEND(int _TID, int _offs, char *filename, char *ip_in);
void FTP_RECV(int _TID, char *ip_in);
void send_file(char *filename_in, int sockfd, long offset, int _TID);
void write_file(int *sockfd, int *_TID);
void combinefiles();
char *strstrip(char *s);
char *ls();
int find(char *str_in);
void printarr();

struct server_dat
{
    int tid;
    int offset;
    char fn[50];
    char ip[16];
};
struct chat
{
    int n_sockfd;
    int g_sockfd;
    char *ip;
    int ser_cli; // 0 for server 1 for client
};

void start_server(char *ip_add)
{
    char *ip;
    ip = ip_add;
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
        return;
    }

    printf(GREEN "[+] Socket successfully created..\n");

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = port;

    e = bind(g_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    if (e < 0)
    {
        printf(RED "[-] Socket bind failed...\n");
        return;
    }
    printf(GREEN "[+] Socket successfully binded..\n");

    if ((listen(g_sockfd, 10)) != 0)
    {
        printf(RED "[-] Listen failed...\n");
        return;
    }
    printf(GREEN "[+] Server listening..\n");

    addr_size = sizeof(cli);

    n_sockfd = accept(g_sockfd, (struct sockaddr *)&cli, &addr_size);

    if (n_sockfd < 0)
    {
        printf(RED "[-] Server accept failed...\n");
        return;
    }
    printf(GREEN "[+] Server accept the client...\n");

    /* char *fn = "blahh.txt";
    printf("putting file %s", fn);
     */
    /* send_file(fn, g_sockfd,servaddr); */
    struct chat cs;
    cs.g_sockfd = g_sockfd;
    cs.n_sockfd = n_sockfd;
    cs.ip = ip;
    cs.ser_cli = 0;
    pthread_t send_c, recv_c;

    pthread_create(&send_c, NULL, send_cs, (void *)&cs);
    pthread_create(&recv_c, NULL, recv_cs, (void *)&cs);

    pthread_join(send_c, NULL);
    pthread_join(recv_c, NULL);

    // CHAT_SERV(n_sockfd, g_sockfd, ip);

    close(g_sockfd);
}
void start_client(char *ip_add)
{
    char *ip;
    ip = ip_add;
    int port = 8080;
    int e;
    int g_sockfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    g_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sockfd < 0)
    {
        printf(RED "[-] socket creation failed...\n");
        return;
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
        return;
    }

    printf(GREEN "[+] connected to the server..\n");

    struct chat cs;
    cs.g_sockfd = g_sockfd;
    cs.n_sockfd = g_sockfd;
    cs.ip = ip;
    cs.ser_cli = 1;
    pthread_t send_c, recv_c;

    pthread_create(&send_c, NULL, send_cs, (void *)&cs);
    pthread_create(&recv_c, NULL, recv_cs, (void *)&cs);

    pthread_join(send_c, NULL);
    pthread_join(recv_c, NULL);
    // function for chat
    // CHAT_CLI(g_sockfd, ip);

    close(g_sockfd);
}

void *send_cs(void *a)
{
    struct chat *ch = (struct chat *)a;
    send_cf(ch->n_sockfd, ch->g_sockfd, ch->ip);
}
void *recv_cs(void *a)
{
    struct chat *ch = (struct chat *)a;
    recv_cf(ch->n_sockfd, ch->n_sockfd, ch->ip);
}

int chat = 0;

void send_cf(int n_sockfd, int g_sockfd, char *ip)
{
    int n;
    char buff[MAX];
    while (true)
    {
        bzero(buff, MAX);
        n = 0;
        printf("");
        while ((buff[n++] = getchar()) != '\n')
            ;
        write(n_sockfd, buff, sizeof(buff));
        //IF SENT EXIT
        if (strncmp("exit", buff, 4) == 0)
        {
            chat = 1;
            printf("[ENDING CHAT...]\n");
            break;
        }
        //IF SENT CP OF SOMETHING
		if (strncmp("cp", buff, 2) == 0)
		{

			bzero(buff, MAX);
			read(g_sockfd, buff, sizeof(buff));

			if (strncmp("[+]", buff, 3) == 0)
			{

				printf("[..]     \t: RECIEVING FILES...\n");
				sleep(1);
				multithread_RECV(ip);

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
    }
}
void recv_cf(int n_sockfd, int g_sockfd, char *ip)
{
    while (true)
    {
        char buff[MAX];
        bzero(buff, MAX);
        read(n_sockfd, buff, sizeof(buff));
        printf(reset "\n\t\t\t%s", buff);
        // IF RECIEVED TEXT HAS EXIT
        if (strncmp("exit", buff, 4) == 0)
        {
            if (!chat)
                printf("\n[TYPE 'exit' TO END CHAT]\n");
            break;
        }
        // IF RECIEVED TEXT HAS CP
		if (strncmp("ls", buff, 2) == 0)
		{
			bzero(buff, MAX);
			int n = 0;
            char *c;
			c = ls();
			c = strstrip(c);
			strcat(c, "\n");
			for (int i = 0; c[i] != '\0'; i++)
				buff[i] = c[i];

			printf("[->] SEND\t: SENDING LS OF CURRENT DIRECTORY\n");
			write(n_sockfd, buff, sizeof(buff));

			bzero(buff, MAX);
		}
        // IF RECIEVED TEXT HAS CP
		if (strncmp("cp", buff, 2) == 0)
		{
			const char s[2] = " ";
			char *token;
			token = strtok(buff, s);
			token = strtok(NULL, s);
			token = strstrip(token);

			int n = 0;
			printf("[->] ....\t: CLIENT REQUESTING FILE -> [%s]\n", token);
			int m = find(token);
			char c[50];
			strcpy(c, token);
			printf("m: %d \tc: %s\n", m, c);
			if (m)
			{
				bzero(buff, MAX);
				printf("sending file %s\n", c);

				char signal[] = "[+]";
				for (int i = 0; i < 3; i++)
					buff[i] = signal[i];
				write(n_sockfd, buff, sizeof(buff));

				multithread_SEND(c, ip);

				strcpy(c, "FILE_SENT\n");
				for (int i = 0; c[i] != '\0'; i++)
					buff[i] = c[i];
				write(n_sockfd, buff, sizeof(buff));
				printf("[->] SEND\t: %s\n", c);
			}
			else
			{
				printf("[->] SEND\t: SENDING ERROR\n");
				strcpy(c, "404 - File not found\n");
				for (int i = 0; c[i] != '\0'; i++)
					buff[i] = c[i];
				write(n_sockfd, buff, sizeof(buff));
			}
		}
    }
}

void multithread_SEND(char *filename, char *ip)
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
    REMAINDER = size % PACKET_SIZE;

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

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&(threads[i]), NULL, startFTP_SEND, &dats[i]);
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("[+] THREADS LISTENING\n");

    printf("\nFILE TRANSFER COMPLETE!\n\n");
}
void multithread_RECV(char *ip)
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
        pthread_create(&(threads[i]), NULL, startFTP_RECV, &dats[i]);
    }

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);
}

void *startFTP_SEND(void *in_dat)
{

    struct server_dat
    {
        int tid;
        int offset;
        char fn[50];
        char ip[16];
    };

    struct server_dat *struct_ptr = (struct server_dat *)in_dat;
    FTP_SEND(struct_ptr->tid,
             struct_ptr->offset,
             struct_ptr->fn,
             struct_ptr->ip);

    printf("[+] THREAD [%d] COMPLETED\n", struct_ptr->tid);

    return NULL;
}
void *startFTP_RECV(void *in_dat)
{
    pthread_mutex_lock(&lock);
    struct client_dat
    {
        int tid;
        char ip[16];
    };
    struct client_dat *struct_ptr = (struct client_dat *)in_dat;
    FTP_RECV(struct_ptr->tid,
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

void FTP_SEND(int _TID, int _offs, char *filename, char *ip_in)
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
    // printf("[%d] Server socket created successfully.\n",_TID);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    e = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e < 0)
    {
        perror("[-] Error in bind");
        exit(1);
    }
    // printf("[%d] Binding successfull.\n",_TID);

    if (listen(sockfd, 10) == 0)
    {
        // printf("[+] THREAD [%d] LISTENING\n",_TID);
    }

    else
    {
        perror("[-] Error in listening");
        exit(1);
    }

    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr *)&new_addr, &addr_size);

    char *fn = filename;
    // calculating the point at which file is going to be read:
    long this_off = _offs;

    send_file(fn, new_sock, this_off, _TID);
    // printf("[+] Data written in the file successfully on client.\n");
    close(sockfd);
}
void FTP_RECV(int _TID, char *ip_in)
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

    if (_TID == THREAD_COUNT - 1)
        size = PACKET_SIZE + REMAINDER;
    else
        size = PACKET_SIZE;

    // write(sockfd,size,sizeof(size));

    int n;
    char buff[MAX];

    // sending size
    sprintf(buff, "%d.[%d]%s", size, _TID, filename);
    // printf("%s\n", buff);
    write(sockfd, buff, sizeof(buff));
    bzero(buff, MAX);

    // recving go signal
    read(sockfd, buff, sizeof(buff));

    fseek(fp, offset, SEEK_SET);
    // sending file
    // printf("\n[%d] sending file from offset [%d]\n",_TID,offset);
    if (strncmp("go", buff, 2) == 0)
    {
        char *data_send_buff[size];
        fread(&data_send_buff, sizeof(data_send_buff), 1, fp);
        printf("[%d] data size: %d ", _TID, size);
        write(sockfd, data_send_buff, sizeof(data_send_buff));
        bzero(data_send_buff, sizeof(data_send_buff));
    }
    fclose(fp);
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
    // printf("[%d] buffer size:   %ld\n", *_TID, sizeof(d_buff));

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
        // printf("[%d] read: %d, left: %d, buffsize: %d\n",*_TID,ret, fsize,sizeof(buffer));
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
void combinefiles()
{
    sleep(1);
    FILE *mainfp;
    char filecon_temp[50] = "";
    char filecon[90] = "";

    // Extracting name of that file
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

    static char retval[2048] = "\t\t  ";

    DIR *folder;
    struct dirent *entry;
    int files = 0;
    char path[2] = ".";
    bzero(retval, sizeof(retval));
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

    char path[90] = ".";
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

void CHAT_SERV(int n_sockfd, int g_sockfd, char *ip)
{
    char buff[MAX];
    int n;

    for (;;)
    {
        printf(reset "[<-] RECV\t: ");
        bzero(buff, MAX);
        read(n_sockfd, buff, sizeof(buff));
        printf("%s", buff);

        // IF RECIEVED TEXT HAS EXIT
        if (strncmp("exit", buff, 4) == 0)
        {
            printf("\n[ENDING CHAT...]\n");
            break;
        }
        // IF RECIEVED TEXT HAS CP
        if (strncmp("ls", buff, 2) == 0)
        {
            bzero(buff, MAX);
            n = 0;
            char *c;
            c = ls();
            c = strstrip(c);
            strcat(c, "\n");
            for (int i = 0; c[i] != '\0'; i++)
                buff[i] = c[i];

            printf("[->] SEND\t: SENDING LS OF CURRENT DIRECTORY\n");
            write(n_sockfd, buff, sizeof(buff));

            bzero(buff, MAX);
        }

        // IF RECIEVED TEXT HAS CP
        else if (strncmp("cp", buff, 2) == 0)
        {
            const char s[2] = " ";
            char *token;
            token = strtok(buff, s);
            token = strtok(NULL, s);
            token = strstrip(token);

            n = 0;
            printf("[->] ....\t: CLIENT REQUESTING FILE -> [%s]\n", token);
            int m = find(token);
            char c[50];
            strcpy(c, token);
            printf("m: %d \tc: %s\n", m, c);
            if (m)
            {
                bzero(buff, MAX);
                printf("sending file %s\n", c);

                char signal[] = "[+]";
                for (int i = 0; i < 3; i++)
                    buff[i] = signal[i];
                write(n_sockfd, buff, sizeof(buff));

                multithread_SEND(c, ip);

                strcpy(c, "FILE_SENT\n");
                for (int i = 0; c[i] != '\0'; i++)
                    buff[i] = c[i];
                write(n_sockfd, buff, sizeof(buff));
                printf("[->] SEND\t: %s\n", c);
            }
            else
            {
                printf("[->] SEND\t: SENDING ERROR\n");
                strcpy(c, "404 - File not found\n");
                for (int i = 0; c[i] != '\0'; i++)
                    buff[i] = c[i];
                write(n_sockfd, buff, sizeof(buff));
            }
        }

        else
        {
            bzero(buff, MAX);
            n = 0;
            printf("[->] SEND\t: ");
            while ((buff[n++] = getchar()) != '\n')
                ;
            write(n_sockfd, buff, sizeof(buff));
            if (strncmp("exit", buff, 4) == 0)
            {
                printf("[ENDING CHAT...]\n");
                break;
            }
        }

        if (strncmp("exit", buff, 4) == 0)
        {
            printf("\n[ENDING CHAT...]\n");
            break;
        }
    }
}
void CHAT_CLI(int g_sockfd, char *ip)
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
                multithread_RECV(ip);

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

void *serv(void *)
{
    char ip[16] = "127.0.0.1";
    start_server(ip);
}

void *cli(void *)
{
    char ip[16];
    printf(reset "Enter ip to connect to server: ");
    scanf("%s", &ip);
    printf("\n");
    printf(GREEN "Attempting connecting with server @%s\n", ip);
    // pthread_cancel(server_thread);
    start_client(ip);
}

int main()
{

    pthread_create(&server_thread, NULL, serv, NULL);
    sleep(1);
    pthread_create(&client_thread, NULL, cli, NULL);

    pthread_join(server_thread, NULL);
    sleep(1);
    pthread_join(client_thread, NULL);
}