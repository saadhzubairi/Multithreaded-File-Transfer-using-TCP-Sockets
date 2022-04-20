#include <unistd.h>
#include <iostream>
#include <cstdlib>

using namespace std;
int main()
{
    cout << "This is the first line of code\n";
    cout.flush();
    sleep(1);
    cout << "This is second line of code after 5 seconds of wait\n";
}
