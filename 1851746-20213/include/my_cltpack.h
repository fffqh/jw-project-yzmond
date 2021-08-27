#ifndef MY_CLTPACK_H
#define MY_CLTPACK_H

#include <unistd.h>
#include <string>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include "./my_socket.h"
#include "./my_getproc.h"
#include "./my_getconf.h"
#include "./my_encrpty.h"
#include "./my_log.h"
#include "./my_pack.h"

#define PACK_STOP -2  //阻塞等待中
#define PACK_UNDO -1  //未处理完成
#define PACK_EMPTY 0  //空状态
#define PACK_HALF  1  //未接收/发送完成

#define CONF_DAT_PATH "./config.dat"
#define PROC_DAT_PATH "./process.dat"
#define USBF_DAT_PATH "./usbfiles.dat"

extern char _conf_ip[16];
extern int  _conf_port  ;
extern bool _conf_quit  ;
extern int  _conf_mindn ;
extern int  _conf_maxdn ;
extern int  _conf_minsn ;
extern int  _conf_maxsn ;
extern bool _conf_newlog;
extern int  _conf_debug ;
extern int  _conf_dprint;
extern int  _conf_isarm;

extern int  _devid;
extern int  _devnum;
extern MYLOG _mylog;

extern u_int _sum_tty;
extern u_int _sum_scr;

//自定义结构体
struct CLTPACK;
struct SEVPACK;
class  CLT_PACK_ACTION;

struct HPADINFO{
    u_short head_pad;
    HPADINFO* next;
};

struct SEVPACK{
    const int       no;
    const int       bno; //0表示无需回复
    bool (CLT_PACK_ACTION::*unpack_fun)(SOCK_INFO*, SEVPACK*, CLTPACK*, NETPACK*); //解包函数
    const u_short   head;
    const char      *str;
    int             status;
};
struct CLTPACK{
    const int        no;
    const int        ispad;
    HPADINFO*        hpif;//待发送包序列的head_pad
    bool (CLT_PACK_ACTION::*pack_fun)(SOCK_INFO*, SEVPACK*, CLTPACK*, NETPACK*); //封包函数
    const u_short    head;
    const char      *str;
    int              status;
};

class CLT_PACK_ACTION{
public:
    SOCK_INFO* sinfo;
    SEVPACK* SINFO;
    CLTPACK* CINFO;
    CLT_PACK_ACTION(SOCK_INFO* s,SEVPACK* SP,CLTPACK* CP)
    {
        sinfo = s;
        SINFO = SP;
        CINFO = CP;
    }
    //客户端：解包拆包并进行相应处理的函数
    bool mkpack_vno(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //检查缓冲区空间是否足够
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 12)
            return false;
        //printf("[%d] 封包pack_vno\n", getpid());   
        
