/*
�汾 0.1 
������Ϣд���Ĳ��԰�
�Ȳ��ܶ������ļ��Ŀ����� w
*/

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h> //waitpid
#include <sys/fcntl.h>

#include "../include/my_encrpty.h"
#include "../include/my_socket.h"
#include "../include/my_pack.h"
#include "../include/my_getproc.h"

//��������
#define MAX_RECON_TIME  5
#define MAX_CON_WATIME  25


#define PACK_STOP -2  //�����ȴ���
#define PACK_UNDO -1  //δ�������
#define PACK_EMPTY 0  //��״̬
#define PACK_HALF  1  //δ����/�������

#define CONF_DAT_PATH "./config.dat"
#define PROC_DAT_PATH "./process.dat"
#define USBF_DAT_PATH "./usbfiles.dat"

//�Զ���ṹ��

struct CLTPACK;
struct SEVPACK;
struct SEVPACK{
    const int       no;
    const int       bno; //0��ʾ����ظ�
    bool (*unpack_fun)(SOCK_INFO*, SEVPACK*, CLTPACK*, NETPACK*); //�������
    const u_short   head;
    const char      *str;
    int             status;
};
struct CLTPACK{
    const int       no;
    int             bno; //������
    bool (*pack_fun)(SOCK_INFO*, SEVPACK*, CLTPACK*, NETPACK*); //�������
    const u_short   head;
    const char      *str;
    int             status;
};


//���ò���
char _conf_ip[16] = "192.168.1.242";
int  _conf_port = 51746;
bool _conf_quit = 1;
int  _conf_mindn = 5;
int  _conf_maxdn = 28;
int  _conf_minsn = 3;
int  _conf_maxsn = 10;
bool _conf_newlog = 1;
int  _conf_debug = 111111;
int  _conf_dprint = 1;
//���в���
int  _devid;
int  _devnum;

/** ���������� **/
//chk_arg: �����ò���
bool chk_arg(int argc, char** argv)
{
    if(argc!=3){
        //log
        printf("�����������client�˳�!");
        return false;
    }
    _devid = atoi(argv[1]);
    _devnum = atoi(argv[2]);
    return true;
}
//read_conf: ��ȡ�����ļ�
bool read_conf()
{
    ;
    return true;
}

/** �������źŴ��� **/
// �ӽ����˳�ԭ��˵��
static const char *childexit_to_str(const int no)
{
    struct {
    	const int no;
    	const char *str;
    } signstr[] = {
	{0,  "��ȡ�������,���Զ˹ر�"},
    {1, "����socketʧ��"},
    {2, "����������"},
    {3, "����д����"},
	{5, "����ʧ�ܣ��ﵽ�����������"},  //��CTRL+C�������ǽ��̲��ػ�
    {6, "selectʧ��"},
    {7, "����֤����ڣ����ӹر�"},
    {8, "��֤�Ƿ�"},
    {-99, "END"}
	};

    int i=0;
    for (i=0; signstr[i].no!=-99; i++)
        if (no == signstr[i].no)
            break;

    return signstr[i].str;
}
//SIGCHLD
void fun_waitChild(int no)
{
    int sub_status;
    pid_t pid;
    while ((pid = waitpid(0, &sub_status, WNOHANG)) > 0) {
        if (WIFEXITED(sub_status)){
            printf("child %d exit with %d\n", pid, WEXITSTATUS(sub_status));
            // subprocess_clear_num += 1;
        }
        else if (WIFSIGNALED(sub_status))
            printf("child %d killed by the %dth signal\n", pid, WTERMSIG(sub_status));
    }
}
//�źŲ�׽��ע��
void set_signal_catch()
{
    signal(SIGCHLD, fun_waitChild); //�ӽ����˳��ź�
}

