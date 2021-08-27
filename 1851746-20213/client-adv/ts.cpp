/*
�汾 1.0
������Դ���ƣ��� 80 �ֽ���
*/

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <stack>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h> //waitpid
#include <sys/fcntl.h>
#include <sys/sysinfo.h>

#include "../include/my_encrpty.h"
#include "../include/my_socket.h"
#include "../include/my_pack.h"
#include "../include/my_getproc.h"
#include "../include/my_getconf.h"
#include "../include/my_log.h"
#include "../include/my_cltpack.h"

using namespace std;

//��������
#define MAX_RECON_TIME  5
#define MAX_CON_WATIME  25

#define PACK_STOP -2  //�����ȴ���
#define PACK_UNDO -1  //δ�������
#define PACK_EMPTY 0  //��״̬
#define PACK_HALF  1  //δ����/�������

#define CONF_PATH "./ts.conf"
#define LOG_PATH  "./ts.log"
#define CNT_PATH  "./ts_count.xls" 
#define _DEBUG_ENV  (_conf_debug/100000)
#define _DEBUG_ERR  (_conf_debug/10000%2)
#define _DEBUG_SPACK  (_conf_debug/1000%2)
#define _DEBUG_RPACK  (_conf_debug/100%2)
#define _DEBUG_SDATA  (_conf_debug/10%2)
#define _DEBUG_RDATA  (_conf_debug%2)


//#define LIMIT_PN_WT 100 //������������ʱ����ʼ������Դ���


//���ò������ļ�ȡ�ã�
char _conf_ip[16] = "192.168.1.242";//
int  _conf_port   = 41746;//
bool _conf_quit   = 1;
int  _conf_mindn  = 5;//
int  _conf_maxdn  = 28;//
int  _conf_minsn  = 3;//
int  _conf_maxsn  = 10;//
bool _conf_newlog = 1;//
int  _conf_debug  = 111111;//..
int  _conf_dprint = 1;//
int  _conf_isarm  = 0;//
int  _conf_isforkprint = 0;//

//���в�����������ȡ�ã�
int  _devid;
int  _devnum;
//������ͳ����
bool  _fork_end_bool = 0; 
u_int _sum_tty = 0;
u_int _sum_scr = 0;
//��¼ fork ʱ��
time_t fst_nSeconds;
time_t fed_nSeconds;
//��¼fork���
unsigned long _subproc_forknum;
unsigned long _subproc_waitnum;
//��־��
MYLOG _mylog(LOG_PATH, getpid());


