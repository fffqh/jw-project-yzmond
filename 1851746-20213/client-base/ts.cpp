/*
版本 0.1 
配置信息写死的测试版
先不管读配置文件的苦力活 w
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

//常量设置
#define MAX_RECON_TIME  5
#define MAX_CON_WATIME  25
#define RECV_BUFSIZE    4096
#define SEND_BUFSIZE    4096

#define PACK_STOP -2  //阻塞等待中
#define PACK_UNDO -1  //未处理完成
#define PACK_EMPTY 0  //空状态
#define PACK_HALF  1  //未接收/发送完成

//自定义结构体
typedef struct{

    int sockfd;
    int devid;

    int recvbuf_len;
    int sendbuf_len;
    u_char recvbuf[RECV_BUFSIZE];
    u_char sendbuf[SEND_BUFSIZE];

}SOCK_INFO;

typedef struct{
    const int         no;
    const int        bno; //0表示无需回复
    const u_short   head;
    const char      *str;
    u_short         size;
    u_short         len;
    int             status;
} SEVPACK;

typedef struct{
    const int       no;
    const int       bno;
    const u_short   head;
    const char      *str;
    int             status;
} CLTPACK;

//配置参数
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
//运行参数
int  _devid;
int  _devnum;

/* 参数与配置 */
//chk_arg: 检查调用参数
bool chk_arg(int argc, char** argv)
{
    if(argc!=3){
        //log
        printf("传入参数有误，client退出!");
        return false;
    }
    _devid = atoi(argv[1]);
    _devnum = atoi(argv[2]);
    return true;
}
//read_conf: 读取配置文件
bool read_conf()
{
    ;
    return true;
}

/* 父进程信号处理 */
// 子进程退出原因说明
static const char *childexit_to_str(const int no)
{
    struct {
    	const int no;
    	const char *str;
    } signstr[] = {
	{0,  "读取数据完成,被对端关闭"},
    {1, "创建socket失败"},
    {2, "发生读错误"},
    {3, "发生写错误"},
	{5, "连接失败，达到最大重连次数"},  //按CTRL+C产生，非进程不截获
    {6, "select失败"},
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
//信号捕捉与注册
void set_signal_catch()
{
    signal(SIGCHLD, fun_waitChild); //子进程退出信号
}

/* TCP建立流程控制 */
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
//创建socket
bool create_socket(int & sock_fd, int nonblock = 0)
{
    if( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        //log
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return false;
    }
    if( nonblock && set_socket_flag(sock_fd, O_NONBLOCK) > 0){
        printf("socket 设置为非阻塞状态成功!\n");
    }
    return true;
}
//与Server连接
bool connect_with_limit(int sockfd, int maxc)
{
    struct  sockaddr_in  servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(_conf_port);
    servaddr.sin_addr.s_addr=inet_addr(_conf_ip);

/*connect策略说明：
   1、连接错误则重连，最大重连次数为 5 次
   2、select 超时停止方式等待连接（设置超时机制，最大等待时长为 MAX_CON_WATIME）
***/
    int con_time = 0;
    int n;

    while(con_time++ < MAX_RECON_TIME){//连接失败，重连多次
        //printf("第%d次尝试连接\n", con_time);
        n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
        //printf("error:%s(errno:%d)\n", strerror(errno), errno);
        if(n == 0){//立即成功了
            //print_func_ret_err("connect", n);
            break;
        }
        else if((n < 0) && (errno == EINPROGRESS)){//connect没有立即成功，但还有成功的希望
            //使用select函数，延时设为MAX_CON_WATIME
            fd_set writefd_set;
            struct timeval time_out;
            time_out.tv_sec = MAX_CON_WATIME;
            time_out.tv_usec = 0;
            FD_ZERO(&writefd_set);
            FD_SET(sockfd, &writefd_set);

            int select_ret = select(sockfd+1, NULL, &writefd_set, NULL, &time_out);
            if(select_ret <= 0){
                //print_func_ret_err("select_forever", select_ret);
                printf("第 %d 次连接失败...\n", con_time);
                continue;
            }
            else if(FD_ISSET(sockfd, &writefd_set)){//到这里不一定成功（接下来如何判断？选择再次调用connect函数来判断）
                n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                if(errno != EISCONN){//连接失败
                    printf("第 %d 次连接失败...\n", con_time);
                    continue;
                }
                else{//连接成功
                    break;
                }
            }
            else{
                printf("第 %d 次连接失败...\n", con_time);
            }
        }

    }//end of while
    if(con_time >= 5){
        printf("error:%s(errno:%d)\n", strerror(errno), errno);
        printf("连接超时，客户端退出。\n");
        return false;
    }
    //connect成功了！！！
    printf("连接主机成功，主机ip : %s，端口：%d \n", _conf_ip, _conf_port);
    errno = 0;
    return true;
}

/* 记录日志：写一个专门用于输出日志的函数 */
//得到当前系统时间
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

/* 缓冲区处理：封包与拆包 */
void pack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO)
{
    printf("[%d] 当前sendbuf大小: %d\n", getpid(), sinfo->sendbuf_len);
    return;
}
void unpack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO)
{
    printf("[%d] 当前recvbuf大小: %d\n", getpid(), sinfo->recvbuf_len);
    return;
}