        CSP_VNO pack(2,0,0);
        if(!netpack->mk_databuf(sizeof(CSP_VNO)))
            return false;
        netpack->head.data_size = sizeof(CSP_VNO);
        netpack->head.pack_size = 8 + sizeof(CSP_VNO);
        memcpy(netpack->databuf, &pack, sizeof(pack));
        if(!netpack->upload(sinfo))
            return false;
        //_mylog.pack_tolog(DIR_SEND, "发最低版本", 8 + sizeof(pack));
        return true;
    }
    bool mkpack_auth(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //检查缓冲区空间是否足够
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8+(int)sizeof(CSP_AUTH))
            return false;
        //printf("[%d] 封包pack_auth\n", getpid());  

        char cpuMHzName[12];
        memset(cpuMHzName,  0, sizeof(cpuMHzName));
        if(_conf_isarm)
            memcpy(cpuMHzName, "BogoMIPS", 8);
        else
            memcpy(cpuMHzName, "cpu MHz", 7);
        
        PROCINFO pinfo_cpuMHz("/proc/cpuinfo", cpuMHzName);
        PROCINFO pinfo_MTotal("/proc/meminfo", "MemTotal");
        string cpuMHz = pinfo_cpuMHz.get();
        string MTotal = pinfo_MTotal.get();   
        if(MTotal == "" || cpuMHz == ""){
            printf("[%u] /proc/cpuinfo /proc/meminfo 读取错误!\n", getpid());
            return false;
        }
        CSP_AUTH pack((u_short)(atoi(cpuMHz.c_str())),(u_short)(((double)(atoi(MTotal.c_str())))/1024), sinfo->devid);
        //将pack加密
        u_int random_num;
        encrypt_client_auth( (u_char*)(&pack), random_num);
        pack.random_num = htonl(random_num);

        if(!netpack->mk_databuf(sizeof(CSP_AUTH)))
            return false;
        netpack->head.data_size = sizeof(CSP_AUTH);
        netpack->head.pack_size = 8+sizeof(CSP_AUTH);
        memcpy(netpack->databuf, &pack, sizeof(pack));
        if(!netpack->upload(sinfo))
            return false;
        //_mylog.pack_tolog(DIR_SEND, "发认证串及基本配置信息", 8 + sizeof(pack));
        //printf("[%d] 封包pack_auth成功结束！\n", getpid());  
        
        return true;
    }
    bool mkpack_sysif(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //检查缓冲区空间是否足够
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8+(int)sizeof(CSP_SYSIF))
            return false;
        //printf("[%d] 封包pack_sysif\n", getpid()); 
        
        PROCINFO pinfo_cpu("/proc/stat", "cpu");
        string cpu = pinfo_cpu.get();
        char buf[5]; u_int ut, nt, st, it;
        sscanf(cpu.c_str(), " %s %u %u %u %u", buf, &ut, &nt, &st, &it);
        u_int m1, m2, m3;
        PROCINFO pinfo_m1("/proc/meminfo", "MemFree");
        PROCINFO pinfo_m2("/proc/meminfo", "MemAvailable");
        PROCINFO pinfo_m3("/proc/meminfo", "Cached");
        m1 = atol(pinfo_m1.get().c_str());
        m2 = atol(pinfo_m2.get().c_str());
        m3 = atol(pinfo_m3.get().c_str());

        CSP_SYSIF pack(ut, nt, st, it, m1+m2+m3);
        if(!netpack->mk_databuf(sizeof(CSP_SYSIF)))
            return false;
        netpack->head.data_size = sizeof(CSP_SYSIF);
        netpack->head.pack_size = 8 + sizeof(CSP_SYSIF);
        memcpy(netpack->databuf, &pack, sizeof(pack));
        if(!netpack->upload(sinfo))
            return false;
        //_mylog.pack_tolog(DIR_SEND, "发系统信息", 8 + sizeof(pack));
        return true;
    }
    bool mkpack_str(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //printf("[%d] 封包pack_str\n", getpid());   

        int max_size = 8191;
        FILE * fp = NULL;
        if(netpack->head.head_index == 0x03){
            fp = fopen(CONF_DAT_PATH, "r");
        }else if(netpack->head.head_index == 0x04){
            fp = fopen(PROC_DAT_PATH, "r");
        }else if(netpack->head.head_index == 0x0c){
            fp = fopen(USBF_DAT_PATH, "r");        
            max_size = 4095;
        }else if(netpack->head.head_index == 0x0d){
            max_size = 0;
        }

        if(max_size && !fp){
            printf("[%d] mkpack_strm faild! 文件打开失败\n", getpid());
            return false;
        }

        u_char buf[8191];
        char c;
        int  i = 0;
        while(max_size && i){
            c = fgetc(fp);
            if(feof(fp) || i >= max_size)
                break;
            buf[i++] = c;
        }
        buf[i] = '\0';
        if(fp) fclose(fp);

        if(SEND_BUFSIZE - sinfo->sendbuf_len < i+1+8)
            return false;
        if(!netpack->mk_databuf(i+1))
            return false;
        netpack->head.data_size = i+1;
        netpack->head.pack_size = i+1+8;
        memcpy(netpack->databuf, buf, i+1);
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_eth(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //printf("[%d] 封包pack_eth\n", getpid());   

        int i = 0;//当前包的 pos
        for(i = 0; CPINFO[i].no != -99; ++i)
            if(CPINFO[i].head == 0x0591)
                break;
        if(!i || CPINFO[i].no == -99)
            return false;//为找到当前包
        while(1){
            //检查空间是否足够
            if(SEND_BUFSIZE-sinfo->sendbuf_len < 8 + (int)sizeof(CSP_ETHIF))
                return false;
            //取得当前的包头pad
            netpack->head.pad = CPINFO[i].hpif->head_pad;
            
            //printf("[%d] test in mkpack_eth 当前封包head_pad=0x%x", getpid(), netpack->head.pad);
            //取得包数据
            CSP_ETHIF pack(netpack->head.pad, sinfo->devid);
            if(!netpack->mk_databuf(sizeof(CSP_ETHIF)))
                return false;
            netpack->head.data_size = sizeof(CSP_ETHIF);
            netpack->head.pack_size = 8 + sizeof(CSP_ETHIF);
            memcpy(netpack->databuf, &pack, sizeof(pack));
            //上传包至 sendbuf
            if(!netpack->upload(sinfo))
                return false;
            //hpif 后移
            HPADINFO* hp = CPINFO[i].hpif;
            CPINFO[i].hpif = CPINFO[i].hpif->next;
            delete hp;
            //检查是否需要继续 mkpack
            if(!CPINFO[i].hpif)
                break;
        }
        return true;      
    }
    bool mkpack_usb(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //printf("[%d] 封包pack_usb\n", getpid());   

        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8+(int)sizeof(CSP_USBIF))
            return false;
        CSP_USBIF pack(sinfo->devid);
        if(!netpack->mk_databuf(sizeof(CSP_USBIF)))
            return false;
        netpack->head.data_size = sizeof(CSP_USBIF);
        netpack->head.pack_size = 8 + sizeof(CSP_USBIF);
        memcpy(netpack->databuf, &pack, sizeof(pack));
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_prn(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //printf("[%d] 封包pack_prn\n", getpid());   

        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8+(int)sizeof(CSP_PRNIF))
            return false;
        CSP_PRNIF pack(sinfo->devid);
        if(!netpack->mk_databuf(sizeof(CSP_PRNIF)))
            return false;
        netpack->head.data_size = sizeof(CSP_PRNIF);
        netpack->head.pack_size = 8 + sizeof(CSP_PRNIF);
        memcpy(netpack->databuf, &pack, sizeof(pack));
        if(!netpack->upload(sinfo))
            return false;
        
        return true;
    }
    bool mkpack_tty(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //printf("[%d] 封包pack_tty\n", getpid());   
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8 + (int)sizeof(CSP_TTYIF))
            return false;
        CSP_TTYIF pack(sinfo->devid, _conf_mindn, _conf_maxdn);
        if(!netpack->mk_databuf(sizeof(CSP_TTYIF)))
            return false;
        netpack->head.data_size = sizeof(CSP_TTYIF);
        netpack->head.pack_size = 8 + sizeof(CSP_TTYIF);
        memcpy(netpack->databuf, &pack, sizeof(pack));
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_mtsn(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //printf("[%d] 封包pack_mtsn\n", getpid());   
        int i = 0;//当前包的 pos
        for(i = 0; CPINFO[i].no != -99; ++i)
            if(CPINFO[i].head == 0x0a91)
                break;
        if(!i || CPINFO[i].no == -99)
            return false;//未找到当前包
        while(1){
            //得到 ttyid
            netpack->head.pad = (CPINFO[i].hpif)->head_pad;
            u_char ttyid = (u_char)(netpack->head.pad);
            //计算 screen_num
            u_char snum = _conf_minsn + rand()%(_conf_maxsn-_conf_minsn + 1);
            //printf("[!!!] snum=%d\n", snum);
            //判读缓冲区是否足够
            int data_size = sizeof(CSP_TSNIF) + snum*sizeof(CSP_SNIF);
            if(SEND_BUFSIZE - sinfo->sendbuf_len < 8 + data_size)
                return false;
            if(!netpack->mk_databuf(data_size))
                return false;
            netpack->head.data_size = (u_short)data_size;
            netpack->head.pack_size = 8 + (u_short)data_size;
            //终端信息包装入
            CSP_TSNIF pack(snum, 0, ttyid);
            int databuf_p = 0;
            memcpy(netpack->databuf, &pack, sizeof(pack));
            databuf_p += sizeof(pack);
            //得到ip
            unsigned char d1,d2,d3,d4;
            sscanf(_conf_ip, "%hhu.%hhu.%hhu.%hhu", &d4, &d3, &d2, &d1);
            //printf("[%d] test _conf_ip 转点分十进制ip d4=%u, d3=%u, d2=%u, d1=%u\n", getpid(), d4,d3,d2,d1);
            u_char ip[4] = {d4,d3,d2,d1};
            //虚屏包装入
            for(int i = 0; i < (int)snum; ++i){
                CSP_SNIF snpack((u_char)(i+1), (u_short)_conf_port, ip);
                memcpy(netpack->databuf+databuf_p, &snpack, sizeof(snpack));
                databuf_p+=sizeof(snpack);
            }
            if(!netpack->upload(sinfo))
                return false;
            //统计信息
            _sum_tty+=1;
            _sum_scr+=snum;
            //hpif后移
            HPADINFO* hp = CPINFO[i].hpif;
            CPINFO[i].hpif = CPINFO[i].hpif->next;
            delete hp;
            //检查是否需要继续 mkpack
            if(!CPINFO[i].hpif)
                break;
        }
        return true;    
    }
    bool mkpack_itsn(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {   //不定长的包
        //printf("[%d] 封包pack_itsn\n", getpid());   
        int i = 0;//当前包的 pos
        for(i = 0; CPINFO[i].no != -99; ++i)
            if(CPINFO[i].head == 0x0b91)
                break;
        if(!i || CPINFO[i].no == -99)
            return false;//未找到当前包
        while(1){
            //得到 ttyid
            netpack->head.pad = (CPINFO[i].hpif)->head_pad;
            u_char ttyid = (u_char)(netpack->head.pad);
            //计算 screen_num
            u_char snum = _conf_minsn + rand()%(_conf_maxsn-_conf_minsn + 1);
            //printf("[!!!] snum=%d\n", snum);
            //判读缓冲区是否足够
            int data_size = sizeof(CSP_TSNIF) + snum*sizeof(CSP_SNIF);
            if(SEND_BUFSIZE - sinfo->sendbuf_len < 8 + data_size)
                return false;
            if(!netpack->mk_databuf(data_size))
                return false;
            netpack->head.data_size = (u_short)data_size;
            netpack->head.pack_size = 8 + (u_short)data_size;
            //终端信息包装入
            CSP_TSNIF pack(snum, 1, ttyid);
            int databuf_p = 0;
            memcpy(netpack->databuf, &pack, sizeof(pack));
            databuf_p += sizeof(pack);
            //得到ip
            unsigned char d1,d2,d3,d4;
            sscanf(_conf_ip,"%hhu.%hhu.%hhu.%hhu", &d4, &d3, &d2, &d1);
            //printf("[%d] test _conf_ip 转点分十进制ip d4=%u, d3=%u, d2=%u, d1=%u\n", getpid(), d4,d3,d2,d1);
            u_char ip[4] = {d4,d3,d2,d1};
            //虚屏包装入
            for(int i = 0; i < (int)snum; ++i){
                CSP_SNIF snpack((u_char)(i+1), (u_short)_conf_port, ip);
                memcpy(netpack->databuf+databuf_p, &snpack, sizeof(snpack));
                databuf_p+=sizeof(snpack);
            }
            if(!netpack->upload(sinfo))
                return false;
            //统计信息
            _sum_tty+=1;
            _sum_scr+=snum;
            //hpif后移
            HPADINFO* hp = CPINFO[i].hpif;
            CPINFO[i].hpif = CPINFO[i].hpif->next;
            delete hp;
            //检查是否需要继续 mkpack
            if(!CPINFO[i].hpif)
                break;
        }
        return true;   
    }
    bool mkpack_done(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //检查缓冲区空间是否足够
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8)
            return false;
        //printf("[%d] 封包pack_done\n", getpid());   
        netpack->head.data_size = 0;
        netpack->head.pack_size = 8;
        if(!netpack->upload(sinfo))
            return false;
        return true;   
    }
    bool mkpack_err(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        return true;
    }

    bool chkpack_auth (SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        //printf("[%d] test enter chkpack_auth\n", getpid());
        SCP_AUTH * pack = (SCP_AUTH*) netpack->databuf;
        pack->vno_main = ntohs(pack->vno_main);
        pack->dt_fail = ntohs(pack->dt_fail);
        pack->dt_succ = ntohs(pack->dt_succ);
        pack->random_num = ntohs(pack->random_num);
        pack->svr_time = ntohs(pack->svr_time);


        //验证与处理    
        //1、认证串解密
        if(!decrypt(pack->key, pack->random_num, pack->svr_time)){
            close(sinfo->sockfd);
            exit(8); //认证非法
        }
        //2、数字证书检查
        struct tm chk_tm = {2017-1900, 0, 0, 0, 0, 0};
        u_int chk_time = mktime(&chk_tm);
        if(pack->svr_time < chk_time){
            close(sinfo->sockfd);
            exit(7);//数字证书过期
        }
        //3、检查版本号
        if(pack->vno_main < 2){
            for(int i = 0; CPINFO[i].no != -99; ++i)
                if(CPINFO[i].head == 0x0091){
                    CPINFO[i].status = PACK_UNDO; //发送最低版本要求报文
                    break;
                }
        }
        //test
        //CPINFO[0].status = PACK_UNDO; //发送最低版本要求报文

        for(int i = 0; CPINFO[i].no != -99; ++i){
            if(CPINFO[i].head == 0x0191){
                //printf("test PACK_UNDO SET\n");
                CPINFO[i].status = PACK_UNDO; //发认证串及基本配置信息
                break;
            }
        }
        //printf("[%d] test chkpack_auth RIGHT!\n", getpid());
        return true;
    }
    bool chkpack_fetch (SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        u_short head = ((netpack->head).head_index << 8) | ((netpack->head).head_type);
        u_short head_pad = (netpack->head).pad;
        
        //printf("[%d] test head in chkpack_fetch if is right : 0x%x\n", getpid(), head);
        int bno = 0;
        for(int i = 0; SPINFO[i].no != 99; ++i){
            if(SPINFO[i].head == head){
                bno = SPINFO[i].bno;
                break;
            }
        }
        if(!bno) return false; //SP中无此 head
        for(int i = 0; CPINFO[i].no != -99; ++i){
            if(CPINFO[i].no == bno){
                CPINFO[i].status = PACK_UNDO;
                if(CPINFO[i].ispad){
                    HPADINFO * hp = new (nothrow)HPADINFO;
                    if(!hp){
                        printf("[%d] new HPADINFO faild！动态申请空间失败!\n", getpid());
                        return false;
                    }
                    hp->head_pad = head_pad;
                    hp->next = CPINFO[i].hpif;
                    CPINFO[i].hpif = hp;
                }
                return true;
            }
        }
        return false; //CP中无此 no
    }
    bool chkpack_err (SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
    {
        return true;
    }

};

#endif