/******** ���������� ********/
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
    //���п��ܴ��ڵ�������Ϣ
    CONF_ITEM conf_item[] = {
        {CFITEM_NONE, "������IP��ַ", ""},
        {CFITEM_NONE, "�˿ں�", ""},
        {CFITEM_NONE, "���̽��ճɹ����˳�", ""},
        {CFITEM_NONE, "��С�����ն�����", ""},
        {CFITEM_NONE, "��������ն�����", ""},
        {CFITEM_NONE, "ÿ���ն���С��������", ""},
        {CFITEM_NONE, "ÿ���ն������������", ""},
        {CFITEM_NONE, "ɾ����־�ļ�", ""},
        {CFITEM_NONE, "DEBUG����", ""},
        {CFITEM_NONE, "DEBUG��Ļ��ʾ", ""},
        {CFITEM_NONE, "�Ƿ�ΪARM", ""},
        {CFITEM_NONE, "�Ƿ��ӡfork���",""},
        {CFITEM_NONE, NULL, ""}
    };
    CONFINFO conf_info(CONF_PATH, " \t", "#", conf_item);
    conf_info.getall();
    //��ʼ��ʽ��ȫ�����ò���
    for(int i = 0; conf_item[i].name != NULL; ++i){
        if(conf_item[i].status == CFITEM_NONE){
            printf("[%d] test read_conf err: can't find [%s]\n", getpid(), conf_item[i].name);
            continue;
        }
        if(!strcmp(conf_item[i].name,"������IP��ַ")){
            memcpy(_conf_ip,conf_item[i].data.c_str(),sizeof(_conf_ip));
        }else if(!strcmp(conf_item[i].name,"�˿ں�")){
            _conf_port = atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name,"���̽��ճɹ����˳�")){
            //printf("[%u] !!!test quit_name = %s\n", getpid(), conf_item[i].name);
            //printf("[%u] !!!test quit_state = %d\n", getpid(), conf_item[i].status);
            //printf("[%u] !!!test quit_str = %s\n", getpid(), conf_item[i].data.c_str());
            _conf_quit = !!atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name,"��С�����ն�����")){
            _conf_mindn = atoi(conf_item[i].data.c_str());
            if(_conf_mindn < 3 || _conf_mindn > 10)
                _conf_mindn = 6;
        }else if(!strcmp(conf_item[i].name,"��������ն�����")){
            _conf_maxdn = atoi(conf_item[i].data.c_str());
            if(_conf_maxdn < 10 || _conf_maxdn > 50)
                _conf_maxdn = 28;
        }else if(!strcmp(conf_item[i].name,"ÿ���ն���С��������")){
            _conf_minsn = atoi(conf_item[i].data.c_str());
            if(_conf_minsn < 1 || _conf_minsn > 3)
                _conf_minsn = 3;
        }else if(!strcmp(conf_item[i].name,"ÿ���ն������������")){
            _conf_maxsn = atoi(conf_item[i].data.c_str());
            if(_conf_maxsn < 4 || _conf_maxsn > 16)
                _conf_maxsn = 10;
        }else if(!strcmp(conf_item[i].name,"ɾ����־�ļ�")){
            _conf_newlog = !!atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name,"DEBUG����")){
            _conf_debug = atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name,"DEBUG��Ļ��ʾ")){
            _conf_dprint= atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name, "�Ƿ�ΪARM")){
            _conf_isarm = atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name, "�Ƿ��ӡfork���")){
            _conf_isforkprint = atoi(conf_item[i].data.c_str());
        }
    }
    return true;
}
//print_conf: ��ӡ���ò���
void print_conf()
{
    printf("[%d] \n=========== ��ӡ�������ò��� ==========\n", getpid());
    printf("_conf_ip = %s\n", _conf_ip);
    printf("_conf_port   = %d\n",  _conf_port  );
    printf("_conf_quit   = %d\n",  _conf_quit  );
    printf("_conf_mindn  = %d\n",  _conf_mindn );
    printf("_conf_maxdn  = %d\n",  _conf_maxdn );
    printf("_conf_minsn  = %d\n",  _conf_minsn );
    printf("_conf_maxsn  = %d\n",  _conf_maxsn );
    printf("_conf_newlog = %d\n",  _conf_newlog);
    printf("_conf_debug  = %d\n",  _conf_debug );
    printf("_conf_dprint = %d\n",  _conf_dprint);
    printf("_conf_isarm  = %d\n",  _conf_isarm);
    printf("_conf_isforkprint = %d\n",  _conf_isforkprint);
    printf("=========================================\n");
    return;
}

/******** �������źŴ��� ********/
//�ӽ����˳�ԭ��˵��
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
    {4, "��ȡ������ɣ����ӱ��Զ˹ر�"},
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
//SIGCHLD�Ĵ�����
void fun_waitChild(int no)
{
    int sub_status;
    pid_t pid;
    while ((pid = waitpid(0, &sub_status, WNOHANG)) > 0) {        
        _subproc_waitnum++;
        if (WIFEXITED(sub_status)){
            //printf("[%d] child %d exit with %d ��%s��\n", getpid(), pid, WEXITSTATUS(sub_status), childexit_to_str(WEXITSTATUS(sub_status)));
            // subprocess_clear_num += 1;
        }
        else if (WIFSIGNALED(sub_status))
            printf("[%d] child %d killed by the %dth signal\n", getpid(), pid, WTERMSIG(sub_status));
        
        //printf("[%d] test _subproc_forknum = %d, _subproc_waitnum = %d\n",getpid(), _subproc_forknum, _subproc_waitnum);
        
        if(_subproc_forknum == _subproc_waitnum){
            _fork_end_bool = true; 
        }

        if(false){
            childexit_to_str(1);//���ⱨ�� ������
        }
    }
}
//�źŲ�׽��ע��
void set_signal_catch()
{
    signal(SIGCHLD, fun_waitChild); //�ӽ����˳��ź�
}

