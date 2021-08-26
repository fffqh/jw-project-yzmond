#ifndef MY_LOG_H
#define MY_LOG_H
#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
#include <sstream>
#include <sys/socket.h>

using namespace std;

#include "./my_socket.h"

#define   DIR_SEND 0
#define   DIR_RECV 1

class MYLOG{
private:
    const char * log_path;
    u_int devid;
    bool  tofle;
    bool  tostd;

    char* get_time_full(void)
    {
        static char s[30]={0};
        char YMD[15] = {0};
        char HMS[10] = {0};
        time_t current_time;
        struct tm* now_time;

        char *cur_time = (char *)malloc(21*sizeof(char));
        time(&current_time);
        now_time = localtime(&current_time);

        strftime(YMD, sizeof(YMD), "%F ", now_time);
        strftime(HMS, sizeof(HMS), "%T", now_time);
        
        strncat(cur_time, YMD, 11);
        strncat(cur_time, HMS, 8);

        memcpy(s, cur_time, strlen(cur_time)+1);
        free(cur_time);

        cur_time = NULL;

        return s;
    }
    string get_time(void)
    {
        char HMS[10] = {0};
        time_t current_time;
        struct tm* now_time;
        time(&current_time);
        now_time = localtime(&current_time);
        strftime(HMS, sizeof(HMS), "%T", now_time);
        return string(HMS);
    }

public:
    MYLOG(const char* path, u_int index)
    {
        log_path = path;
        devid = index;
        tofle  = 1;
        tostd  = 0;
    }
    void set_fle(bool f)
    {
        tofle = f;
    }
    void set_std(bool s)
    {
        tostd = s;
    }
    void set_devid(u_int index)
    {
        devid = index;
    }
    
    bool buf_tolog(int dir, u_char* buf, int len)
    {
        FILE* fp = NULL;
        fp = fopen(log_path, "a"); //追加写
        if(!fp){
            printf("[%d] mylog::buf_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
            return false;
        }
        if(dir == DIR_SEND){
            fprintf(fp, "%s [%u] 发送数据%d字节\n", get_time().c_str(), devid, len);
            fprintf(fp, "%s [%u] (发送数据为:)\n", get_time().c_str(), devid);
        }
        else{
            fprintf(fp, "%s [%u] 读取%d字节\n", get_time().c_str(), devid, len);
            fprintf(fp, "%s [%u] (读取数据为:)\n", get_time().c_str(), devid);
        }
        //将 buf 按格式写入log
        for(int i = 0; i < (len + 15)/16; ++i){ //按行
            fprintf(fp, "  %04X: ", i);
            //hex
            for(int k = 0; k < 8; ++k){
                (16*i+k < len)? fprintf(fp, " %02x", buf[16*i+k]): fprintf(fp, "   ");
            }
            fprintf(fp, " -");
            for(int k = 8; k < 16; ++k){
                (16*i+k < len)? fprintf(fp, " %02x", buf[16*i+k]):fprintf(fp, "   ");
            }
            fprintf(fp, "  ");
            //char
            for(int k = 0; k < 16; ++k){
                if(16*i+k < len){
                    (isprint(buf[16*i+k]) && !iscntrl(buf[16*i+k])) ? 
                        fprintf(fp, "%c", buf[16*i+k]) : 
                        fprintf(fp, ".");
                }
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
        return true;
    }
    bool str_tolog(const char* str)
    {
        if(tostd){
            printf("%s [%u] %s\n", get_time().c_str(), devid, str);
        }
        
        if(tofle){
            FILE* fp = NULL;
            fp = fopen(log_path, "a"); //追加写
            if(!fp){
                printf("[%d] mylog::str_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
                return false;
            }
            fprintf(fp, "%s [%u] %s\n", get_time().c_str(), devid, str);
            fclose(fp);
        }

        return true;
    }
    bool pack_tolog(int dir, const char* intf, int len=0)
    {
        if(tostd){
            if(dir == DIR_RECV)
                printf("%s [%u] 收到客户端状态请求[intf=%s]\n", get_time().c_str(), devid, intf);
            else
                printf("%s [%u] 收到客户端状态应答[intf=%s len=%d(C-0)]\n", get_time().c_str(), devid, intf, len);
        }
        if(tofle){
            FILE* fp = NULL;
            fp = fopen(log_path, "a"); //追加写
            if(!fp){
                printf("[%d] mylog::pack_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
                return false;
            }

            if(dir == DIR_RECV)
                fprintf(fp, "%s [%u] 收到客户端状态请求[intf=%s]\n", get_time().c_str(), devid, intf);
            else
                fprintf(fp, "%s [%u] 收到客户端状态应答[intf=%s len=%d(C-0)]\n", get_time().c_str(), devid, intf, len);

            fclose(fp);
        }
        return true;
    }

};

#endif


