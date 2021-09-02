/*
版本 0.0
server-base 核心流程实现 
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

#define PACK_STOP -2  //阻塞等待中
#define PACK_UNDO -1  //未处理完成
#define PACK_EMPTY 0  //空状态
#define PACK_HALF  1  //未接收/发送完成

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

//读取配置参数
bool read_conf()
{
    CONF_ITEM conf_item[] = {
        {CFITEM_NONE, "监听端口号"      , (void*)&_conf_lisenport   , 0,  65535, CFTYPE_INT32, ""},
        {CFITEM_NONE, "设备连接间隔"    , (void*)&_conf_conngap     , 5,    600, CFTYPE_INT32, ""},
        {CFITEM_NONE, "设备采样间隔"    , (void*)&_conf_trangap     ,15,   3600, CFTYPE_INT32, ""},
        {CFITEM_NONE, "服务器IP地址号"  , (void*) _conf_svrip       , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "服务器端口号"    , (void*)&_conf_svrport     , 0,  65536, CFTYPE_INT32, ""},
        {CFITEM_NONE, "数据库名"        , (void*) _conf_dbname      , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "用户名"          , (void*) _conf_dbuser      , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "用户口令"        , (void*) _conf_dbpwd       , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "未应答超时"      , (void*)&_conf_repd_outtime, 2,    120, CFTYPE_INT32, ""},
        {CFITEM_NONE, "传输超时"        , (void*)&_conf_tran_outtime, 2,    120, CFTYPE_INT32, ""},
        {CFITEM_NONE, "主日志大小"      , (void*)&_conf_mainlog_size, 0, 102400, CFTYPE_INT32, ""},
        {CFITEM_NONE, "分日志大小"      , (void*)&_conf_sublog_size , 0, 102400, CFTYPE_INT32, ""},
        {CFITEM_NONE, "屏幕显示"        , (void*)&_conf_dprint      , 0,      1, CFTYPE_INT32, ""},
        {CFITEM_NONE, "tmp_packet"      , (void*) _conf_tmp_packet  , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "tmp_socket"      , (void*) _conf_tmp_socket  , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "dev_packet"      , (void*) _conf_dev_packet  , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, "dev_socket"      , (void*) _conf_dev_socket  , 0,      0, CFTYPE_STR  , ""},
        {CFITEM_NONE, NULL              , (void*) NULL              , 0,      0, CFTYPE_STR  , ""}
    };

    CONFINFO conf_info(CONF_PATH, "=", "//", conf_item);
    printf("[%u] 取得的配置参数共%d项\n", getpid(), conf_info.getall());
    printf("[%u] 取得的有效参数共%d项\n", getpid(), conf_info.chkvar());
    
    return true;
}
//print_conf: 打印配置参数
void print_conf()
{
    printf("[%d] \n============= 打印所有配置参数 =============\n", getpid());
    printf(" [监听端口号     ] = %d\n", _conf_lisenport   );
    printf(" [设备连接间隔   ] = %d\n", _conf_conngap     );
    printf(" [设备采样间隔   ] = %d\n", _conf_trangap     );
    printf(" [服务器IP地址号 ] = %s\n", _conf_svrip       );
    printf(" [服务器端口号   ] = %d\n", _conf_svrport     );
    printf(" [数据库名       ] = %s\n", _conf_dbname      );
    printf(" [用户名         ] = %s\n", _conf_dbuser      );
    printf(" [用户口令       ] = %s\n", _conf_dbpwd       );
    printf(" [未应答超时     ] = %d\n", _conf_repd_outtime);
    printf(" [传输超时       ] = %d\n", _conf_tran_outtime);
    printf(" [主日志大小     ] = %d\n", _conf_mainlog_size);
    printf(" [分日志大小     ] = %d\n", _conf_sublog_size );
    printf(" [屏幕显示       ] = %d\n", _conf_dprint      );
    printf(" [tmp_packet     ] = %s\n", _conf_tmp_packet  );
    printf(" [tmp_socket     ] = %s\n", _conf_tmp_socket  );
    printf(" [dev_packet     ] = %s\n", _conf_dev_packet  );
    printf(" [dev_socket     ] = %s\n", _conf_dev_socket  );
    printf("=============================================\n");
    return;
}

/******** 父进程信号处理 ********/
//子进程退出原因说明
static const char *childexit_to_str(const int no)
{
    struct {
    	const int no;
    	const char *str;
    } signstr[] = {
	{0, "读取数据完成,被对端关闭"},
    {1, "创建socket失败"},
    {2, "发生读错误"},
    {3, "发生写错误"},
    {4, "读取数据完成，连接被对端关闭"},
	{5, "连接失败，达到最大重连次数"},  //按CTRL+C产生，非进程不截获
    {6, "select失败"},
    {7, "数字证书过期，连接关闭"},
    {8, "认证非法"},
    {-99, "END"}
	};

    int i=0;
    for (i=0; signstr[i].no!=-99; i++)
        if (no == signstr[i].no)
            break;

    return signstr[i].str;
}
//SIGCHLD的处理函数
void fun_waitChild(int no)
{
    int sub_status;
    pid_t pid;
    while ((pid = waitpid(-1, &sub_status, WNOHANG)) > 0) {        
        //_subproc_waitnum++;
        if (WIFEXITED(sub_status)){
            printf("[%d] child %d exit with %d （%s）\n", getpid(), pid, WEXITSTATUS(sub_status), childexit_to_str(WEXITSTATUS(sub_status)));
        }
        else if (WIFSIGNALED(sub_status)){
            printf("[%d] child %d killed by the %dth signal\n", getpid(), pid, WTERMSIG(sub_status));
        }
    
        if(false){
            childexit_to_str(1);//避免报错 呜呜呜
        }
    }
}
//信号捕捉与注册
void set_signal_catch()
{
    signal(SIGCHLD, fun_waitChild); //子进程退出信号
}

