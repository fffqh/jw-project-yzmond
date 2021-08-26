#ifndef MY_PACK_H
#define MY_PACK_H

#include <iostream>
#include <stdio.h>
#include <iomanip>
#include <string>
#include <string.h>
#include <unistd.h>
#include <ctype.h> //�жϿɼ��ַ�
#include <sstream> //�ַ���������
#include <algorithm> //�������random_shufft
#include <vector>
#include <sys/socket.h>

using namespace std;

#include "my_socket.h"
#include "my_getproc.h"
#include "my_log.h"

extern MYLOG _mylog;

// __HEAD_PACK
// +-----------+------------+----------------------+
// | head_type | head_index | pack_size            |          
// +-----------+------------+----------------------+
// | pad                    | data_size            |
// +------------------------+----------------------+

struct _PK_{
    u_char head_type;
    u_char head_index;
    const char* str;
    u_char sub;
};

_PK_ MYPK[] = {
    {0x91, 0x00, "����Ͱ汾Ҫ��",0},
    {0x91, 0x01, "����֤��������������Ϣ",0},
    {0x91, 0x02, "��ϵͳ��Ϣ",0},
    {0x91, 0x03, "��������Ϣ",0},
    {0x91, 0x04, "��������Ϣ",0},
    {0x91, 0x05, "����̫����Ϣ",1},
    {0x91, 0x07, "��USB����Ϣ",0},
    {0x91, 0x0c, "��U���ļ��б���Ϣ",0},
    {0x91, 0x08, "����ӡ����Ϣ",0},
    {0x91, 0x0d, "����ӡ������Ϣ",0},
    {0x91, 0x09, "���ն˷�����Ϣ",0},
    {0x91, 0x0a, "�����ն���Ϣ",1},
    {0x91, 0x0b, "��IP�ն���Ϣ",1},
    {0x91, 0xff, "���а����յ�",0},
    {0x00, 0x00, "ERR", 0}
};

struct _PKINFO_{
    _PK_* PKLST;
    const char* getstr(u_char hty, u_char hid)
    {
        for(int i = 0; PKLST[i].head_type != 0x00; ++i)
            if(PKLST[i].head_type == hty && PKLST[i].head_index == hid)
                return PKLST[i].str;
        return NULL;
    }
    u_char is_sub(u_char hty, u_char hid)
    {
        for(int i = 0; PKLST[i].head_type != 0x00; ++i)
            if(PKLST[i].head_type == hty && PKLST[i].head_index == hid)
                return PKLST[i].sub;
        return 0;        
    }

};

_PKINFO_ MYPKINFO = {MYPK};

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
    u_char name[32];
    CSP_PRNIF(u_int devid){
        is_live = (u_char)((devid/10) %2);
        if(is_live){
            work_num = htons((devid % 100) % 25);
        }else{
            work_num = htons(0);
        }
        memset(name, 0, sizeof(name));
        memcpy(name, "PRN-fqh-1746", 12);
    }
};

struct CSP_ETHIF{
    u_char is_live;  u_char is_set;  u_char UpOrDw; u_char pad1;
    u_char mac[6]; u_short options;
    u_int  ipaddr, mask;
    u_int  ipaddr_1, mask_1;
    u_int  ipaddr_2, mask_2;
    u_int  ipaddr_3, mask_3;
    u_int  ipaddr_4, mask_4;
    u_int  ipaddr_5, mask_5;
    u_int  stat_data[16]; //recv + send

    CSP_ETHIF(u_short hpad, u_int devid)
    {
        is_live = 1;
        is_set  = 1;
        UpOrDw  = 1;
        // 00:01:6C:06:A6:xx
        mac[0] = 0x00;
        mac[1] = 0x01;
        mac[2] = 0x6c;
        mac[3] = 0x06;
        mac[4] = 0xa6;
        mac[5] = devid%100;
        // 100MB + ȫ˫�� + �Զ�Э��
        options = htons(0x0007); 
        // ip + mask
        ipaddr = htonl((((u_int)(devid%100)*1000 + 168)*1000 + 89)*1000 + 99);
        ipaddr_1 = htonl((((u_int)(devid%100)*1000 + 168)*1000 + 89)*1000 + 1);
        ipaddr_2 = htonl((((u_int)(devid%100)*1000 + 168)*1000 + 89)*1000 + 2);
        ipaddr_3 = htonl((((u_int)(devid%100)*1000 + 168)*1000 + 89)*1000 + 3);
        ipaddr_4 = htonl((((u_int)(devid%100)*1000 + 168)*1000 + 89)*1000 + 4);
        ipaddr_5 = htonl((((u_int)(devid%100)*1000 + 168)*1000 + 89)*1000 + 5);
        mask = htonl((((u_int)(255*1000 + 255)*1000 + 255)*1000 + 0));
        mask_1 = htonl((((u_int)(255*1000 + 255)*1000 + 255)*1000 + 0));
        mask_2 = htonl((((u_int)(255*1000 + 255)*1000 + 255)*1000 + 0));
        mask_3 = htonl((((u_int)(255*1000 + 255)*1000 + 255)*1000 + 0));
        mask_4 = htonl((((u_int)(255*1000 + 255)*1000 + 255)*1000 + 0));
        mask_5 = htonl((((u_int)(255*1000 + 255)*1000 + 255)*1000 + 0));
        
        char proc_name[6] = {0};
        if(hpad == 0x0000){
            memcpy(proc_name, "ens32", 5);
        }else if(hpad == 0x0001){
            memcpy(proc_name, "lo", 2);
        }else{
            printf("[%d] Create CSP_ETHIF failed���Ƿ���pad_head��\n", getpid());
            return;
        }
        //recv+send
        PROCINFO pinfo("/proc/net/dev", proc_name);
        string eth_info = pinfo.get();
        istringstream info(eth_info);
        u_int d;
        for(int i = 0; i < 16; ++i){
            info >> d;
            stat_data[i] = htonl(d);
        }

    }

};

