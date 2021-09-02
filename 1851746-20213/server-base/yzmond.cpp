/*
�汾 0.0
server-base ��������ʵ�� 
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
#include <sys/wait.h>   //waitpid
#include <sys/fcntl.h>
#include <sys/sysinfo.h>

#include "../include/my_socket.h"
#include "../include/my_getconf.h"
#include "../include/my_pack.h"
#include "../include/my_svrpack.h"
#include "../include/my_log.h"

#define CONF_PATH "./yzmond.conf"
#define LOG_PATH "./yzmond.log"

#define PACK_STOP -2  //�����ȴ���
#define PACK_UNDO -1  //δ�������
#define PACK_EMPTY 0  //��״̬
#define PACK_HALF  1  //δ����/�������

int32_t _conf_lisenport     = 41746;
int32_t _conf_conngap       = 15;
int32_t _conf_trangap       = 900;
char    _conf_svrip[16]     = "localhost";
int32_t _conf_svrport       = 3306;
char    _conf_dbname[16]    = "yzmon_1851746";
char    _conf_dbuser[16]    = "dbuser_1851746";
char    _conf_dbpwd [16]    = "yzmond.1851746";
int32_t _conf_repd_outtime  = 30;
int32_t _conf_tran_outtime  = 60;
int32_t _conf_mainlog_size  = 10240;
int32_t _conf_sublog_size   = 1024;
int32_t _conf_dprint        = 0;
char    _conf_tmp_packet[5]    = "0000";
char    _conf_tmp_socket[5]    = "0000";
char    _conf_dev_packet[5]    = "0000";
char    _conf_dev_socket[5]    = "0000";

MYLOG       _mylog(LOG_PATH, getpid());
MYMDB       _mymdb();

int32_t     _conf_debug = 111111;

//��ȡ���ò���
bool read_conf()
{
    CONF_ITEM conf_item[] = {
        {CFITEM_NONE, "�����˿ں�"      , (void*)&_conf_lisenport   , 0,  65535, CFTYPE_INT32, ""},
        {CFITEM_NONE, "�豸���Ӽ��"    , (void*)&_conf_conngap     , 5,    600, CFTYPE_INT32, ""},
        {CFITEM_NONE, "�豸�������"    , (void*)&_conf_trangap     ,15,   3600, CFTYPE_INT32, ""},
        {CFITEM_NONE, "������IP��ַ��"  , (void*) _conf_svrip       , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "�������˿ں�"    , (void*)&_conf_svrport     , 0,  65536, CFTYPE_INT32, ""},
        {CFITEM_NONE, "���ݿ���"        , (void*) _conf_dbname      , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "�û���"          , (void*) _conf_dbuser      , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "�û�����"        , (void*) _conf_dbpwd       , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "δӦ��ʱ"      , (void*)&_conf_repd_outtime, 2,    120, CFTYPE_INT32, ""},
        {CFITEM_NONE, "���䳬ʱ"        , (void*)&_conf_tran_outtime, 2,    120, CFTYPE_INT32, ""},
        {CFITEM_NONE, "����־��С"      , (void*)&_conf_mainlog_size, 0, 102400, CFTYPE_INT32, ""},
        {CFITEM_NONE, "����־��С"      , (void*)&_conf_sublog_size , 0, 102400, CFTYPE_INT32, ""},
        {CFITEM_NONE, "��Ļ��ʾ"        , (void*)&_conf_dprint      , 0,      1, CFTYPE_INT32, ""},
        {CFITEM_NONE, "tmp_packet"      , (void*) _conf_tmp_packet  , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "tmp_socket"      , (void*) _conf_tmp_socket  , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "dev_packet"      , (void*) _conf_dev_packet  , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "dev_socket"      , (void*) _conf_dev_socket  , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, NULL              , (void*) NULL              , 0,      0, CFTYPE_STR  , ""}
    };

    CONFINFO conf_info(CONF_PATH, "=", "//", conf_item);
    printf("[%u] ȡ�õ����ò�����%d��\n", getpid(), conf_info.getall());
    printf("[%u] ȡ�õ���Ч������%d��\n", getpid(), conf_info.chkvar());
    
    return true;
}
//print_conf: ��ӡ���ò���
void print_conf()
{
    printf("[%d] \n============= ��ӡ�������ò��� =============\n", getpid());
    printf(" [�����˿ں�     ] = %d\n", _conf_lisenport   );
    printf(" [�豸���Ӽ��   ] = %d\n", _conf_conngap     );
    printf(" [�豸�������   ] = %d\n", _conf_trangap     );
    printf(" [������IP��ַ�� ] = %s\n", _conf_svrip       );
    printf(" [�������˿ں�   ] = %d\n", _conf_svrport     );
    printf(" [���ݿ���       ] = %s\n", _conf_dbname      );
    printf(" [�û���         ] = %s\n", _conf_dbuser      );
    printf(" [�û�����       ] = %s\n", _conf_dbpwd       );
    printf(" [δӦ��ʱ     ] = %d\n", _conf_repd_outtime);
    printf(" [���䳬ʱ       ] = %d\n", _conf_tran_outtime);
    printf(" [����־��С     ] = %d\n", _conf_mainlog_size);
    printf(" [����־��С     ] = %d\n", _conf_sublog_size );
    printf(" [��Ļ��ʾ       ] = %d\n", _conf_dprint      );
    printf(" [tmp_packet     ] = %s\n", _conf_tmp_packet  );
    printf(" [tmp_socket     ] = %s\n", _conf_tmp_socket  );
    printf(" [dev_packet     ] = %s\n", _conf_dev_packet  );
    printf(" [dev_socket     ] = %s\n", _conf_dev_socket  );
    printf("=============================================\n");
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
	{0, "��ȡ�������,���Զ˹ر�"},
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
    while ((pid = waitpid(-1, &sub_status, WNOHANG)) > 0) {        
        //_subproc_waitnum++;
        if (WIFEXITED(sub_status)){
            printf("[%d] child %d exit with %d ��%s��\n", getpid(), pid, WEXITSTATUS(sub_status), childexit_to_str(WEXITSTATUS(sub_status)));
        }
        else if (WIFSIGNALED(sub_status)){
            printf("[%d] child %d killed by the %dth signal\n", getpid(), pid, WTERMSIG(sub_status));
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
//��svr��ip��port����socket_fd
bool bind_iftosocket(int & sock_fd)
{
    struct sockaddr_in  svraddr;

    memset(&svraddr, 0, sizeof(svraddr));
    svraddr.sin_family = AF_INET;
    svraddr.sin_addr.s_addr = inet_addr("10.80.42.242");
    svraddr.sin_port = htons(_conf_lisenport);
    
    if( bind(sock_fd, (struct sockaddr*)&svraddr, sizeof(svraddr)) == -1){
        printf("[%d] bindʧ��! %s(%d)\n", getpid(), strerror(errno), errno);
        return false;
    }
    return true;
}
//��ӡconnfd�ĶԶ���Ϣ
void get_connect_info(int connfd, string& dev_ip, int & dev_port)
{
    struct sockaddr_in  peerAddr; //������ַ�����ӵı��ص�ַ�����ӵ���ص�ַ
    socklen_t peerAddrLen;

    //������������Ϣ
    peerAddrLen=sizeof(peerAddr);
    getpeername(connfd, (struct sockaddr *)&peerAddr, &peerAddrLen);
    printf("[%d] ���豸���ӳɹ��� %s:%d��\n", getpid(),\
            inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
    dev_ip = string(inet_ntoa(peerAddr.sin_addr));
    dev_port = ntohs(peerAddr.sin_port);
}


/******** �������ӿ� ********/
void pack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, SEV_PACK_ACTION* PA)
{
    printf("[%d] ��pack���ǰ����ǰsendbuf��С: %d\n", getpid(), sinfo->sendbuf_len);
    //����Ƿ���Ҫ���
    for(int i = 0; SPINFO[i].no!=-99; ++i){
        //printf("PackName=%s, PackStatus=%d\n", CPINFO[i].str, CPINFO[i].status);
        if(SPINFO[i].status == PACK_UNDO){ //��Ҫ���
            NETPACK pack(SPINFO[i].head);
            printf("[%d] ���pack%d\n", getpid(), SPINFO[i].no);
            //printf("[%d] test ���netpack.head�Ƿ���ȷ head_type:0x%x head_index:0x%x\n", getpid(), pack.head.head_type, pack.head.head_index);
            if((PA->*(SPINFO[i].pack_fun))(sinfo, &pack))
                SPINFO[i].status = PACK_EMPTY; //�ѳɹ����
        }
    }
    printf("[%d] ��pack���󣩵�ǰsendbuf��С: %d\n", getpid(), sinfo->sendbuf_len);
    return;
}
void unpack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, SEV_PACK_ACTION* PA)
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
        for(i = 0; CPINFO[i].no != -99; ++i)
            if(head == CPINFO[i].head)
                break;
        if(CPINFO[i].no == -99) //�Ƿ������˳�
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

        printf("[%d] ����������: head:0x%x, pack_size:%d, data_size:%d\n", \
                                            getpid(), head, pack_size, data_size);
        
        //������������ʼ���
        NETPACK pack;
        if(!pack.dwload(sinfo, pack_size, data_size))
            return; //���ʧ��
        printf("[%d] �������! ��ǰrecvbuf��С: %d\n", getpid(), sinfo->recvbuf_len);
        
        //tolog
        // ostringstream intfbuf;
        // intfbuf << SPINFO[i].str;
        // if(SPINFO[i].head == 0x0511 || SPINFO[i].head == 0x0a11 || SPINFO[i].head == 0x0b11)
        //     intfbuf << "(" << pack.head.pad <<")";
        // if(_DEBUG_ENV)
        //     _mylog.pack_tolog(DIR_RECV, intfbuf.str().c_str());
        
        //�����ɣ����м���봦��
        if(!((PA->*(CPINFO[i].unpack_fun))(sinfo, &pack))){
            printf("[%d] ���ʧ�ܣ�%s��\n", getpid(), CPINFO[i].str);
            return;
        }
        printf("[%d] �����������%s��!\n", getpid(), CPINFO[i].str);
    }
    return; //�����ܵĳɹ�
}

