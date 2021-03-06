#ifndef MY_LOG_H
#define MY_LOG_H
#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/file.h> //加文件锁

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

    time_t GetDateTime(char * psDateTime) 
    {
        time_t nSeconds;
        struct tm * pTM;
        
        time(&nSeconds);
        pTM = localtime(&nSeconds);

        /* 系统日期和时间,格式: yyyymmddHHMMSS */
        sprintf(psDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
                pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
                pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
                
        return nSeconds;
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
    
    bool sif_tolog()
    {
        PROCINFO pinfo_cpu_op("/proc/stat", "cpu");
        PROCINFO pinfo_mem_op("/proc/meminfo", "mem");
        char sys_info_buf[100];
        sprintf(sys_info_buf, "系统当前内存占用率(%.2lf%%)/CPU占用率(%.2lf%%)"
                ,  pinfo_mem_op.get_mem_op(), pinfo_cpu_op.get_cpu_op());
        
        if(tostd){
            printf("%s [%u] %s\n", get_time().c_str(), devid, sys_info_buf);
        }

        if(tofle){
            FILE* fp = NULL;
            fp = fopen(log_path, "a"); //追加写
            if(!fp){
                printf("[%d] mylog::rw_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
                return false;
            }
            //对文件加写锁
            int ifd = fileno(fp);
            if(flock(ifd, LOCK_EX)!=0){
                printf("[%d] 文件加锁失败，可能出现数据错误(%s)!\n", getpid(), log_path);
            }
            fprintf(fp, "%s [%u] %s\n", get_time().c_str(), devid, sys_info_buf);
            //解锁
            if(flock(ifd, LOCK_UN)!=0){
                printf("[%u] 文件解锁失败，可能出现错误（%s）\n", getpid(), log_path);
            }
            fclose(fp);       
        }

        return true;
    }
    bool rw_tolog(int dir, int len)
    {
        if(tostd){
            if(dir == DIR_SEND){
                printf("%s [%u] 发送数据%d字节\n", get_time().c_str(), devid, len);
            }
            else{
                printf("%s [%u] 读取%d字节\n", get_time().c_str(), devid, len);
            }
        }
        if(tofle){
            FILE* fp = NULL;
            fp = fopen(log_path, "a"); //追加写
            if(!fp){
                printf("[%d] mylog::rw_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
                return false;
            }
            //对文件加写锁
            int ifd = fileno(fp);
            if(flock(ifd, LOCK_EX)!=0){
                printf("[%d] 文件加锁失败，可能出现数据错误(%s)!\n", getpid(), log_path);
            }

            if(dir == DIR_SEND){
                fprintf(fp, "%s [%u] 发送数据%d字节\n", get_time().c_str(), devid, len);
            }
            else{
                fprintf(fp, "%s [%u] 读取%d字节\n", get_time().c_str(), devid, len);
            }
            //解锁
            if(flock(ifd, LOCK_UN)!=0){
                printf("[%u] 文件解锁失败，可能出现错误（%s）\n", getpid(), log_path);
            }
            fclose(fp);
        }


        return true;
    }
    bool buf_tolog(int dir, u_char* buf, int len)
    {
        if(tostd){
            if(dir == DIR_SEND){
                printf("%s [%u] (发送数据为:)\n", get_time().c_str(), devid);
            }
            else{
                printf("%s [%u] (读取数据为:)\n", get_time().c_str(), devid);
            }
            //将 buf 按格式写入log
            for(int i = 0; i < (len + 15)/16; ++i){ //按行
                printf("  %04X: ", i);
                //hex
                for(int k = 0; k < 8; ++k){
                    (16*i+k < len)? printf(" %02x", buf[16*i+k]): printf("   ");
                }
                printf(" -");
                for(int k = 8; k < 16; ++k){
                    (16*i+k < len)? printf(" %02x", buf[16*i+k]):printf("   ");
                }
                printf("  ");
                //char
                for(int k = 0; k < 16; ++k){
                    if(16*i+k < len){
                        (isprint(buf[16*i+k]) && !iscntrl(buf[16*i+k])) ? 
                            printf("%c", buf[16*i+k]) : 
                            printf(".");
                    }
                }
                printf("\n");
            }
        }

        if(!tofle)
            return true;
        FILE* fp = NULL;
        fp = fopen(log_path, "a"); //追加写
        if(!fp){
            printf("[%d] mylog::buf_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
            return false;
        }

        //对文件加写锁
        int ifd = fileno(fp);
        if(flock(ifd, LOCK_EX)!=0){
            printf("[%d] 文件加锁失败，可能出现数据错误(%s)!\n", getpid(), log_path);
        }

        if(dir == DIR_SEND){
            fprintf(fp, "%s [%u] (发送数据为:)\n", get_time().c_str(), devid);
        }
        else{
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

        //解锁
        if(flock(ifd, LOCK_UN)!=0){
            printf("[%u] 文件解锁失败，可能出现错误（%s）\n", getpid(), log_path);
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
            //对文件加写锁
            int ifd = fileno(fp);
            if(flock(ifd, LOCK_EX)!=0){
                printf("[%d] 文件加锁失败，可能出现数据错误(%s)!\n", getpid(), log_path);
            }
            fprintf(fp, "%s [%u] %s\n", get_time().c_str(), devid, str);
            //解锁
            if(flock(ifd, LOCK_UN)!=0){
                printf("[%u] 文件解锁失败，可能出现错误（%s）\n", getpid(), log_path);
            }            
            fclose(fp);
        }
        return true;
    }
    bool pack_tolog(int dir, const char* intf, int len=0)
    {
        //return true;
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
            
            //对文件加写锁
            int ifd = fileno(fp);
            if(flock(ifd, LOCK_EX)!=0){
                printf("[%d] 文件加锁失败，可能出现数据错误(%s)!\n", getpid(), log_path);
            }

            if(dir == DIR_RECV)
                fprintf(fp, "%s [%u] 收到客户端状态请求[intf=%s]\n", get_time().c_str(), devid, intf);
            else
                fprintf(fp, "%s [%u] 收到客户端状态应答[intf=%s len=%d(C-0)]\n", get_time().c_str(), devid, intf, len);
            //解锁
            if(flock(ifd, LOCK_UN)!=0){
                printf("[%u] 文件解锁失败，可能出现错误（%s）\n", getpid(), log_path);
            }   
            fclose(fp);
        }
        return true;
    }
    bool fork_tolog(int fnum, int wnum)
    {
        int pos = 1 + wnum - fnum;
        const char* info[]={
        "本次子进程回收数大于分裂数，出错",
        "本次子进程回收全部完成",
        "本次子进程未回收完成"
        };
        if(tostd){
            printf("%s [%u] %s(%d/%d/%d).\n", get_time().c_str(), devid, info[pos], fnum-wnum, fnum, wnum);
        }
        if(tofle){
            FILE* fp = NULL;
            fp = fopen(log_path, "a"); //追加写
            if(!fp){
                printf("[%d] mylog::fork_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
                return false;
            }
            //对文件加写锁
            int ifd = fileno(fp);
            if(flock(ifd, LOCK_EX)!=0){
                printf("[%d] 文件加锁失败，可能出现数据错误(%s)!\n", getpid(), log_path);
            }

            fprintf(fp, "%s [%u] %s(%d/%d/%d).\n", get_time().c_str(), devid, info[pos], fnum-wnum, fnum, wnum);
            //解锁
            if(flock(ifd, LOCK_UN)!=0){
                printf("[%u] 文件解锁失败，可能出现错误（%s）\n", getpid(), log_path);
            }   
            fclose(fp);    
        }
        return true;
    }


    time_t fst_tolog()
    {
        const int DTSize = 30;
        char DateTime[DTSize] = {0};
        time_t nSeconds = GetDateTime(DateTime);
        if(tostd){
            printf("%s [%u] fork子进程开始[%s]\n", get_time().c_str(), devid, DateTime);
        }
        if(tofle){
            FILE* fp = NULL;
            fp = fopen(log_path, "a"); //追加写
            if(!fp){
                printf("[%d] mylog::fork_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
                return nSeconds;
            }
            //对文件加写锁
            int ifd = fileno(fp);
            if(flock(ifd, LOCK_EX)!=0){
                printf("[%d] 文件加锁失败，可能出现数据错误(%s)!\n", getpid(), log_path);
            }
            fprintf(fp, "%s [%u] fork子进程开始[%s]\n", get_time().c_str(), devid, DateTime);
            //解锁
            if(flock(ifd, LOCK_UN)!=0){
                printf("[%u] 文件解锁失败，可能出现错误（%s）\n", getpid(), log_path);
            }   
            fclose(fp);  

        }
        return nSeconds;
    }
    time_t fed_tolog(time_t fst_nSeconds, int num)
    {
        const int DTSize = 30;
        char DateTime[DTSize] = {0};
        time_t fed_nSeconds = GetDateTime(DateTime);
        
        //printf("[%d] test fst_nSeconds=%ld, fed_nSeconds=%ld\n", getpid(), fst_nSeconds, fed_nSeconds);

        if(tostd){
            printf("%s [%u] fork子进程结束[%s]，总数=%d，耗时=%ld秒\n",
                    get_time().c_str(), devid, DateTime, num,(fed_nSeconds-fst_nSeconds));
        }
    
        if(tofle){
            FILE* fp = NULL;
            fp = fopen(log_path, "a"); //追加写
            if(!fp){
                printf("[%d] mylog::fed_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
                return fed_nSeconds;
            }
            //对文件加写锁
            int ifd = fileno(fp);
            if(flock(ifd, LOCK_EX)!=0){
                printf("[%d] 文件加锁失败，可能出现数据错误(%s)!\n", getpid(), log_path);
            }
            fprintf(fp, "%s [%u] fork子进程结束[%s]，总数=%d，耗时=%ld秒\n",
                    get_time().c_str(), devid, DateTime, num, fed_nSeconds-fst_nSeconds);
            //解锁
            if(flock(ifd, LOCK_UN)!=0){
                printf("[%u] 文件解锁失败，可能出现错误（%s）\n", getpid(), log_path);
            }   
            fclose(fp);      
        }
        return fed_nSeconds;
    }

    bool cnt_tolog(u_int sumtty, u_int sumscr)
    {
        if(tostd){
            printf("%s\t%u\t%u\t%u\t%u\n", get_time().c_str(), devid, 1, sumtty, sumscr);
        }
        if (tofle){
            FILE* fp = NULL;
            fp = fopen(log_path, "a"); //追加写
            if(!fp){
                printf("[%d] mylog::cnt_tolog failed！文件打开失败（%s）\n", getpid(), log_path);
                return false;
            }
            fprintf(fp, "%s\t%u\t%u\t%u\t%u\n", get_time().c_str(), devid, 1, sumtty, sumscr);
            fclose(fp); 
        }
        return true;     
    }
};

#endif