/** TCP�������̿��� **/
int set_socket_flag(int sockfd, int FLAG)
{
    int flags;
    if( (flags = fcntl(sockfd, F_GETFL, NULL)) <0){
        printf("fcntl F_GETFL error:%s(errno: %d) \n", strerror(errno), errno);
        return -1;
    }
    if( fcntl(sockfd, F_SETFL, flags|FLAG) == -1){
        printf("fcntl F_SETFL error:%s(errno: %d) \n", strerror(errno), errno);
        return -1;
    }
    return 1;
}
//����socket
bool create_socket(int & sock_fd, int nonblock = 0)
{
    if( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        //log
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return false;
    }
    if( nonblock && set_socket_flag(sock_fd, O_NONBLOCK) > 0){
        printf("socket ����Ϊ������״̬�ɹ�!\n");
    }
    return true;
}
//��Server����
bool connect_with_limit(int sockfd, int maxc)
{
    struct  sockaddr_in  servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(_conf_port);
    servaddr.sin_addr.s_addr=inet_addr(_conf_ip);

/*connect����˵����
   1�����Ӵ����������������������Ϊ 5 ��
   2��select ��ʱֹͣ��ʽ�ȴ����ӣ����ó�ʱ���ƣ����ȴ�ʱ��Ϊ MAX_CON_WATIME��
***/
    int con_time = 0;
    int n;

    while(con_time++ < MAX_RECON_TIME){//����ʧ�ܣ��������
        //printf("��%d�γ�������\n", con_time);
        n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
        //printf("error:%s(errno:%d)\n", strerror(errno), errno);
        if(n == 0){//�����ɹ���
            //print_func_ret_err("connect", n);
            break;
        }
        else if((n < 0) && (errno == EINPROGRESS)){//connectû�������ɹ��������гɹ���ϣ��
            //ʹ��select��������ʱ��ΪMAX_CON_WATIME
            fd_set writefd_set;
            struct timeval time_out;
            time_out.tv_sec = MAX_CON_WATIME;
            time_out.tv_usec = 0;
            FD_ZERO(&writefd_set);
            FD_SET(sockfd, &writefd_set);

            int select_ret = select(sockfd+1, NULL, &writefd_set, NULL, &time_out);
            if(select_ret <= 0){
                //print_func_ret_err("select_forever", select_ret);
                printf("�� %d ������ʧ��...\n", con_time);
                continue;
            }
            else if(FD_ISSET(sockfd, &writefd_set)){//�����ﲻһ���ɹ�������������жϣ�ѡ���ٴε���connect�������жϣ�
                n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                if(errno != EISCONN){//����ʧ��
                    printf("�� %d ������ʧ��...\n", con_time);
                    continue;
                }
                else{//���ӳɹ�
                    break;
                }
            }
            else{
                printf("�� %d ������ʧ��...\n", con_time);
            }
        }

    }//end of while
    if(con_time >= 5){
        printf("error:%s(errno:%d)\n", strerror(errno), errno);
        printf("���ӳ�ʱ���ͻ����˳���\n");
        return false;
    }
    //connect�ɹ��ˣ�����
    printf("���������ɹ�������ip : %s���˿ڣ�%d \n", _conf_ip, _conf_port);
    errno = 0;
    return true;
}

/** ��¼��־��дһ��ר�����������־�ĺ��� **/
//�õ���ǰϵͳʱ��
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

    //printf("\nCurrent time: %s\n\n", cur_time);
	memcpy(s, cur_time, strlen(cur_time)+1);
    free(cur_time);

    cur_time = NULL;

    return s;
}