/******** TCP建立流程控制 ********/
//设置socket参数 
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
//创建socket
bool create_socket(int & sock_fd, int nonblock = 0)
{
    if( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        //log
        printf("[%d] create socket error: %s(errno: %d)\n",getpid(), strerror(errno), errno);
        return false;
    }
    if( nonblock && set_socket_flag(sock_fd, O_NONBLOCK) > 0){
        //printf("socket 设置为非阻塞状态成功!\n");
        ;
    }
    return true;
}
//将svr的ip与port绑定至socket_fd
bool bind_iftosocket(int & sock_fd)
{
    struct sockaddr_in  svraddr;

    memset(&svraddr, 0, sizeof(svraddr));
    svraddr.sin_family = AF_INET;
    svraddr.sin_addr.s_addr = inet_addr("10.80.42.242");
    svraddr.sin_port = htons(_conf_lisenport);
    
    if( bind(sock_fd, (struct sockaddr*)&svraddr, sizeof(svraddr)) == -1){
        printf("[%d] bind失败! %s(%d)\n", getpid(), strerror(errno), errno);
        return false;
    }
    return true;
}
//打印connfd的对端信息
void get_connect_info(int connfd, string& dev_ip, int & dev_port)
{
    struct sockaddr_in  peerAddr; //监听地址、连接的本地地址、连接的外地地址
    socklen_t peerAddrLen;

    //输出外地连接信息
    peerAddrLen=sizeof(peerAddr);
    getpeername(connfd, (struct sockaddr *)&peerAddr, &peerAddrLen);
    printf("[%d] 与设备连接成功（ %s:%d）\n", getpid(),\
            inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
    dev_ip = string(inet_ntoa(peerAddr.sin_addr));
    dev_port = ntohs(peerAddr.sin_port);
}


/******** 封拆包主接口 ********/
void pack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, SEV_PACK_ACTION* PA)
{
    printf("[%d] （pack检查前）当前sendbuf大小: %d\n", getpid(), sinfo->sendbuf_len);
    //检查是否需要封包
    for(int i = 0; SPINFO[i].no!=-99; ++i){
        //printf("PackName=%s, PackStatus=%d\n", CPINFO[i].str, CPINFO[i].status);
        if(SPINFO[i].status == PACK_UNDO){ //需要封包
            NETPACK pack(SPINFO[i].head);
            printf("[%d] 检查pack%d\n", getpid(), SPINFO[i].no);
            //printf("[%d] test 检查netpack.head是否正确 head_type:0x%x head_index:0x%x\n", getpid(), pack.head.head_type, pack.head.head_index);
            if((PA->*(SPINFO[i].pack_fun))(sinfo, &pack))
                SPINFO[i].status = PACK_EMPTY; //已成功封包
        }
    }
    printf("[%d] （pack检查后）当前sendbuf大小: %d\n", getpid(), sinfo->sendbuf_len);
    return;
}
void unpack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, SEV_PACK_ACTION* PA)
{
    while(1){
        //printf("[%d] （unpack检查前）当前recvbuf大小: %d\n", getpid(), sinfo->recvbuf_len);
        //判断是否进行解包：recvbuf_len大于最小包大小（只含报文头）
        if(sinfo->recvbuf_len < 8)
            return;

        //判断包类型，并确定是否解包
        int p = 0;
        u_short head;
        memcpy(&head, sinfo->recvbuf, 2);
        p+=2;
        int i;
        for(i = 0; CPINFO[i].no != -99; ++i)
            if(head == CPINFO[i].head)
                break;
        if(CPINFO[i].no == -99) //非法包，退出
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
            return; //不完整的包

        printf("[%d] 解包测试完成: head:0x%x, pack_size:%d, data_size:%d\n", \
                                            getpid(), head, pack_size, data_size);
        
        //若包完整，开始解包
        NETPACK pack;
        if(!pack.dwload(sinfo, pack_size, data_size))
            return; //解包失败
        printf("[%d] 解包结束! 当前recvbuf大小: %d\n", getpid(), sinfo->recvbuf_len);
        
        //tolog
        // ostringstream intfbuf;
        // intfbuf << SPINFO[i].str;
        // if(SPINFO[i].head == 0x0511 || SPINFO[i].head == 0x0a11 || SPINFO[i].head == 0x0b11)
        //     intfbuf << "(" << pack.head.pad <<")";
        // if(_DEBUG_ENV)
        //     _mylog.pack_tolog(DIR_RECV, intfbuf.str().c_str());
        
        //解包完成，进行检查与处理
        if(!((PA->*(CPINFO[i].unpack_fun))(sinfo, &pack))){
            printf("[%d] 解包失败（%s）\n", getpid(), CPINFO[i].str);
            return;
        }
        printf("[%d] 处理包结束（%s）!\n", getpid(), CPINFO[i].str);
    }
    return; //不可能的成功
}

