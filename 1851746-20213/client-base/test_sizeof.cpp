#include <iostream>
#include <stdio.h>
using namespace std;


int main()
{
    const char* PTO[] = {"SSH", "ר��SSH", "FTP"};
    const int PTONUM = 3;
    const char* STA[] = {"����", "�ػ�", "�ѵ�¼"};
    const int STANUM = 3;
    const char* SIF[] = {"����ϵͳ", "���𿪻�", "�߳���ҵ", "������ҵ"};
    const int SIFNUM = 4;
    const char* TTY[] = {"vt100", "vt220", "xcode"};
    const int TTYNUM = 3;

    for(int i = 0; i < PTONUM; ++i){
        printf("%s - sizeof : %d\n", PTO[i], string(PTO[i]).length());
    }

 
    return 0;
}