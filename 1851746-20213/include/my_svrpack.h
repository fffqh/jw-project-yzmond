#ifndef MY_SVRPACK_H
#define MY_SVRPACK_H

#include <unistd.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <sys/socket.h>

#include "./my_socket.h"
#include "./my_pack.h"
#include "./my_mysql.h"
#include "./my_encrpty.h"

#define PACK_STOP -2  //阻塞等待中
#define PACK_UNDO -1  //未处理完成
#define PACK_EMPTY 0  //空状态
#define PACK_HALF  1  //未接收/发送完成


extern int32_t _conf_trangap;
extern int32_t _conf_conngap;
extern MYMDB   _mymdb;

struct CLTPACK;
struct SEVPACK;
class  SEV_PACK_ACTION;

struct HPADINFO{
    u_short head_pad;
    HPADINFO* next;
};

struct SEVPACK{
    const int        no;
    const int        ispad; //0表示无需回复
    HPADINFO*        hpif; //待发送包序列的head_pad
    bool (SEV_PACK_ACTION::*pack_fun)(SOCK_INFO*, NETPACK*); //封包函数
    const u_short   head;
    const char      *str;
    int             status;
};
struct CLTPACK{
    const int        no;
    const int        bno;
    bool (SEV_PACK_ACTION::*unpack_fun)(SOCK_INFO*, NETPACK*); //解包函数
    const u_short    head;
    const char      *str;
    int              status;
};
class SEV_PACK_ACTION{
public:
    SOCK_INFO* sinfo;
    SEVPACK* SINFO;
    CLTPACK* CINFO;
    u_short  ram;
    SEV_PACK_ACTION(SOCK_INFO* s,SEVPACK* SP,CLTPACK* CP)
    {
        sinfo = s;
        SINFO = SP;
        CINFO = CP;
    }

    //服务端：封包拆包并进行相应处理函数
    bool mkpack_auth(SOCK_INFO* s, NETPACK* netpack)
    {
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8 + (int)sizeof(SCP_AUTH))
            return false;
        
        SCP_AUTH pack(3,0,0, _conf_conngap, _conf_trangap);

