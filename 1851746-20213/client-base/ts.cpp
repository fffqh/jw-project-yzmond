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

//��������
#define MAX_RECON_TIME  5
#define MAX_CON_WATIME  25
#define RECV_BUFSIZE    4096
#define SEND_BUFSIZE    4096

//socket��Ϣ�ṹ��
typedef struct{

    int sockfd;
    int devid;

    int recvbuf_len;
    int sendbuf_len;
    u_char recvbuf[RECV_BUFSIZE];
    u_char sendbuf[SEND_BUFSIZE];

}SOCK_INFO;
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

/* ���������� */
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

/* �������źŴ��� */
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

/* TCP�������̿��� */
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

/* ��¼��־��дһ��ר�����������־�ĺ��� */
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

/* ������� */




//��Serverͨ�ţ�devid�ӽ���
int sub(int devid)
{
    SOCK_INFO sinfo;
    sinfo.devid = devid;

    if(!create_socket(sinfo.sockfd, 1)){//����socket����2�����������Ƿ������
        return 1;
    }
    if(!connect_with_limit(sinfo.sockfd, 5)){//����Server
        return 5;
    }
    printf("[%d] Connected(%s:%d) OK.\n", getpid(), _conf_ip, _conf_port);

    //ʹ�� select ģ�ͽ��ж�д����
    sinfo.recvbuf_len = 0;
    sinfo.sendbuf_len = 0;
    fd_set rfd_set, wfd_set;
    int maxfd, maxi;
    int sel;
    int len;
    FD_ZERO(&rfd_set);
    FD_ZERO(&wfd_set);
    while(1){
        if(sinfo.recvbuf_len < RECV_BUFSIZE) //���������п���ʱ���ö�fd
            FD_SET(sinfo.sockfd, &rfd_set);
        if(sinfo.sendbuf_len > 0) //д������������ʱ����дfd
            FD_SET(sinfo.sockfd, &wfd_set);

        maxfd = sinfo.sockfd + 1;
        sel = select(maxfd, &rfd_set, &wfd_set, NULL, NULL/*��ʱΪ���޳��ĵȴ�ʱ��*/);
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
        }
        //д
        if( sel>0 && FD_ISSET(sinfo.sockfd, &wfd_set)){
            len = send(sinfo.sockfd, sinfo.sendbuf, sinfo.sendbuf_len, 0);
            if(len <= 0){
                exit(3);//����д����
            }
            sinfo.sendbuf_len -= len;
            if(sinfo.sendbuf_len > 0){//��ʣ������ǰ��
                memmove(sinfo.sendbuf, sinfo.sendbuf + len, sinfo.sendbuf_len);
            }
        }
        //����
        if(sel < 0){
            return 6;//Fail to select;
        }
        //��ʱ������
    }
    return 0;//exitcode;
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

