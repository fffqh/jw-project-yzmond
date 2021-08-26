#ifndef MY_GETPROC_H
#define MY_GETPROC_H
#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
#include <sys/socket.h>
using namespace std;


//��ȡ�õ� proc ��Ϣ�� string �ķ�ʽ����
class PROCINFO{
public:
    const char* proc_path;
    const char* proc_name;

    PROCINFO(const char* path, const char* name)
    {
        proc_path = path;
        proc_name = name;
    }
    string get()
    {
        string info = "";
        ifstream fin;
        fin.open(proc_path, ios::in);
        if(!fin.is_open()){
            printf("[%d] get proc failed���ļ���ʧ�ܣ�%s��\n", getpid(), proc_path);
            return info;
        }
        string buf;
        while(getline(fin,buf))
        {
            if(buf.find(proc_name)!=buf.npos){ //�ҵ�
                int st;
                if((buf.find(":")) != buf.npos){
                    st = buf.find(":");
                    int size = buf.length() - st -1;
                    info = buf.substr(st + 1, size);
                }
                else{
                    info = buf;
                }                
                break;
            }
        }
        //printf("[%d] test proc get ��%s:%s��\n", getpid(), proc_name, info.c_str());
        fin.close();
        return info;
    }
};

#endif