/******** 主/子进程控制 ********/

//子进程：关闭建听 listenfd，完全接管 connfd 的收发
int sub(int connfd)
{
    //取得 socket 基本信息
    string dev_ip;
    int    dev_port;
    get_connect_info(connfd, dev_ip, dev_port);
    
    //报文的声明与定义
    SEVPACK SERVER_PACK_INFO[] = {
    {1  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_auth  , 0x0111, "认证请求",              PACK_UNDO},
    {2  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_sysif , 0x0211, "取系统信息",            PACK_EMPTY},
    {3  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_cfif  , 0x0311, "取配置信息",            PACK_EMPTY},
    {4  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_pcif  , 0x0411, "取进程信息",            PACK_EMPTY},
    {5  ,1 ,NULL ,&SEV_PACK_ACTION::mkpack_ethu  , 0x0511, "取以太口信息",          PACK_EMPTY},
    {6  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_usbif , 0x0711, "取USB口信息",           PACK_EMPTY},
    {7  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0c11, "取U盘上文件列表信息",   PACK_EMPTY},
    {8  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_prnif , 0x0811, "取打印口信息",          PACK_EMPTY},
    {9  ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0d11, "取打印队列信息",        PACK_EMPTY},
    {10 ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_ttyif , 0x0911, "取终端服务信息",        PACK_EMPTY},
    {11 ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0a11, "取哑终端信息",          PACK_EMPTY},
    {12 ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0b11, "取IP终端端信息",        PACK_EMPTY},
    {13 ,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0xff11, "所有包均收到",          PACK_EMPTY},
    {-99,0 ,NULL ,&SEV_PACK_ACTION::mkpack_err   , 0x0000, "ERR",                   PACK_EMPTY}
    };
    CLTPACK CLIENT_PACK_INFO[] = {
    {1  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0091, "发最低版本要求", PACK_EMPTY},
    {2  ,0 ,&SEV_PACK_ACTION::unpack_auth  , 0x0191, "发认证串及基本配置信息", PACK_EMPTY},
    {3  ,0 ,&SEV_PACK_ACTION::unpack_sysif , 0x0291, "发系统信息", PACK_EMPTY},
    {4  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0391, "发配置信息", PACK_EMPTY},
    {5  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0491, "发进程信息", PACK_EMPTY},
    {6  ,1 ,&SEV_PACK_ACTION::unpack_err   , 0x0591, "发以太口信息", PACK_EMPTY},
    {7  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0791, "发USB口信息", PACK_EMPTY},
    {8  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0c91, "发U盘文件列表信息", PACK_EMPTY},
    {9  ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0891, "发打印口信息", PACK_EMPTY},
    {10 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0d91, "发打印队列信息", PACK_EMPTY},
    {11 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0991, "发终端服务信息", PACK_EMPTY},
    {12 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0a91, "发哑终端信息", PACK_EMPTY},
    {13 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0b91, "发IP终端信息", PACK_EMPTY},
    {14 ,0 ,&SEV_PACK_ACTION::unpack_err   , 0xff91, "所有包均收到", PACK_EMPTY},
    {-99,0 ,&SEV_PACK_ACTION::unpack_err   , 0x0000, "ERR",         PACK_EMPTY}
    };   
    
    SOCK_INFO sinfo;
    sinfo.sockfd = connfd;
    sinfo.recvbuf_len = 0;
    sinfo.sendbuf_len = 0;
    
    SEV_PACK_ACTION SEVPA(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO);
    
    // select 读写控制模型
    fd_set rfd_set, wfd_set;
    int    maxfd;
    int    sel;
    int    len;
    struct timeval timeout;
    while(true){
        //开始封包
        pack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO, &SEVPA);
        //设置超时时间
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        //重置读写集
        FD_ZERO(&rfd_set);
        FD_ZERO(&wfd_set);
        if(sinfo.recvbuf_len < RECV_BUFSIZE) //读缓冲区有空余时，置读fd
            FD_SET(sinfo.sockfd, &rfd_set);
        if(sinfo.sendbuf_len > 0) //写缓冲区有内容时，置写fd
            FD_SET(sinfo.sockfd, &wfd_set);
        bool test_rfd = FD_ISSET(sinfo.sockfd, &rfd_set);
        bool test_wfd = FD_ISSET(sinfo.sockfd, &wfd_set);
        maxfd = sinfo.sockfd + 1;
        // select开始
        sel = select(maxfd, &rfd_set, &wfd_set, NULL, &timeout);
        // 读
        if(sel > 0 && FD_ISSET(sinfo.sockfd, &rfd_set)){
            len = recv(sinfo.sockfd, sinfo.recvbuf + sinfo.recvbuf_len, 
                                    RECV_BUFSIZE - sinfo.recvbuf_len, 0);
            if(len == 0){
                close(sinfo.sockfd);
                exit(4); //连接被对端关闭
            }
            if(len < 0){
                close(sinfo.sockfd);
                exit(2);
            }
            sinfo.recvbuf_len += len;
            //开始解包
            // ... ...
            unpack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO, &SEVPA);
        }
        // 写
        if(sel > 0 && FD_ISSET(sinfo.sockfd, &wfd_set)){
            len = send(sinfo.sockfd, sinfo.sendbuf, sinfo.sendbuf_len, 0);
            if(len <= 0){
                close(sinfo.sockfd);
                exit(3); //发生写错误
            }
            sinfo.sendbuf_len -= len;
            printf("[%d] 发送字节数:%d，senbuf_len:%d\n", getpid(), len, sinfo.sendbuf_len);
        }
        // select 出错
        if(sel < 0){
            close(sinfo.sockfd);
            exit(6);
        }
        // select 超时
        if(sel == 0){
            printf("[%d] select 超时返回（rfd=%d wfd=%d clt_ip=%s:%d）\n", getpid(), test_rfd, test_wfd, dev_ip.c_str(), dev_port);
        }
    
    }
    close(sinfo.sockfd);
    exit(0);
}
//父进程：一直对 listen_fd 进行连接监控，已连接的 fd 交由子进程监控
int main()
{
    //变为守护进程
    if(daemon(0,1)!=0){
        printf("[%d] %s (%d)\n",getpid(), strerror(errno), errno);
        return 0;
    }
    srand(time(NULL));
    if(!read_conf()) //读取配置文件并取得配置参数 _conf_*
        return 0;
    print_conf();
    _mymdb.set(_conf_svrip, _conf_dbuser, _conf_dbpwd, _conf_dbname);
    //注册信号
    set_signal_catch();

    int listen_fd; //监听sock
    if(!create_socket(listen_fd,1)){
        return 0;
    }
    if(!bind_iftosocket(listen_fd)){
        return 0;
    }
    if(listen(listen_fd, 1024)==-1){
        printf("[%d] listen 失败！ %s(errno:%d)\n", getpid(), strerror(errno), errno);
        return 0;
    }

    int connfd; 
    int maxfd;
    int sel;
    fd_set rfd_set;
    struct timeval timeout;
    //父进程：一直对listen_fd进行连接监控，已连接的fd交由子进程监控
    //子进程：关闭建听listenfd，完全接管connfd的收发
    while(true){
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        FD_ZERO(&rfd_set);
        FD_SET(listen_fd, &rfd_set);
        maxfd = listen_fd + 1;
        sel = select(maxfd, &rfd_set, NULL, NULL, &timeout);
        //接收连接
        if(sel > 0 && FD_ISSET(listen_fd, &rfd_set)){
            if((connfd = accept(listen_fd, (struct sockaddr*)NULL, NULL)) == -1){
                printf("[%d] accept错误 %s(errno:%d)\n", getpid(), strerror(errno), errno);
                sleep(1);
                continue;
            }
            //将 connfd 交给子进程
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