/******** TCP�������̿��� ********/
//����socket���� 
int set_socket_flag(int sockfd, int FLAG)
{
    int flags;
    if( (flags = fcntl(sockfd, F_GETFL, NULL)) <0){
        printf("[%d] fcntl F_GETFL error:%s(errno: %d) \n",getpid(), strerror(errno), errno);
        return -1;
    }
    if( fcntl(sockfd, F_SETFL, flags|FLAG) == -1){
        printf("[%d] fcntl F_SETFL error:%s(errno: %d) \n",getpid(), strerror(errno), errno);
        return -1;
    }
    return 1;
}
//����socket
bool create_socket(int & sock_fd, int nonblock = 0)
{
    if( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        //log
        printf("[%d] create socket error: %s(errno: %d)\n",getpid(), strerror(errno), errno);
        return false;
    }
    if( nonblock && set_socket_flag(sock_fd, O_NONBLOCK) > 0){
        //printf("socket ����Ϊ������״̬�ɹ�!\n");
        ;
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
                printf("[%d]�� %d ������ʧ��...\n", getpid(), con_time);
                continue;
            }
            else if(FD_ISSET(sockfd, &writefd_set)){//�����ﲻһ���ɹ�������������жϣ�ѡ���ٴε���connect�������жϣ�
                n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                if(errno != EISCONN){//����ʧ��
                    printf("[%d]�� %d ������ʧ��...\n", getpid(), con_time);
                    continue;
                }
                else{//���ӳɹ�
                    break;
                }
            }
            else{
                printf("[%d]�� %d ������ʧ��...\n",getpid(), con_time);
            }
        }

    }//end of while
    if(con_time >= 5){
        printf("[%d]error:%s(errno:%d)\n",getpid(), strerror(errno), errno);
        printf("[%d]���ӳ�ʱ���ͻ����˳���\n",getpid());
        return false;
    }
    //connect�ɹ��ˣ�����
    //printf("[%d]���������ɹ�������ip : %s���˿ڣ�%d \n", getpid(), _conf_ip, _conf_port);
    errno = 0;
    return true;
}

/******** �������ӿ� ********/
void pack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, CLT_PACK_ACTION* PA)
{
    //printf("[%d] ��pack���ǰ����ǰsendbuf��С: %d\n", getpid(), sinfo->sendbuf_len);
    //����Ƿ���Ҫ���
    for(int i = 0; CPINFO[i].no!=-99; ++i){
        //printf("PackName=%s, PackStatus=%d\n", CPINFO[i].str, CPINFO[i].status);
        if(CPINFO[i].status == PACK_UNDO){ //��Ҫ���
            NETPACK pack(CPINFO[i].head);
            //printf("[%d] ���pack%d\n", getpid(), CPINFO[i].no);
            //printf("[%d] test ���netpack.head�Ƿ���ȷ head_type:0x%x head_index:0x%x\n", getpid(), pack.head.head_type, pack.head.head_index);
            if((PA->*(CPINFO[i].pack_fun))(sinfo, SPINFO, CPINFO, &pack))
                CPINFO[i].status = PACK_EMPTY; //�ѳɹ����
        }
    }
    //printf("[%d] ��pack���󣩵�ǰsendbuf��С: %d\n", getpid(), sinfo->sendbuf_len);
    return;
}
void unpack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, CLT_PACK_ACTION* PA)
{
    while(1){
        //printf("[%d] ��unpack���ǰ����ǰrecvbuf��С: %d\n", getpid(), sinfo->recvbuf_len);
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

        //printf("[%d] ����������: head:0x%x, pack_size:%d, data_size:%d\n", getpid(), head, pack_size, data_size);
        
        //������������ʼ���
        NETPACK pack;
        if(!pack.dwload(sinfo, pack_size, data_size))
            return; //���ʧ��
        //printf("[%d] �������! ��ǰrecvbuf��С: %d\n", getpid(), sinfo->recvbuf_len);
        
        //tolog
        ostringstream intfbuf;
        intfbuf << SPINFO[i].str;
        if(SPINFO[i].head == 0x0511 || SPINFO[i].head == 0x0a11 || SPINFO[i].head == 0x0b11)
            intfbuf << "(" << pack.head.pad <<")";
        if(_DEBUG_ENV)
            _mylog.pack_tolog(DIR_RECV, intfbuf.str().c_str());
        
        //�����ɣ����м���봦��
        if(!((PA->*(SPINFO[i].unpack_fun))(sinfo, SPINFO, CPINFO, &pack)))
            return;
        //printf("[%d] ���������!\n", getpid());
    }
    return; //�����ܵĳɹ�
}


