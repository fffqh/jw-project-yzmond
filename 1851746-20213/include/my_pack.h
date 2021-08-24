#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <ctype.h> //判断可见字符
#include <sys/socket.h>

using namespace std;

#include "my_socket.h"

// __HEAD_PACK
// +-----------+------------+----------------------+
// | head_type | head_index | pack_size            |          
// +-----------+------------+----------------------+
// | pad                    | data_size            |
// +------------------------+----------------------+

struct HEADPACK{
    u_char head_type;
    u_char head_index;
    u_short pack_size;
    u_short pad;
    u_short data_size;
};

struct SCP_AUTH{
    u_short vno_main;                u_char vno_sub1, vno_sub2;
    u_short dt_fail;                 u_short dt_succ;
    u_char ety_allow; u_char pad1;   u_short pad2;
    u_char key[32];
    u_int random_num, svr_time;
};

struct CSP_AUTH{
    u_short cpuMHz; u_short MTotal;
    u_short ROM;    u_short dev_inid;
    u_char  dev_gid[16];
    u_char  dev_tid[16];
    u_char  dev_vid[16];
    u_char  ethnum, syncnum, asyncnum, switchnum;
    u_char  usbnum, prnnum; u_short pad1;
    u_int   devid;
    u_char  devno, pad2; u_short pad3;
    u_char  key[32];
    u_int   random_num;
    CSP_AUTH( u_short in_cpuMHz, u_short in_MTotal, u_int in_devid){
        cpuMHz = htons(in_cpuMHz);
        MTotal = htons(in_MTotal);
        ROM = htons(581);
        dev_inid = htons(1746);
        memcpy(dev_gid, "fanqianhui", sizeof("fanqianhui"));
        memcpy(dev_tid, "fqh-6471581", sizeof("fqh-6471581"));
        memcpy(dev_vid, "Ver:1746", sizeof("Ver:1746"));
        ethnum    =  1 + in_devid%2;
        syncnum   = 0;
        asyncnum  = ((in_devid/10) % 3)*8;
        switchnum = 0;
        usbnum    = (in_devid/100) % 2;
        prnnum    = (in_devid/1000) % 2;
    
        devid = htonl(in_devid);
        devno = 1;
        memcpy(key, "yzmond:id*str&to!tongji@by#Auth^", 32);
    }
};

struct CSP_SYSIF{
    u_int user_cputime;
    u_int nice_cputime;
    u_int sys_cputime;
    u_int idle_cputime;
    u_int freed_memory;
    CSP_SYSIF(u_int ut, u_int nt, u_int st, u_int it, u_int m)
    {
        user_cputime = htonl(ut);
        nice_cputime = htonl(nt);
        sys_cputime  = htonl(st);
        idle_cputime = htonl(it);
        freed_memory = htonl(m);
    }
};


struct CSP_VNO{
    u_short vno_main;
    u_char  vno_sub1;
    u_char  vno_sub2;
    CSP_VNO(u_short m, u_char s1, u_char s2){
        vno_main = htons(m);
        vno_sub1 = s1;
        vno_sub2 = s2;
    }
};

struct CSP_USBIF{
    u_char is_usb; u_char pad1;
    u_short pad2;
    CSP_USBIF(u_int devid){
        is_usb = (u_char) (devid%2);
        pad1 = 0x00;
        pad2 = 0x0000;
    }
};

struct CSP_PRNIF{
    u_char is_live; u_char pad1; 
    u_short work_num;
    u_char name[12];
    CSP_PRNIF(u_int devid){
        is_live = (u_char)((devid/10) %2);
        if(is_live){
            work_num = htons((devid % 100) % 25);
        }else{
            work_num = htons(0);
        }
        memcpy(name, "PRN-fqh-1746", 12);
    }
}

class NETPACK{
public:
    /*包数据*/
    HEADPACK head;
    u_char* databuf;
    
