#ifndef MY_GETPROC_H
#define MY_GETPROC_H
#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
#include <sys/socket.h>
using namespace std;


//将取得的 proc 信息以 string 的方式返回
class PROCINFO{
public:
    const char* proc_path;
    const char* proc_name;

    PROCINFO(const char* path, const char* name)
    {
        proc_path = path;
        proc_name = name;
    }
    //得到 proc_path 中的 proc_name 行的信息
    string get()
    {
        string info = "";
        ifstream fin;
        fin.open(proc_path, ios::in);
        if(!fin.is_open()){
            printf("[%d] get proc failed！文件打开失败（%s）\n", getpid(), proc_path);
            return info;
        }
        string buf;
        while(getline(fin,buf))
        {
            if(buf.find(proc_name)!=buf.npos){ //找到
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
        //printf("[%d] test proc get 【%s:%s】\n", getpid(), proc_name, info.c_str());
        fin.close();
        return info;
    }
    //得到cpu使用率信息，proc_path=/proc/stat，proc_name = cpu
    double get_cpu_op()
    {
        double cpu_op;        
        string cpuinfo = get();
        if(cpuinfo == "")
            return -1;
        
        char buf[5]; unsigned long long ut, nt, st, it;
        sscanf(cpuinfo.c_str(), " %s %llu %llu %llu %llu", buf, &ut, &nt, &st, &it);
        if(ut+nt+st+it<=0)
            return -1;    
        cpu_op = ((double)(ut+st)/(ut+nt+st+it)) * 100;
        return cpu_op;
    }
    //得到内存占用率信息，proc_path=任意，proc_name = 任意
    double get_mem_op()
    {
        PROCINFO pinfo_ram("/proc/meminfo", "MemTotal");
        PROCINFO pinfo_free("/proc/meminfo", "MemFree");
        unsigned long long ram, free;
        sscanf(pinfo_ram.get().c_str(), " %llu", &ram);
        sscanf(pinfo_free.get().c_str(), " %llu", &free);
        if(!ram)
            return -1;
        return ((double)(ram-free)/ram)*100;
    }

};

#endif