/******** ��Դ���� ********/
bool watch()
{
    struct sysinfo mysysif;
    while(true){
        if(sysinfo(&mysysif)!=0){
            printf("[%d] watch ��Դ���ʧ�ܣ�\n", getpid());
            return false;
        }
        // printf("[%u] watch: totalram  = %lu\n", getpid(), mysysif.totalram );
        // printf("[%u] watch: freeram   = %lu\n", getpid(), mysysif.freeram  );
        // printf("[%u] watch: sharedram = %lu\n", getpid(), mysysif.sharedram);
        // printf("[%u] watch: bufferram = %lu\n", getpid(), mysysif.bufferram);
        // printf("[%u] watch: freeswap  = %lu\n", getpid(), mysysif.freeswap );
        // printf("[%u] watch: procs     = %d\n", getpid(), mysysif.procs    );
        if( _subproc_forknum - _subproc_waitnum < 200)
            break;
        
    }
    return true;
}

/******** ��/�ӽ��̿��� ********/
int sub(u_int devid)
{
    srand(devid);

    SEVPACK SERVER_PACK_INFO[] = {
    {1  ,2 , &CLT_PACK_ACTION::chkpack_auth  , 0x0111, "��֤����",        PACK_EMPTY},
    {2  ,3 , &CLT_PACK_ACTION::chkpack_fetch , 0x0211, "ȡϵͳ��Ϣ",      PACK_EMPTY},
    {3  ,4 , &CLT_PACK_ACTION::chkpack_fetch , 0x0311, "ȡ������Ϣ",      PACK_EMPTY},
    {4  ,5 , &CLT_PACK_ACTION::chkpack_fetch , 0x0411, "ȡ������Ϣ",      PACK_EMPTY},
    {5  ,6 , &CLT_PACK_ACTION::chkpack_fetch  , 0x0511, "ȡ��̫����Ϣ",    PACK_EMPTY},
    {6  ,7 , &CLT_PACK_ACTION::chkpack_fetch , 0x0711, "ȡUSB����Ϣ",     PACK_EMPTY},
    {7  ,8 , &CLT_PACK_ACTION::chkpack_fetch , 0x0c11, "ȡU�����ļ��б���Ϣ", PACK_EMPTY},
    {8  ,9 , &CLT_PACK_ACTION::chkpack_fetch , 0x0811, "ȡ��ӡ����Ϣ",    PACK_EMPTY},
    {9  ,10, &CLT_PACK_ACTION::chkpack_fetch , 0x0d11, "ȡ��ӡ������Ϣ",  PACK_EMPTY},
    {10 ,11, &CLT_PACK_ACTION::chkpack_fetch , 0x0911, "ȡ�ն˷�����Ϣ",  PACK_EMPTY},
    {11 ,12, &CLT_PACK_ACTION::chkpack_fetch , 0x0a11, "ȡ���ն���Ϣ",    PACK_EMPTY},
    {12 ,13, &CLT_PACK_ACTION::chkpack_fetch , 0x0b11, "ȡIP�ն˶���Ϣ",  PACK_EMPTY},
    {13 ,14 ,&CLT_PACK_ACTION::chkpack_fetch , 0xff11, "���а����յ�",    PACK_EMPTY},
    {-99,0 , &CLT_PACK_ACTION::chkpack_err   , 0x0000, "ERR",            PACK_EMPTY}
    };
    CLTPACK CLIENT_PACK_INFO[] = {
    {1  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_vno  , 0x0091, "����Ͱ汾Ҫ��", PACK_EMPTY},
    {2  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_auth , 0x0191, "����֤��������������Ϣ", PACK_EMPTY},
    {3  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_sysif, 0x0291, "��ϵͳ��Ϣ", PACK_EMPTY},
    {4  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_str  , 0x0391, "��������Ϣ", PACK_EMPTY},
    {5  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_str  , 0x0491, "��������Ϣ", PACK_EMPTY},
    {6  ,1 ,NULL , &CLT_PACK_ACTION::mkpack_eth  , 0x0591, "����̫����Ϣ", PACK_EMPTY},
    {7  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_usb  , 0x0791, "��USB����Ϣ", PACK_EMPTY},
    {8  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_str  , 0x0c91, "��U���ļ��б���Ϣ", PACK_EMPTY},
    {9  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_prn  , 0x0891, "����ӡ����Ϣ", PACK_EMPTY},
    {10 ,0 ,NULL , &CLT_PACK_ACTION::mkpack_str  , 0x0d91, "����ӡ������Ϣ", PACK_EMPTY},
    {11 ,0 ,NULL , &CLT_PACK_ACTION::mkpack_tty  , 0x0991, "���ն˷�����Ϣ", PACK_EMPTY},
    {12 ,1 ,NULL , &CLT_PACK_ACTION::mkpack_mtsn , 0x0a91, "�����ն���Ϣ", PACK_EMPTY},
    {13 ,1 ,NULL , &CLT_PACK_ACTION::mkpack_itsn , 0x0b91, "��IP�ն���Ϣ", PACK_EMPTY},
    {14 ,0 ,NULL , &CLT_PACK_ACTION::mkpack_done , 0xff91, "���а����յ�", PACK_EMPTY},
    {-99,0 ,NULL , &CLT_PACK_ACTION::mkpack_err  , 0x0000, "ERR",         PACK_EMPTY}
    };
    
    SOCK_INFO sinfo;
    sinfo.devid = devid;
    _mylog.set_devid(devid);

    CLT_PACK_ACTION CLTPA(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO);
    
    if(!create_socket(sinfo.sockfd, 1)){//����socket����2�����������Ƿ������
        exit(1);
    }
    if(!connect_with_limit(sinfo.sockfd, 5)){//����Server
        close(sinfo.sockfd);
        exit(5);
    }
    char buf[64] = {0};
    sprintf(buf,"Connected(%s:%d) OK.", _conf_ip, _conf_port);
    if(_DEBUG_ENV)
        _mylog.str_tolog(buf);

    //ʹ�� select ģ�ͽ��ж�д����
    sinfo.recvbuf_len = 0;
    sinfo.sendbuf_len = 0;
    fd_set rfd_set, wfd_set;
    int maxfd;
    int sel;
    int len;
    struct timeval timeout;
    while(1){
        //���ó�ʱʱ��
        timeout.tv_sec  = 10;
        timeout.tv_usec = 0;
        //���FDSET
        FD_ZERO(&rfd_set);
        FD_ZERO(&wfd_set);
        if(sinfo.recvbuf_len < RECV_BUFSIZE) //���������п���ʱ���ö�fd
            FD_SET(sinfo.sockfd, &rfd_set);
        if(sinfo.sendbuf_len > 0) //д������������ʱ����дfd
            FD_SET(sinfo.sockfd, &wfd_set);
        bool test_rfd = FD_ISSET(sinfo.sockfd, &rfd_set);
        bool test_wfd = FD_ISSET(sinfo.sockfd, &wfd_set);
        maxfd = sinfo.sockfd + 1;
        sel = select(maxfd, &rfd_set, &wfd_set, NULL, &timeout/*��ʱΪ���޳��ĵȴ�ʱ��*/);
        //printf("[%d] --- --- --- --- test select ���� --- --- --- --- --- ---\n", getpid());
        //��
        if( sel>0 && FD_ISSET(sinfo.sockfd, &rfd_set)){
            len = recv(sinfo.sockfd, sinfo.recvbuf + sinfo.recvbuf_len, 
                                            RECV_BUFSIZE - sinfo.recvbuf_len, 0);
            if(len == 0){
                //д��ͳ����Ϣ
                MYLOG cntlog(CNT_PATH, devid);
                cntlog.set_std(0);
                cntlog.set_fle(1);
                cntlog.cnt_tolog(_sum_tty, _sum_scr);
                close(sinfo.sockfd);
                exit(4);//���ӱ��Զ˹ر�
            }
            if(len < 0){
                close(sinfo.sockfd);
                exit(2);//����������
            }
            sinfo.recvbuf_len += len;
            if(_DEBUG_RPACK)
                _mylog.rw_tolog(DIR_RECV,len);
            if(_DEBUG_RDATA)
                _mylog.buf_tolog(DIR_RECV, sinfo.recvbuf + sinfo.recvbuf_len-len, len);
            unpack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO, &CLTPA);
        }
        //д
        if( sel>0 && FD_ISSET(sinfo.sockfd, &wfd_set)){
            len = send(sinfo.sockfd, sinfo.sendbuf, sinfo.sendbuf_len, 0);
            if(len <= 0){
                close(sinfo.sockfd);
                exit(3);//����д����
            }
            sinfo.sendbuf_len -= len;
            if(_DEBUG_SPACK)
                _mylog.rw_tolog(DIR_SEND, len);
            if(_DEBUG_SDATA)
                _mylog.buf_tolog(DIR_SEND, sinfo.sendbuf, len);
            if(sinfo.sendbuf_len > 0){//��ʣ������ǰ��
                memmove(sinfo.sendbuf, sinfo.sendbuf + len, sinfo.sendbuf_len);
            }
            //printf("[%d] �����ֽ���:%d��senbuf_len:%d\n", getpid(), len, sinfo.sendbuf_len);
        }
        //����
        if(sel < 0){
            close(sinfo.sockfd);
            exit(6);//Fail to select;
        }
        if(sel == 0){
            printf("[%u] select ��ʱ����(rfd=%d wfd=%d)(devid=%u)\n", getpid(), test_rfd, test_wfd, devid);
        }
        //��ʱ������
        pack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO,&CLTPA);
    }
    close(sinfo.sockfd);
    exit(0);//exitcode;
}