        if(!netpack->mk_databuf(sizeof(SCP_AUTH)))
            return false;
        netpack->head.data_size = sizeof(SCP_AUTH);
        netpack->head.pack_size = 8+sizeof(SCP_AUTH);
        memcpy(netpack->databuf, &pack, sizeof(pack));
        if(!netpack->upload(sinfo))
            return false;
        printf("[%d] 封包pack_auth成功结束！\n", getpid());  
        return true;
    }
    bool mkpack_sysif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8)
            return false;

        int i = 0;//当前包的 pos
        for(i = 0; SINFO[i].no != -99; ++i)
            if(SINFO[i].head == 0x0211)
                break;
        if(SINFO[i].no == -99)
            return false;//未找到当前包        

        netpack->head.data_size = 0;
        netpack->head.pack_size = 8;
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_cfif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8)
            return false;

        int i = 0;//当前包的 pos
        for(i = 0; SINFO[i].no != -99; ++i)
            if(SINFO[i].head == 0x0311)
                break;
        if(SINFO[i].no == -99)
            return false;//未找到当前包        

        netpack->head.data_size = 0;
        netpack->head.pack_size = 8;
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_pcif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8)
            return false;

        int i = 0;//当前包的 pos
        for(i = 0; SINFO[i].no != -99; ++i)
            if(SINFO[i].head == 0x0411)
                break;
        if(SINFO[i].no == -99)
            return false;//未找到当前包        

        netpack->head.data_size = 0;
        netpack->head.pack_size = 8;
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_ethu(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8)
            return false;
        
        int i = 0;//当前包的 pos
        for(i = 0; SINFO[i].no != -99; ++i)
            if(SINFO[i].head == 0x0511)
                break;
        if(SINFO[i].no == -99)
            return false;//未找到当前包        

        while(1){
            netpack->head.pad = (SINFO[i].hpif)->head_pad;
            netpack->head.data_size = 0;
            netpack->head.pack_size = 8;
            if(!netpack->upload(sinfo))
                return false;
            HPADINFO* hp = SINFO[i].hpif;
            SINFO[i].hpif = SINFO[i].hpif->next;
            delete hp;
            if(!SINFO[i].hpif)
                break;
        }
        return true;
    }
    bool mkpack_usbif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8)
            return false;

        int i = 0;//当前包的 pos
        for(i = 0; SINFO[i].no != -99; ++i)
            if(SINFO[i].head == 0x0711)
                break;
        if(SINFO[i].no == -99)
            return false;//未找到当前包        

        netpack->head.data_size = 0;
        netpack->head.pack_size = 8;
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_prnif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8)
            return false;

        int i = 0;//当前包的 pos
        for(i = 0; SINFO[i].no != -99; ++i)
            if(SINFO[i].head == 0x0811)
                break;
        if(SINFO[i].no == -99)
            return false;//未找到当前包        

        netpack->head.data_size = 0;
        netpack->head.pack_size = 8;
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_ttyif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        if(SEND_BUFSIZE - sinfo->sendbuf_len < 8)
            return false;

        int i = 0;//当前包的 pos
        for(i = 0; SINFO[i].no != -99; ++i)
            if(SINFO[i].head == 0x0911)
                break;
        if(SINFO[i].no == -99)
            return false;//未找到当前包        

        netpack->head.data_size = 0;
        netpack->head.pack_size = 8;
        if(!netpack->upload(sinfo))
            return false;
        return true;
    }
    bool mkpack_err(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        return true;
    }
    
    bool unpack_auth(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        CSP_AUTH * pack = (CSP_AUTH*) netpack->databuf;

        //报文解密，认证与检查
        decrypt_client_auth(netpack->databuf, pack->random_num);
        char keybuf[33] = {0};
        memcpy(keybuf, pack->key, 32);
        if(strcmp(keybuf, "yzmond:id*str&to!tongji@by#Auth^")){
            printf("[%d] unpack_auth failed! (认证串不匹配)\n", getpid());
            _mymdb.close();
            exit(8);
        }

        sinfo->devid = (u_int)ntohl(pack->devid);
        //整理数据至db
        string data[ENUM_base_prnnum + 1];
        ostringstream devidout;
        devidout << (u_int)(sinfo->devid);
        data[ENUM_base_devid] = devidout.str();
        if(!_mymdb.new_base(data[ENUM_base_devid]))
            return false;
        
        data[ENUM_base_devno] = string("1");
        
        char DateTimebuf[50] = {0};
        GetDateTime(DateTimebuf);
        data[ENUM_base_time]  = string(DateTimebuf);
        
        GetClientIP(sinfo->sockfd, data[ENUM_base_ipaddr]);
        ostringstream out_sid;
        out_sid << pack->gid << "-" << ntohs(pack->dev_inid);
        data[ENUM_base_sid] = out.str();
        data[ENUM_base_type] = string(pack->dev_tid);
        data[ENUM_base_version] = string(pack->dev_vid);

        ostringstream out;
        out << (u_short)ntohs(pack->cpuMHz);
        data[ENUM_base_cpu] = out.str();
        out.clear();
        out << (u_short)ntohs(pack->MTotal);
        data[ENUM_base_sdram]       = out.str();
        out.clear();
        out << (u_short)ntohs(pack->ROM);
        data[ENUM_base_flash]       = out.str();
        out.clear();
        out << (u_short)ntohs(pack->ethnum);
        data[ENUM_base_ethnum]      = out.str();
        out.clear();
        out << (u_short)ntohs(pack->syncnum);
        data[ENUM_base_syncnum]     = out.str();
        out.clear();
        out << (u_short)ntohs(pack->asyncnum);
        data[ENUM_base_asyncnum]    = out.str();
        out.clear();
        out << (u_short)ntohs(pack->switchnum);
        data[ENUM_base_switchnum]   = out.str();
        out.clear();
        out << (u_short)ntohs(pack->subnum);
        data[ENUM_base_usbnum]      = out.str();
        out.clear();
        out << (u_short)ntohs(pack->prnnum);
        data[ENUM_base_prnnum]      = out.str();

        this->ram = (u_short)ntohs(pack->MTotal);

        //写入数据库中的 base 队列
        if(!_mymdb.add_base( data[ENUM_base_devid], base(ENUM_base_devno), base(ENUM_base_prnnum), data + 1)){
            printf("[%d] unpack_auth failed! mymdb.add_base 失败！\n", getpid());
            return false;
        }
        //将相关包置UNDO
        for(int i = 0; SINFO[i].no != -99; ++i){
            // 系统信息、配置信息、进程信息
            if(SINFO[i].head == 0x0211 || SINFO[i].head == 0x0311 || SINFO[i].head == 0x0411)
                SINFO[i].status = PACK_UNDO;
            // 以太口信息（需要头部 pad）
            if(SINFO[i].head == 0x0511){
                SINFO[i].status = PACK_UNDO;
                for(u_short ethid = 0; ethid < ntohs(pack->ethnum); ++ethid){
                    HPADINFO * hp = new (nothrow)HPADINFO;
                    if(!hp){
                        printf("[%d] new HPADINFO faild！动态申请空间失败!\n", getpid());
                        return false;
                    }
                    hp->head_pad = ethid;
                    hp->next = CPINFO[i].hpif;
                    CPINFO[i].hpif = hp;
                }
            }
            // USB口信息、打印口信息、终端服务信息
            if(SINFO[i].head == 0x0711 || SINFO[i].head == 0x0811 || SINFO[i].head == 0x0911)
                SINFO[i].status = PACK_UNDO;
        }

        string switchinfo = "NULL";
        if(!_mymdb.add_base( data[ENUM_base_devid], base(ENUM_base_switchinfo), base(ENUM_base_switchinfo), &switchinfo)){
            printf("[%d] unpack_auth failed! mymdb.add_base 失败！\n", getpid());
            return false;        
        }
        return true;
    }
    bool unpack_sysif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        CSP_SYSIF * pack = (CSP_SYSIF*) netpack->databuf;
        string data[2];
        double cpu_used;
        u_int ut = ntohl(pack->user_cputime);
        u_int nt = ntohl(pack->nice_cputime);
        u_int st = ntohl(pack->sys_cputime);
        u_int it = ntohl(pack->idle_cputime);        
        cpu_used = (double)(ut+st)/(ut+nt+st+it);
        double sdram_used;
        u_int fm_MB = (u_int)ntohl(pack->freed_memory)/1024;
        sdram_used = (double)fm_MB/this->ram;

        ostringstream out;
        out << cpu_used;
        data[0] = out.str();
        out.clear();
        out << sdram_used;
        data[1] = out.str();
        out.clear();
        out << (u_int)(sinfo->devid);
        if(!_mymdb.add_base(out.str(), base(ENUM_base_cpu_used), base(ENUM_base_sdram_used), data)){
            printf("[%d] unpack_sysif failed! mymdb.add_base 失败！\n", getpid());
            return false;
        }
        return true;
    }
    bool unpack_cfif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        char *buf = new char[8192];
        if(!buf){
            printf("[%d] unpack_cfif failed！动态空间申请失败！\n", getpid());
            return false;
        }
        u_short dsize = (u_short)ntohs(netpack->head.data_size);
        memcpy(buf, netpack->databuf, dsize);
        buf[dsize + 1] = '\0';

        ostringstream out;
        out << buf;
        string data = out.str();
        out.clear();
        out << (u_short)(sinfo->devid);

        delete [] buf;

        if(!_mymdb.add_base(out.str(), base(ENUM_base_config), base(ENUM_base_config), &data)){
            printf("[%d] unpack_cfif failed! mymdb.add_base 失败！\n", getpid());
            return false;
        }
        return true;
    }
    bool unpack_pcif(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        char * buf = new char[8192];
        if(!buf){
            printf("[%d] unpack_pcif failed！动态空间申请失败！\n", getpid());
            return false;
        }
        u_short dsize = (u_short)ntohs(netpack->head.data_size);
        memcpy(buf, netpack->databuf, dsize);
        buf[dsize + 1] = '\0';

        ostringstream out;
        out << buf;
        string data = out.str();
        out.clear();
        out << (u_short) (sinfo->devid);
        delete [] buf;

        if(!_mymdb.add_base(out.str(), base(ENUM_base_process), base(ENUM_base_process), &data)){
            printf("[%d] unpack_pcif failed！mymdb.add_base 失败！\n", getpid());
            return false;
        }
        return true;
    }
    bool unpack_ethu(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        
    }
    bool unpack_usb(SOCK_INFO* sinfo, NETPACK* netpack)
    {
        
    }
    bool unpack_prn(SOCK_INFO* sinfo, NETPACK* netpack)
    {

    }
    bool unpack_usbf(SOCK_INFO* sinfo, NETPACK* netpack)
    {

    }
    bool unpack_prnl(SOCK_INFO* sinfo, NETPACK* netpack)
    {

    }
    bool unpack_ttyif(SOCK_INFO*sinfo, NETPACK* netpack)
    {

    }
    bool unpack_mtsn(SOCK_INFO*sinfo, NETPACK* netpack)
    {

    }
    bool unpack_itsn(SOCK_INFO*sinfo, NETPACK* netpack)
    {

    }
    bool unpack_ok(SOCK_INFO*sinfo, NETPACK* netpack)
    {

    }

    bool unpack_err(SOCK_INFO* s, NETPACK* netpack)
    {
        return true;
    }
private:

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
    void GetClientIP(int connfd, string& dev_ip)
    {
        struct sockaddr_in  peerAddr; //监听地址、连接的本地地址、连接的外地地址
        socklen_t peerAddrLen;

        //输出外地连接信息
        peerAddrLen=sizeof(peerAddr);
        getpeername(connfd, (struct sockaddr *)&peerAddr, &peerAddrLen);
        printf("[%d] 与设备连接成功（ %s:%d）\n", getpid(),\
                inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
        dev_ip = string(inet_ntoa(peerAddr.sin_addr));
    }

};

#endif