struct CSP_TTYIF{
    u_char mtty[16];
    u_char tty[254];
    u_short num;
    CSP_TTYIF(u_int devid, int mindn, int maxdn)
    {
        int total = mindn + rand()%( maxdn- mindn + 1 );
        int async_num = 8*(devid/10%3);
        int async_term_num = 0;
        if(async_num){
            async_term_num = 1 + rand()%async_num;
        }
        total = (total >= async_term_num)? total : async_term_num;
        int ip_term_num = total - async_term_num;

        //init
        memset(mtty, 0, sizeof(mtty));
        memset(tty, 0, sizeof(tty));
        //mtty rand_set
        vector<int> temp;
        for(int i = 0; i < async_num; ++i){
            temp.push_back(i);
        }
        random_shuffle(temp.begin(), temp.end());
        for(int i = 0; i < async_term_num; ++i){
            mtty[temp[i]] = 1;
        }
        //tty rand_set
        temp.clear();
        for(int i = 0; i < 254; ++i){
            temp.push_back(i);
        }
        random_shuffle(temp.begin(), temp.end());
        for(int i = 0; i < ip_term_num; ++i){
            tty[temp[i]] = 1;
        }
        num = htons((u_short)(total + rand()%(270-total + 1)));
    }
};

struct CSP_TSNIF{
    u_char port;
    u_char sport;
    u_char live_sno;
    u_char num_sno;
    u_int  ttyip;
    u_char tty_type[12];
    u_char tty_state[8];
    CSP_TSNIF(u_char snum, bool isip, u_char ttyid){
        port =  ttyid;
        sport = ttyid;
        num_sno = snum;
        //printf("[!!!] num_sno=%u\n", snum);
        live_sno = rand()%(snum);
        
        memset(tty_type, 0, sizeof(tty_type));
        memset(tty_state, 0, sizeof(tty_state));
        
        if(isip){
            ttyip = htonl((u_int)192168089000 + ttyid); //192.168.89.x
            memcpy(tty_type, "IP�ն�", sizeof("IP�ն�")); 
        }else{
            ttyip = htonl(0);
            memcpy(tty_type, "�����ն�", sizeof("�����ն�"));
        }
        
        if(rand()%2){
            memcpy(tty_state, "����", sizeof("����"));
        }else{
            memcpy(tty_state, "�˵�", sizeof("�˵�"));
        }
    }
};


struct CSP_SNIF{
    u_char  snid;
    u_char  pad1;
    u_short svr_port;
    u_int   svr_ip;
    u_char  sn_pto[12];
    u_char  sn_state[8];
    u_char  sn_info[24];
    u_char  sn_ttyty[12];
    u_int   tyy_time;
    u_int   stoty_byte;
    u_int   rfoty_byte;
    u_int   stosr_byte;
    u_int   rfosr_byte;
    u_int   ping_min;
    u_int   ping_avg;
    u_int   ping_max;
    CSP_SNIF(u_char id, u_short port, u_int ip){
        const char* PTO[] = {"SSH", "ר��SSH", "FTP"};
        const int PTONUM = 3;
        const char* STA[] = {"����", "�ػ�", "�ѵ�¼"};
        const int STANUM = 3;
        const char* SIF[] = {"����ϵͳ", "���𿪻�", "�߳���ҵ", "������ҵ"};
        const int SIFNUM = 4;
        const char* TTY[] = {"vt100", "vt220", "xcode"};
        const int TTYNUM = 3;
        
        int rand_index = 0;
        snid = id;
        svr_port = htons(port);
        svr_ip = htonl(ip);

        rand_index = rand() % PTONUM;
        memcpy(sn_pto, PTO[rand_index], string(PTO[rand_index]).length());
        rand_index = rand() % STANUM;
        memcpy(sn_state, STA[rand_index], string(STA[rand_index]).length());
        rand_index = rand() % SIFNUM;
        memcpy(sn_info, SIF[rand_index], string(SIF[rand_index]).length());
        rand_index = rand() % TTYNUM;
        memcpy(sn_ttyty, TTY[rand_index], string(TTY[rand_index]).length());
        time_t t;
        time(&t);
        tyy_time = htonl((u_int)(t));
        stoty_byte = htonl(rand()%10000000);
        rfoty_byte = htonl(rand()%10000000);
        stosr_byte = htonl(rand()%10000000);
        rfosr_byte = htonl(rand()%10000000);
        ping_min   = htonl(rand()%123456);
        ping_avg   = htonl(rand()%123456);
        ping_max   = htonl(rand()%123456);
    }
};

