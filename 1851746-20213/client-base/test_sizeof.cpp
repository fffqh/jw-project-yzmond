#include <iostream>
#include <stdio.h>
using namespace std;


int main()
{
    const char* PTO[] = {"SSH", "专用SSH", "FTP"};
    const int PTONUM = 3;
    const char* STA[] = {"开机", "关机", "已登录"};
    const int STANUM = 3;
    const char* SIF[] = {"储蓄系统", "基金开户", "高程作业", "计网作业"};
    const int SIFNUM = 4;
    const char* TTY[] = {"vt100", "vt220", "xcode"};
    const int TTYNUM = 3;

    for(int i = 0; i < PTONUM; ++i){
        printf("%s - sizeof : %d\n", PTO[i], string(PTO[i]).length());
    }

 
    return 0;
}