/******** ��/�ӽ��̿��� ********/

//�ӽ��̣��رս��� listenfd����ȫ�ӹ� connfd ���շ�
int sub(int connfd)
{
    //ȡ�� socket ������Ϣ
    string dev_ip;
    int    dev_port;
    get_connect_info(connfd, dev_ip, dev_port);
    
    //���ĵ������붨��
    SEVPACK SERVER_PACK_INFO[] = {
    {1  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_auth  , 0x0111, "��֤����",              PACK_UNDO},
    {2  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_sysif , 0x0211, "ȡϵͳ��Ϣ",            PACK_EMPTY},
    {3  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_cfif  , 0x0311, "ȡ������Ϣ",            PACK_EMPTY},
    {4  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_pcif  , 0x0411, "ȡ������Ϣ",            PACK_EMPTY},
    {5  ,1 ,NULL ,&SEV_PACK_ACTION::mkpack_ethu  , 0x0511, "ȡ��̫����Ϣ",          PACK_EMPTY},
    {6  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_usbif , 0x0711, "ȡUSB����Ϣ",           PACK_EMPTY},
    {7  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0c11, "ȡU�����ļ��б���Ϣ",   PACK_EMPTY},
    {8  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_prnif , 0x0811, "ȡ��ӡ����Ϣ",          PACK_EMPTY},
    {9  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0d11, "ȡ��ӡ������Ϣ",        PACK_EMPTY},
    {10 ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_ttyif , 0x0911, "ȡ�ն˷�����Ϣ",        PACK_EMPTY},
    {11 ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0a11, "ȡ���ն���Ϣ",          PACK_EMPTY},
    {12 ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0b11, "ȡIP�ն˶���Ϣ",        PACK_EMPTY},
    {13 ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0xff11, "���а����յ�",          PACK_EMPTY},
    {-99,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0000, "ERR",                   PACK_EMPTY}
    };
    CLTPACK CLIENT_PACK_INFO[] = {
    {1  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0091, "����Ͱ汾Ҫ��", PACK_EMPTY},
    {2  ,0 ,&SEV_PACK_ACTION::unpack_auth  , 0x0191, "����֤��������������Ϣ", PACK_EMPTY},
    {3  ,0 ,&SEV_PACK_ACTION::unpack_sysif , 0x0291, "��ϵͳ��Ϣ", PACK_EMPTY},
    {4  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0391, "��������Ϣ", PACK_EMPTY},
    {5  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0491, "��������Ϣ", PACK_EMPTY},
    {6  ,1 ,&SEV_PACK_ACTION::unpack_err   , 0x0591, "����̫����Ϣ", PACK_EMPTY},
    {7  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0791, "��USB����Ϣ", PACK_EMPTY},
    {8  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0c91, "��U���ļ��б���Ϣ", PACK_EMPTY},
    {9  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0891, "����ӡ����Ϣ", PACK_EMPTY},
    {10 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0d91, "����ӡ������Ϣ", PACK_EMPTY},
    {11 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0991, "���ն˷�����Ϣ", PACK_EMPTY},
    {12 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0a91, "�����ն���Ϣ", PACK_EMPTY},
    {13 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0b91, "��IP�ն���Ϣ", PACK_EMPTY},
    {14 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0xff91, "���а����յ�", PACK_EMPTY},
    {-99,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0000, "ERR",         PACK_EMPTY}
    };   
    
    SOCK_INFO sinfo;
    sinfo.sockfd = connfd;
    sinfo.recvbuf_len = 0;
    sinfo.sendbuf_len = 0;
    
    SEV_PACK_ACTION SEVPA(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO);
    
    // select ��д����ģ��
    fd_set rfd_set, wfd_set;
    int    maxfd;
    int    sel;
    int    len;
    struct timeval timeout;
    while(true){
        //��ʼ���
        pack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO, &SEVPA);
        //���ó�ʱʱ��
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        //���ö�д��
        FD_ZERO(&rfd_set);
        FD_ZERO(&wfd_set);
        if(sinfo.recvbuf_len < RECV_BUFSIZE) //���������п���ʱ���ö�fd
            FD_SET(sinfo.sockfd, &rfd_set);
        if(sinfo.sendbuf_len > 0) //д������������ʱ����дfd
            FD_SET(sinfo.sockfd, &wfd_set);
        bool test_rfd = FD_ISSET(sinfo.sockfd, &rfd_set);
        bool test_wfd = FD_ISSET(sinfo.sockfd, &wfd_set);
        maxfd = sinfo.sockfd + 1;
        // select��ʼ
        sel = select(maxfd, &rfd_set, &wfd_set, NULL, &timeout);
        // ��
        if(sel > 0 && FD_ISSET(sinfo.sockfd, &rfd_set)){
            len = recv(sinfo.sockfd, sinfo.recvbuf + sinfo.recvbuf_len, 
                                    RECV_BUFSIZE - sinfo.recvbuf_len, 0);
            if(len == 0){
                close(sinfo.sockfd);
                exit(4); //���ӱ��Զ˹ر�
            }
            if(len < 0){
                close(sinfo.sockfd);
                exit(2);
            }
            sinfo.recvbuf_len += len;
            //��ʼ���
            // ... ...
            unpack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO, &SEVPA);
        }
        // д
        if(sel > 0 && FD_ISSET(sinfo.sockfd, &wfd_set)){
            len = send(sinfo.sockfd, sinfo.sendbuf, sinfo.sendbuf_len, 0);
            if(len <= 0){
                close(sinfo.sockfd);
                exit(3); //����д����
            }
            sinfo.sendbuf_len -= len;
            printf("[%d] �����ֽ���:%d��senbuf_len:%d\n", getpid(), len, sinfo.sendbuf_len);
        }
        // select ����
        if(sel < 0){
            close(sinfo.sockfd);
            exit(6);
        }
        // select ��ʱ
        if(sel == 0){
            printf("[%d] select ��ʱ���أ�rfd=%d wfd=%d clt_ip=%s:%d��\n", getpid(), test_rfd, test_wfd, dev_ip.c_str(), dev_port);
        }
    
    }
    close(sinfo.sockfd);
    exit(0);
}
//�����̣�һֱ�� listen_fd �������Ӽ�أ������ӵ� fd �����ӽ��̼��
int main()
{
    //��Ϊ�ػ�����
    if(daemon(0,1)!=0){
        printf("[%d] %s (%d)\n",getpid(), strerror(errno), errno);
        return 0;
    }
    srand(time(NULL));
    if(!read_conf()) //��ȡ�����ļ���ȡ�����ò��� _conf_*
        return 0;
    print_conf();
    _mymdb.set(_conf_svrip, _conf_dbuser, _conf_dbpwd, _conf_dbname);
    //ע���ź�
    set_signal_catch();

    int listen_fd; //����sock
    if(!create_socket(listen_fd,1)){
        return 0;
    }
    if(!bind_iftosocket(listen_fd)){
        return 0;
    }
    if(listen(listen_fd, 1024)==-1){
        printf("[%d] listen ʧ�ܣ� %s(errno:%d)\n", getpid(), strerror(errno), errno);
        return 0;
    }

    int connfd; 
    int maxfd;
    int sel;
    fd_set rfd_set;
    struct timeval timeout;
    //�����̣�һֱ��listen_fd�������Ӽ�أ������ӵ�fd�����ӽ��̼��
    //�ӽ��̣��رս���listenfd����ȫ�ӹ�connfd���շ�
    while(true){
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        FD_ZERO(&rfd_set);
        FD_SET(listen_fd, &rfd_set);
        maxfd = listen_fd + 1;
        sel = select(maxfd, &rfd_set, NULL, NULL, &timeout);
        //��������
        if(sel > 0 && FD_ISSET(listen_fd, &rfd_set)){
            if((connfd = accept(listen_fd, (struct sockaddr*)NULL, NULL)) == -1){
                printf("[%d] accept���� %s(errno:%d)\n", getpid(), strerror(errno), errno);
                sleep(1);
                continue;
            }
            //�� connfd �����ӽ���
            pid_t pid = fork();
            if(pid == -1){ //fork err
                printf("[%d] fork error: %s(errno:%d)\n", getpid(), strerror(errno), errno);
                break;
            }
            else if(pid == 0){ //child
                exit( sub(connfd) );
            }
            close(connfd);
        }
    }

    return 0;
}