    /*构造函数*/
    NETPACK() //拆包
    {
        head = {0};
        databuf = NULL;
    };
    NETPACK(u_short head_info) //封包
    {
        memcpy(&(head.head_type), &head_info, 1);
        memcpy(&(head.head_index), (u_char*)(&head_info) + 1, 1);
        head.pad = 0x0000;
        databuf = NULL;
    }
    /*析构函数*/
    ~NETPACK(){if(databuf) delete databuf;}

    /*操作函数*/
    //装载包
    bool upload(SOCK_INFO* sinfo)
    {
        if( !head.pack_size || SEND_BUFSIZE-sinfo->sendbuf_len < head.pack_size)
            return false;
        u_short dsize = head.data_size;
        u_short psize = head.pack_size;
        //主机序转网络序
        head.data_size = htons(head.data_size);
        head.pack_size = htons(head.pack_size);
        //to sendbuf
        memcpy(sinfo->sendbuf + sinfo->sendbuf_len, &head, psize-dsize);
        memcpy(sinfo->sendbuf + sinfo->sendbuf_len + psize-dsize, databuf, dsize);
        sinfo->sendbuf_len += psize;
        return true;
    }
    //卸载包
    bool dwload(SOCK_INFO* sinfo, u_short pack_size, u_short data_size )
    {
        if( !pack_size || sinfo->recvbuf_len < pack_size)
            return false;
        //得到报文头
        memcpy(&head, sinfo->recvbuf, pack_size - data_size);
        head.pack_size = ntohs(head.pack_size);
        head.data_size = ntohs(head.data_size);
        printf("[%d] 得到报文头 head_type:0x%02x head_index:0x%02x pack_size:%d data_size:%d\n"
                , getpid(), head.head_type, head.head_index, head.pack_size, head.data_size);
        //得到报文数据
        databuf = new (nothrow) u_char[data_size];
        if(!databuf){
            printf("[%d] dwload failed！动态申请空间失败\n", getpid());
            return false;
        }
        memcpy(databuf, sinfo->recvbuf+pack_size-data_size, data_size);
        printf("[%d] 得到报文(len:%d) data_first:%02x\n"
                , getpid(), data_size, databuf[0]);
        //更新recvbuf_len
        sinfo->recvbuf_len -= pack_size;
        if(sinfo->recvbuf_len > 0)
            memmove(sinfo->recvbuf, sinfo->recvbuf + pack_size, sinfo->recvbuf_len);
        //to_log
        data_tolog("./client_data.log", data_size);
        return true;
    }
    //申请包空间
    bool mk_databuf(int size)
    {
        if(size <= 0)
            return false;
        databuf = new (nothrow) u_char [size];
        if(!databuf){
            printf("[%d] mk_databuf failed！动态申请空间失败\n", getpid());
            return false;
        }

        return true;
    }

private:
    void data_tolog(string log_path, int data_size)
    {
        FILE* fp = NULL;
        fp = fopen(log_path.c_str(), "a"); //追加写
        if(!fp){
            printf("[%d] data_tolog failed！文件打开失败（%s）\n", getpid(), log_path.c_str());
            return;
        }

        //将databuf按格式写入log
        for(int i = 0; i < (data_size + 15)/16; ++i){ //按行
            fprintf(fp, "  %04X: ", i);
            //hex
            for(int k = 0; k < 8; ++k){
                (16*i+k < data_size)? fprintf(fp, " %02x", databuf[16*i+k]): fprintf(fp, "   ");
            }
            fprintf(fp, " -");
            for(int k = 8; k < 16; ++k){
                (16*i+k < data_size)? fprintf(fp, " %02x", databuf[16*i+k]):fprintf(fp, "   ");
            }
            fprintf(fp, "  ");
            //char
            for(int k = 0; k < 16; ++k){
                if(16*i+k < data_size){
                    (isprint(databuf[16*i+k]) && !iscntrl(databuf[16*i+k])) ? 
                        fprintf(fp, "%c", databuf[16*i+k]) : 
                        fprintf(fp, ".");
                }
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }

};