/** ����������������� **/
//ר���ӿ�
bool mkpack_vno(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    //��黺�����ռ��Ƿ��㹻
    if(SEND_BUFSIZE - sinfo->sendbuf_len < 12)
        return false;
    printf("[%d] ���pack_vno\n", getpid());   
    
    CSP_VNO pack(2,0,0);
    if(!netpack->mk_databuf(sizeof(CSP_VNO)))
        return false;
    netpack->head.data_size = sizeof(CSP_VNO);
    netpack->head.pack_size = 8 + sizeof(CSP_VNO);
    memcpy(netpack->databuf, &pack, sizeof(pack));
    if(!netpack->upload(sinfo))
        return false;

    return true;
}
bool mkpack_auth(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    //��黺�����ռ��Ƿ��㹻
    if(SEND_BUFSIZE - sinfo->sendbuf_len < 8+(int)sizeof(CSP_AUTH))
        return false;
    printf("[%d] ���pack_auth\n", getpid());  

    PROCINFO pinfo_cpuMHz("/proc/cpuinfo", "cpu MHz");
    PROCINFO pinfo_MTotal("/proc/meminfo", "MemTotal");
    string cpuMHz = pinfo_cpuMHz.get();
    string MTotal = pinfo_MTotal.get();   
    if(MTotal == "" || cpuMHz == "")
        return false;
    
    CSP_AUTH pack((u_short)(atoi(cpuMHz.c_str())),(u_short)(((double)(atoi(MTotal.c_str())))/1024), sinfo->devid);
    //��pack����
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
    return true;
}
bool mkpack_sysif(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    //��黺�����ռ��Ƿ��㹻
    if(SEND_BUFSIZE - sinfo->sendbuf_len < 8+(int)sizeof(CSP_SYSIF))
        return false;
    printf("[%d] ���pack_sysif\n", getpid()); 
    
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
    return true;
}
bool mkpack_str(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    printf("[%d] ���pack_str\n", getpid());   

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
        printf("[%d] mkpack_strm faild! �ļ���ʧ��\n", getpid());
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
    printf("[%d] ���pack_eth\n", getpid());   

    int i = 0;
    for(i = 0; CPINFO[i].no != -99; ++i)
        if(CPINFO[i].head == 0x0591)
            break;
    if(!i || CPINFO[i].no == -99)
        return false;
    
    u_short head_pad;

    while(1){
        if(((u_short)CPINFO[i].bno) & 0x0001)
            head_pad = 0x0000;
        else if(((u_short)CPINFO[i].bno) & 0x0002)
            head_pad = 0x0001;
        else
            break;
        
        printf("[%d] test in mkpack_eth ��ǰ���head_pad=0x%x", getpid(), head_pad);
        netpack->head.pad = htons(head_pad);
        CSP_ETHIF pack(head_pad, sinfo->devid);
        if(!netpack->mk_databuf(sizeof(CSP_ETHIF)))
            return false;
        netpack->head.data_size = sizeof(CSP_ETHIF);
        netpack->head.pack_size = 8 + sizeof(CSP_ETHIF);
        memcpy(netpack->databuf, &pack, sizeof(pack));
        if(!netpack->upload(sinfo))
            return false;

        if(head_pad == 0x0000)
            CPINFO[i].bno &= 0xFFFE;
        else if(head_pad == 0x0001)
            CPINFO[i].bno &= 0xFFFD;

    }
    return true;        
}
bool mkpack_usb(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    printf("[%d] ���pack_usb\n", getpid());   

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

    // if(pack.is_usb){
    //     for(int i = 0; CPINFO[i].no!=-99; ++i){
    //         if(CPINFO[i].head == 0x0c91){
    //             CPINFO[i].status = PACK_UNDO;
    //             break;
    //         }
    //     }
    // }
    return true;
}
bool mkpack_prn(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    printf("[%d] ���pack_prn\n", getpid());   

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
    
    // if(ntohs(pack.work_num) >0){
    //     for(int i = 0; CPINFO[i].no!=-99; ++i){
    //         if(CPINFO[i].head == 0x0d91){
    //             CPINFO[i].status = PACK_UNDO;
    //             break;
    //         }
    //     }
    // }
    return true;
}
bool mkpack_tty(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{

}


bool mkpack_err(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{

    return true;
}

bool chkpack_auth (SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    SCP_AUTH * pack = (SCP_AUTH*) netpack->databuf;
    pack->vno_main = ntohs(pack->vno_main);
    pack->dt_fail = ntohs(pack->dt_fail);
    pack->dt_succ = ntohs(pack->dt_succ);
    pack->random_num = ntohs(pack->random_num);
    pack->svr_time = ntohs(pack->svr_time);


    //��֤�봦��    
    //1����֤������
    if(!decrypt(pack->key, pack->random_num, pack->svr_time)){
        close(sinfo->sockfd);
        exit(8); //��֤�Ƿ�
    }
    //2������֤����
    struct tm chk_tm = {2017-1900, 0, 0, 0, 0, 0};
    u_int chk_time = mktime(&chk_tm);
    if(pack->svr_time < chk_time){
        close(sinfo->sockfd);
        exit(7);//����֤�����
    }
    //3�����汾��
    if(pack->vno_main < 2){
        for(int i = 0; CPINFO[i].no != -99; ++i)
            if(CPINFO[i].head == 0x0091){
                CPINFO[i].status = PACK_UNDO; //������Ͱ汾Ҫ����
                break;
            }
    }
    //test
    //CPINFO[0].status = PACK_UNDO; //������Ͱ汾Ҫ����

    for(int i = 0; CPINFO[i].no != -99; ++i){
        if(CPINFO[i].head == 0x0191){
            CPINFO[i].status = PACK_UNDO; //����֤��������������Ϣ
            break;
        }
    }

    return true;
}
bool chkpack_fetch (SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    u_short head = ((netpack->head).head_index << 8) | ((netpack->head).head_type);
    printf("[%d] test head in chkpack_fetch if is right : 0x%x\n", getpid(), head);
    int bno = 0;
    for(int i = 0; SPINFO[i].no != 99; ++i){
        if(SPINFO[i].head == head){
            bno = SPINFO[i].bno;
            break;
        }
    }

    if(!bno) return false; //SP���޴� head
    
    for(int i = 0; CPINFO[i].no != -99; ++i){
        if(CPINFO[i].no == bno){
            CPINFO[i].status = PACK_UNDO;
            return true;
        }
    }
    return false; //CP���޴� no
}
bool chkpack_fchn (SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    u_short head = ((netpack->head).head_index << 8) | ((netpack->head).head_type);
    u_short head_pad = (netpack->head).pad;
    int bno = 0;
    for(int i = 0; SPINFO[i].no != 99; ++i){
        if(SPINFO[i].head == head){
            bno = SPINFO[i].bno;
            break;
        }
    }
    if(!bno) return false; //SP���޴� head
    for(int i = 0; CPINFO[i].no != -99; ++i){
        if(CPINFO[i].no == bno){
            CPINFO[i].status = PACK_UNDO;
            if(head_pad == 0x0000)
                CPINFO[i].bno |= 0x0001;
            if(head_pad == 0x0001)
                CPINFO[i].bno |= 0x0002;
            
            return true;
        }
    }
    return false; //CP���޴� no
}
bool chkpack_err (SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, NETPACK* netpack)
{
    return true;
}

//���ӿ�
void pack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO)
{
    printf("[%d] ��pack���ǰ����ǰsendbuf��С: %d\n", getpid(), sinfo->sendbuf_len);
    //����Ƿ���Ҫ���
    for(int i = 0; CPINFO[i].no!=-99; ++i){
        if(CPINFO[i].status == PACK_UNDO){ //��Ҫ���
            NETPACK pack(CPINFO[i].head);
            //printf("[%d] ���pack%d\n", getpid(), CPINFO[i].no);
            //printf("[%d] test ���netpack.head�Ƿ���ȷ head_type:0x%x head_index:0x%x\n", getpid(), pack.head.head_type, pack.head.head_index);
            if(CPINFO[i].pack_fun(sinfo, SPINFO, CPINFO, &pack))
                CPINFO[i].status = PACK_EMPTY; //�ѳɹ����
        }
    }
    printf("[%d] ��pack���󣩵�ǰsendbuf��С: %d\n", getpid(), sinfo->sendbuf_len);
    return;
}
void unpack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO)
{
    while(1){
        printf("[%d] ��unpack���ǰ����ǰrecvbuf��С: %d\n", getpid(), sinfo->recvbuf_len);
        //�ж��Ƿ���н����recvbuf_len������С����С��ֻ������ͷ��
        if(sinfo->recvbuf_len < 8)
            return;

        //�жϰ����ͣ���ȷ���Ƿ���
        int p = 0;
        u_short head;
        memcpy(&head, sinfo->recvbuf, 2);
        p+=2;
        int i;
        for(i = 0; SPINFO[i].no != -99; ++i)
            if(head == SPINFO[i].head)
                break;
        if(SPINFO[i].no == -99) //�Ƿ������˳�
            return;
        
        u_short pack_size;
        u_short data_size;
        memcpy(&pack_size, sinfo->recvbuf + p, 2);
        p += 4;
        memcpy(&data_size, sinfo->recvbuf + p, 2);
        p += 2;
        pack_size = ntohs(pack_size);
        data_size = ntohs(data_size);
        if(sinfo->recvbuf_len < (int)pack_size)
            return; //�������İ�

        printf("[%d] ����������: head:0x%x, pack_size:%d, data_size:%d\n", getpid(), head, pack_size, data_size);
        
        //������������ʼ���    
        NETPACK pack;
        if(!pack.dwload(sinfo, pack_size, data_size))
            return; //���ʧ��
        printf("[%d] �������! ��ǰrecvbuf��С: %d\n", getpid(), sinfo->recvbuf_len);
        //�����ɣ����м���봦��
        if(!SPINFO[i].unpack_fun(sinfo, SPINFO, CPINFO, &pack))
            return;
        printf("[%d] ���������!\n", getpid());
    
    }
    return; //�����ܵĳɹ�
}