int main(int argc, char** argv)
{
    srand(time(NULL));
    if(!chk_arg(argc, argv)){//��鲢ȡ�����в��� _devid, _devnum
        return 0;
    }
    if(!read_conf()){//��ȡ�����ļ���ȡ�����ò��� _conf_*
        return 0;
    }
    //test
    print_conf();
    //ע���ź�
    set_signal_catch();
    
    //�Ƿ�ɾ��ԭ��־
    if(_conf_newlog){
        remove(LOG_PATH);
        remove(CNT_PATH);
    }
    //��ʼ��log
    (_conf_dprint == 1)? _mylog.set_std(1) : _mylog.set_std(0);
    
    while(1){    
        //ȫ�ֱ�����ʼ��
        _subproc_waitnum = 0;
        _subproc_forknum = 0;
        //log: fork�ӽ��̿�ʼ��
        
        if(!_DEBUG_ENV)
            _mylog.set_fle(0);
        if( _conf_isforkprint )
            _mylog.set_std(1);        
        fst_nSeconds = _mylog.fst_tolog();
        if(!_DEBUG_ENV)
            _mylog.set_fle(1);
        if( _conf_isforkprint )
            _mylog.set_std(_conf_dprint);
        
        //���� devnum ���ӽ���
        for(int i = 0; i < _devnum; ++i){
            pid_t pid = fork();
            if(pid == -1){//fork err
                printf("fork error: %s(errno: %d)\n",strerror(errno),errno);
                return 0;
            }
            else if(pid == 0){//child
                exit( sub(_devid + i) ); //�ӽ��̵����м�����
            }

            if( _conf_isforkprint && !(i % _conf_isforkprint))
                printf("[%u] ��ǰ�ѷ���/�ѻ��ս����� = %lu/%lu\n", getpid(), _subproc_forknum, _subproc_waitnum);
            watch();
            
            _subproc_forknum++;
        }
        //������˯��
        while(1){
            if(_fork_end_bool){
                //printf("[%d] test fork_end !!!\n", getpid());
                //log: fork&wait �������
                if(!_DEBUG_ENV)
                    _mylog.set_fle(0);
                if(_conf_isforkprint)
                    _mylog.set_std(1);
                
                _mylog.fork_tolog(_subproc_forknum, _subproc_waitnum);
                fed_nSeconds = _mylog.fed_tolog(fst_nSeconds, _subproc_forknum);
                if(!_DEBUG_ENV)
                    _mylog.set_fle(1);
                if(_conf_isforkprint)
                    _mylog.set_std(_conf_dprint);

                _fork_end_bool = false;
                break;
            }
            sleep(1);
        }
        if(_conf_quit)
            break;
        if(_DEBUG_ENV)
            _mylog.str_tolog("һ�����ݷ��ͽ�����5�������ظ�����.....");        
        sleep(5);
    }
    return 0;
}


/************
struct sysinfo {
      long uptime;          // ���������ھ�����ʱ�� 
      unsigned long loads[3];  
      // 1, 5, and 15 minute load averages 
      unsigned long totalram;  // �ܵĿ��õ��ڴ��С 
      unsigned long freeram;   // ��δ��ʹ�õ��ڴ��С
      unsigned long sharedram; // ����Ĵ洢���Ĵ�С
      unsigned long bufferram; // ����Ĵ洢���Ĵ�С 
      unsigned long totalswap; // ��������С 
      unsigned long freeswap;  // �����õĽ�������С 
      unsigned short procs;    // ��ǰ������Ŀ 
      unsigned long totalhigh; // �ܵĸ��ڴ��С 
      unsigned long freehigh;  // ���õĸ��ڴ��С 
      unsigned int mem_unit;   // ���ֽ�Ϊ��λ���ڴ��С 
      char _f[20-2*sizeof(long)-sizeof(int)]; 
      // libc5�Ĳ���
}
*/


