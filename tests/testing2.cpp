#include <stdio.h>
#include <iostream>
#include <string.h>

using namespace std;

int main()
{

    char buffer[] = "1312309.filenam.asdsad";
    const char delim[] = ".";
   	char *token;
	token = strtok(buffer,delim);
    int num = stoi(token);

    printf("token:%s and num:%d\n",token,num);
    token = strtok(NULL, delim);
    token = strtok(NULL, delim);
    printf("token:%s\n",token);

    return (0);
}