//��Serverͨ�ţ�devid�ӽ���
int sub(int devid)
{
    SEVPACK SERVER_PACK_INFO[] = {
    {1  ,2 , chkpack_auth  , 0x0111, "��֤����",        PACK_EMPTY},
    {2  ,3 , chkpack_fetch , 0x0211, "ȡϵͳ��Ϣ",      PACK_EMPTY},
    {3  ,4 , chkpack_fetch , 0x0311, "ȡ������Ϣ",      PACK_EMPTY},
    {4  ,5 , chkpack_fetch , 0x0411, "ȡ������Ϣ",      PACK_EMPTY},
    {5  ,6 , chkpack_fchn  , 0x0511, "ȡ��̫����Ϣ",    PACK_EMPTY},
    {6  ,7 , chkpack_fetch , 0x0711, "ȡUSB����Ϣ",     PACK_EMPTY},
    {7  ,8 , chkpack_fetch , 0x0c11, "ȡU�����ļ��б���Ϣ", PACK_EMPTY},
    {8  ,9 , chkpack_fetch , 0x0811, "ȡ��ӡ����Ϣ",    PACK_EMPTY},
    {9  ,10, chkpack_fetch , 0x0d11, "ȡ��ӡ������Ϣ",  PACK_EMPTY},
    {10 ,11, chkpack_fetch , 0x0911, "ȡ�ն˷�����Ϣ",  PACK_EMPTY},
    {11 ,12, chkpack_fetch , 0x0a11, "ȡ���ն���Ϣ",    PACK_EMPTY},
    {12 ,13, chkpack_fetch , 0x0b11, "ȡIP�ն˶���Ϣ",  PACK_EMPTY},
    {13 ,0 , chkpack_err   , 0xff11, "���а����յ�",    PACK_EMPTY},
    {-99,0 , chkpack_err   , 0x0000, "ERR",            PACK_EMPTY}
    };
    CLTPACK CLIENT_PACK_INFO[] = {
    {1  ,0 , mkpack_vno  , 0x0091, "����Ͱ汾Ҫ��", PACK_EMPTY},
    {2  ,0 , mkpack_auth , 0x0191, "����֤��������������Ϣ", PACK_EMPTY},
    {3  ,0 , mkpack_sysif, 0x0291, "��ϵͳ��Ϣ", PACK_EMPTY},
    {4  ,0 , mkpack_str  , 0x0391, "��������Ϣ", PACK_EMPTY},
    {5  ,0 , mkpack_str  , 0x0491, "��������Ϣ", PACK_EMPTY},
    {6  ,2 , mkpack_eth  , 0x0591, "����̫����Ϣ", PACK_EMPTY},
    {7  ,0 , mkpack_usb  , 0x0791, "��USB����Ϣ", PACK_EMPTY},
    {8  ,0 , mkpack_str  , 0x0c91, "��U���ļ��б���Ϣ", PACK_EMPTY},
    {9  ,0 , mkpack_prn  , 0x0891, "����ӡ����Ϣ", PACK_EMPTY},
    {10 ,0 , mkpack_str  , 0x0d91, "����ӡ������Ϣ", PACK_EMPTY},
    {11 ,0 , mkpack_err  , 0x0991, "���ն˷�����Ϣ", PACK_EMPTY},
    {12 ,0 , mkpack_err  , 0x0a91, "�����ն���Ϣ", PACK_EMPTY},
    {13 ,0 , mkpack_err  , 0x0b91, "��IP�ն���Ϣ", PACK_EMPTY},
    {14 ,0 , mkpack_err  , 0xff91, "���а����յ�", PACK_EMPTY},
    {-99,0 , mkpack_err  , 0x0000, "ERR", PACK_EMPTY}
    };
    SOCK_INFO sinfo;
    sinfo.devid = devid;

    if(!create_socket(sinfo.sockfd, 1)){//����socket����2�����������Ƿ������
        exit(1);
    }
    if(!connect_with_limit(sinfo.sockfd, 5)){//����Server
        exit(5);
    }
    printf("[%d] Connected(%s:%d) OK.\n", getpid(), _conf_ip, _conf_port);

    //ʹ�� select ģ�ͽ��ж�д����
    sinfo.recvbuf_len = 0;
    sinfo.sendbuf_len = 0;
    fd_set rfd_set, wfd_set;
    int maxfd;
    int sel;
    int len;
    while(1){
        FD_ZERO(&rfd_set);
        FD_ZERO(&wfd_set);
        if(sinfo.recvbuf_len < RECV_BUFSIZE) //���������п���ʱ���ö�fd
            FD_SET(sinfo.sockfd, &rfd_set);
        if(sinfo.sendbuf_len > 0) //д������������ʱ����дfd
            FD_SET(sinfo.sockfd, &wfd_set);
        //test
        printf("[%d] test wfd:%d\n", getpid(), FD_ISSET(sinfo.sockfd, &wfd_set));
        printf("[%d] test rfd:%d\n", getpid(), FD_ISSET(sinfo.sockfd, &rfd_set));

        maxfd = sinfo.sockfd + 1;
        sel = select(maxfd, &rfd_set, &wfd_set, NULL, NULL/*��ʱΪ���޳��ĵȴ�ʱ��*/);
        printf("[%d] --- --- --- --- test select ���� --- --- --- --- --- ---\n", getpid());
        //��
        if( sel>0 && FD_ISSET(sinfo.sockfd, &rfd_set)){
            len = recv(sinfo.sockfd, sinfo.recvbuf + sinfo.recvbuf_len, 
                                            RECV_BUFSIZE - sinfo.recvbuf_len, 0);
            if(len <= 0){
                exit(2);//����������
            }
            sinfo.recvbuf_len += len;
            //������������������
            //...
            unpack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO);
        }
        //д
        if( sel>0 && FD_ISSET(sinfo.sockfd, &wfd_set)){
            len = send(sinfo.sockfd, sinfo.sendbuf, sinfo.sendbuf_len, 0);
            if(len < 0){
                exit(3);//����д����
            }
            sinfo.sendbuf_len -= len;
            if(sinfo.sendbuf_len > 0){//��ʣ������ǰ��
                memmove(sinfo.sendbuf, sinfo.sendbuf + len, sinfo.sendbuf_len);
            }
            printf("[%d] �����ֽ���:%d��senbuf_len:%d\n", getpid(), len, sinfo.sendbuf_len);
        }
        //����
        if(sel < 0){
            exit(6);//Fail to select;
        }
        //��ʱ������
        pack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO);
    }
    exit(0);//exitcode;
}

//�����̵���
int main(int argc, char** argv)
{

    if(!chk_arg(argc, argv)){//��鲢ȡ�����в��� _devid, _devnum
        return 0;
    }
    if(!read_conf()){//��ȡ�����ļ���ȡ�����ò��� _conf_*
        return 0;
    }
    //ע���ź�
    set_signal_catch();

    //���� devnum ���ӽ���
    //log: fork�ӽ��̿�ʼ��
    printf("[%d] log:fork�ӽ��̿�ʼ[%s]\n", getpid(), get_time_full());
    for(int i = 0; i < _devnum; ++i){
        pid_t pid = fork();
        if(pid == -1){//fork err
            printf("fork error: %s(errno: %d)\n",strerror(errno),errno);
            return 0;
        }
        else if(pid == 0){//child
            exit( sub(_devid + i) ); //�ӽ��̵����м�����
        }
    }
    while(1){
        sleep(1);
    }
}