class NETPACK{
public:
    /*������*/
    HEADPACK head;
    u_char* databuf;
    
    /*���캯��*/
    NETPACK() //���
    {
        head = {0};
        databuf = NULL;
    };
    NETPACK(u_short head_info) //���
    {
        memcpy(&(head.head_type), &head_info, 1);
        memcpy(&(head.head_index), (u_char*)(&head_info) + 1, 1);
        head.pad = 0x0000;
        databuf = NULL;
    }
    /*��������*/
    ~NETPACK(){if(databuf) delete databuf;}

    /*��������*/
    //װ�ذ�
    bool upload(SOCK_INFO* sinfo)
    {
        if( !head.pack_size || SEND_BUFSIZE-sinfo->sendbuf_len < head.pack_size)
            return false;
        u_short dsize = head.data_size;
        u_short psize = head.pack_size;
        
        //tolog
        ostringstream intfbuf;
        intfbuf << MYPKINFO.getstr(head.head_type, head.head_index);
        if(MYPKINFO.is_sub(head.head_type, head.head_index))
            intfbuf << "(" << head.pad <<")";
        //printf("test sig 6th!!!!\n");
        _mylog.pack_tolog(DIR_SEND, intfbuf.str().c_str(), psize);
        //������ת������
        head.data_size = htons(head.data_size);
        head.pack_size = htons(head.pack_size);
        head.pad = htons(head.pad);
        //to sendbuf
        memcpy(sinfo->sendbuf + sinfo->sendbuf_len, &head, psize-dsize);
        if(dsize>0)
            memcpy(sinfo->sendbuf + sinfo->sendbuf_len + psize-dsize, databuf, dsize);
        sinfo->sendbuf_len += psize;
        return true;
    }
    //ж�ذ�
    bool dwload(SOCK_INFO* sinfo, u_short pack_size, u_short data_size )
    {
        if( !pack_size || sinfo->recvbuf_len < pack_size)
            return false;
        //�õ�����ͷ
        memcpy(&head, sinfo->recvbuf, pack_size - data_size);
        head.pack_size = ntohs(head.pack_size);
        head.data_size = ntohs(head.data_size);
        head.pad = ntohs(head.pad);
        //printf("[%d] �õ�����ͷ head_type:0x%02x head_index:0x%02x pack_size:%d data_size:%d\n"
        //        , getpid(), head.head_type, head.head_index, head.pack_size, head.data_size);
        //�õ���������
        databuf = new (nothrow) u_char[data_size];
        if(!databuf){
            printf("[%d] dwload failed����̬����ռ�ʧ��\n", getpid());
            return false;
        }
        memcpy(databuf, sinfo->recvbuf+pack_size-data_size, data_size);
        //printf("[%d] �õ�����(len:%d) data_first:%02x\n"
        //        , getpid(), data_size, databuf[0]);
        //����recvbuf_len
        sinfo->recvbuf_len -= pack_size;
        if(sinfo->recvbuf_len > 0)
            memmove(sinfo->recvbuf, sinfo->recvbuf + pack_size, sinfo->recvbuf_len);
        //to_log
        //data_tolog("./client_recv.log", data_size);
        return true;
    }
    //������ռ�
    bool mk_databuf(int size)
    {
        if(size <= 0)
            return false;
        if(databuf)
            delete databuf;
        databuf = new (nothrow) u_char [size];
        if(!databuf){
            printf("[%d] mk_databuf failed����̬����ռ�ʧ��\n", getpid());
            return false;
        }

        return true;
    }
    void set_headpad(u_short pad)
    {
        head.pad = pad;
    }
private:
    void data_tolog(string log_path, int data_size)
    {
        FILE* fp = NULL;
        fp = fopen(log_path.c_str(), "a"); //׷��д
        if(!fp){
            printf("[%d] data_tolog failed���ļ���ʧ�ܣ�%s��\n", getpid(), log_path.c_str());
            return;
        }

        //��databuf����ʽд��log
        for(int i = 0; i < (data_size + 15)/16; ++i){ //����
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

#endif
