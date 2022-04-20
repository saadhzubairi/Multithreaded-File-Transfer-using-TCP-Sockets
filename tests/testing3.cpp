#include <stdio.h>
#include <string>
#include <string.h>
#include <unistd.h>

using namespace std;

void fun(char * c){
    printf("char c is %s\n",c);
}

int main(int argc, char const *argv[])
{
    /* char f[10] = "asdass";
    char* c = f;
    
    int x = 20;
    char ft[10];
    sprintf(ft,"%s %d",c,x);
    strcat(f,ft);
    printf("f: %s\n",f);

    fun(c); */

    int c = 84;
    int b = 16;
    int d = c%b;
    printf("d:\t%d\n",d);
    return 0;
}