//与Server通信：devid子进程
int sub(int devid)
{
    SEVPACK SERVER_PACK_INFO[] = {
    {1  ,2 , 0x1101, "认证请求", 0,0, PACK_EMPTY},
    {2  ,3 , 0x1102, "取系统信息", 0,0, PACK_EMPTY},
    {3  ,4 , 0x1103, "取配置信息", 0,0, PACK_EMPTY},
    {4  ,5 , 0x1104, "取进程信息", 0,0, PACK_EMPTY},
    {5  ,6 , 0x1105, "取以太口信息", 0,0, PACK_EMPTY},
    {6  ,7 , 0x1107, "取USB口信息", 0,0, PACK_EMPTY},
    {7  ,8 , 0x110c, "取U盘上文件列表信息", 0,0, PACK_EMPTY},
    {8  ,9 , 0x1108, "取打印口信息", 0,0, PACK_EMPTY},
    {9  ,10, 0x110d, "取打印队列信息", 0,0, PACK_EMPTY},
    {10 ,11, 0x1109, "取终端服务信息", 0,0, PACK_EMPTY},
    {11 ,12, 0x110a, "取哑终端信息", 0,0, PACK_EMPTY},
    {12 ,13, 0x110b, "取IP终端端信息", 0,0, PACK_EMPTY},
    {13 ,0 , 0x11ff, "所有包均收到", 0,0, PACK_EMPTY},
    {-99,0 , 0x0000, "ERR", 0,0, PACK_EMPTY}
    };
    CLTPACK CLIENT_PACK_INFO[] = {
    {1  ,0 , 0x9100, "发最低版本要求", PACK_EMPTY},
    {2  ,0 , 0x9101, "发认证串及基本配置信息", PACK_EMPTY},
    {3  ,0 , 0x9102, "发系统信息", PACK_EMPTY},
    {4  ,0 , 0x9103, "发配置信息", PACK_EMPTY},
    {5  ,0 , 0x9104, "发进程信息", PACK_EMPTY},
    {6  ,0 , 0x9105, "发以太口信息", PACK_EMPTY},
    {7  ,0 , 0x9107, "发USB口信息", PACK_EMPTY},
    {8  ,0 , 0x910c, "发U盘文件列表信息", PACK_EMPTY},
    {9  ,0 , 0x9108, "发打印口信息", PACK_EMPTY},
    {10 ,0 , 0x910d, "发打印队列信息", PACK_EMPTY},
    {11 ,0 , 0x9109, "发终端服务信息", PACK_EMPTY},
    {12 ,0 , 0x910a, "发哑终端信息", PACK_EMPTY},
    {13 ,0 , 0x910b, "发IP终端信息", PACK_EMPTY},
    {14 ,0 , 0x91ff, "所有包均收到", PACK_EMPTY},
    {-99,0 , 0x0000, "ERR", PACK_EMPTY}
    };
    SOCK_INFO sinfo;
    sinfo.devid = devid;

    if(!create_socket(sinfo.sockfd, 1)){//创建socket，第2个参数代表是否非阻塞
        exit(1);
    }
    if(!connect_with_limit(sinfo.sockfd, 5)){//连接Server
        exit(5);
    }
    printf("[%d] Connected(%s:%d) OK.\n", getpid(), _conf_ip, _conf_port);

    //使用 select 模型进行读写控制
    sinfo.recvbuf_len = 0;
    sinfo.sendbuf_len = 0;
    fd_set rfd_set, wfd_set;
    int maxfd;
    int sel;
    int len;
    FD_ZERO(&rfd_set);
    FD_ZERO(&wfd_set);
    while(1){
        if(sinfo.recvbuf_len < RECV_BUFSIZE) //读缓冲区有空余时，置读fd
            FD_SET(sinfo.sockfd, &rfd_set);
        if(sinfo.sendbuf_len > 0) //写缓冲区有内容时，置写fd
            FD_SET(sinfo.sockfd, &wfd_set);

        maxfd = sinfo.sockfd + 1;
        sel = select(maxfd, &rfd_set, &wfd_set, NULL, NULL/*暂时为无限长的等待时间*/);
        //读
        if( sel>0 && FD_ISSET(sinfo.sockfd, &rfd_set)){
            len = recv(sinfo.sockfd, sinfo.recvbuf + sinfo.recvbuf_len, 
                                            RECV_BUFSIZE - sinfo.recvbuf_len, 0);
            if(len <= 0){
                exit(2);//发生读错误
            }
            sinfo.recvbuf_len += len;
            //处理读缓冲区（拆包）
            //...
            unpack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO);
        }
        //写
        if( sel>0 && FD_ISSET(sinfo.sockfd, &wfd_set)){
            len = send(sinfo.sockfd, sinfo.sendbuf, sinfo.sendbuf_len, 0);
            if(len <= 0){
                exit(3);//发生写错误
            }
            sinfo.sendbuf_len -= len;
            if(sinfo.sendbuf_len > 0){//将剩余数据前移
                memmove(sinfo.sendbuf, sinfo.sendbuf + len, sinfo.sendbuf_len);
            }
        }
        //错误
        if(sel < 0){
            exit(6);//Fail to select;
        }
        //超时，继续
        pack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO);
    }
    exit(0);//exitcode;
}

//主进程调度
int main(int argc, char** argv)
{

    if(!chk_arg(argc, argv)){//检查并取得运行参数 _devid, _devnum
        return 0;
    }
    if(!read_conf()){//读取配置文件并取得配置参数 _conf_*
        return 0;
    }
    //注册信号
    set_signal_catch();

    //分裂 devnum 个子进程
    //log: fork子进程开始！
    printf("[%d] log:fork子进程开始[%s]\n", getpid(), get_time_full());
    for(int i = 0; i < _devnum; ++i){
        pid_t pid = fork();
        if(pid == -1){//fork err
            printf("fork error: %s(errno: %d)\n",strerror(errno),errno);
            return 0;
        }
        else if(pid == 0){//child
            exit( sub(_devid + i) ); //子进程的运行及返回
        }
    }
    while(1){
        sleep(1);
    }
}

