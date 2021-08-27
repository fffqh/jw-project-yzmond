/*
版本 1.0
加入资源控制，向 80 分进发
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

//常量设置
#define MAX_RECON_TIME  5
#define MAX_CON_WATIME  25

#define PACK_STOP -2  //阻塞等待中
#define PACK_UNDO -1  //未处理完成
#define PACK_EMPTY 0  //空状态
#define PACK_HALF  1  //未接收/发送完成

#define CONF_PATH "./ts.conf"
#define LOG_PATH  "./ts.log"
#define CNT_PATH  "./ts_count.xls" 
#define _DEBUG_ENV  (_conf_debug/100000)
#define _DEBUG_ERR  (_conf_debug/10000%2)
#define _DEBUG_SPACK  (_conf_debug/1000%2)
#define _DEBUG_RPACK  (_conf_debug/100%2)
#define _DEBUG_SDATA  (_conf_debug/10%2)
#define _DEBUG_RDATA  (_conf_debug%2)


//#define LIMIT_PN_WT 100 //进程数量超过时，开始进行资源监控


//配置参数（文件取得）
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

//运行参数（命令行取得）
int  _devid;
int  _devnum;
//开关与统计量
bool  _fork_end_bool = 0; 
u_int _sum_tty = 0;
u_int _sum_scr = 0;
//记录 fork 时长
time_t fst_nSeconds;
time_t fed_nSeconds;
//记录fork情况
unsigned long _subproc_forknum;
unsigned long _subproc_waitnum;
//日志类
MYLOG _mylog(LOG_PATH, getpid());


/******** 参数与配置 ********/
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
    //所有可能存在的配置信息
    CONF_ITEM conf_item[] = {
        {CFITEM_NONE, "服务器IP地址", ""},
        {CFITEM_NONE, "端口号", ""},
        {CFITEM_NONE, "进程接收成功后退出", ""},
        {CFITEM_NONE, "最小配置终端数量", ""},
        {CFITEM_NONE, "最大配置终端数量", ""},
        {CFITEM_NONE, "每个终端最小虚屏数量", ""},
        {CFITEM_NONE, "每个终端最大虚屏数量", ""},
        {CFITEM_NONE, "删除日志文件", ""},
        {CFITEM_NONE, "DEBUG设置", ""},
        {CFITEM_NONE, "DEBUG屏幕显示", ""},
        {CFITEM_NONE, "是否为ARM", ""},
        {CFITEM_NONE, "是否打印fork情况",""},
        {CFITEM_NONE, NULL, ""}
    };
    CONFINFO conf_info(CONF_PATH, " \t", "#", conf_item);
    conf_info.getall();
    //开始格式化全局配置参数
    for(int i = 0; conf_item[i].name != NULL; ++i){
        if(conf_item[i].status == CFITEM_NONE){
            printf("[%d] test read_conf err: can't find [%s]\n", getpid(), conf_item[i].name);
            continue;
        }
        if(!strcmp(conf_item[i].name,"服务器IP地址")){
            memcpy(_conf_ip,conf_item[i].data.c_str(),sizeof(_conf_ip));
        }else if(!strcmp(conf_item[i].name,"端口号")){
            _conf_port = atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name,"进程接收成功后退出")){
            //printf("[%u] !!!test quit_name = %s\n", getpid(), conf_item[i].name);
            //printf("[%u] !!!test quit_state = %d\n", getpid(), conf_item[i].status);
            //printf("[%u] !!!test quit_str = %s\n", getpid(), conf_item[i].data.c_str());
            _conf_quit = !!atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name,"最小配置终端数量")){
            _conf_mindn = atoi(conf_item[i].data.c_str());
            if(_conf_mindn < 3 || _conf_mindn > 10)
                _conf_mindn = 6;
        }else if(!strcmp(conf_item[i].name,"最大配置终端数量")){
            _conf_maxdn = atoi(conf_item[i].data.c_str());
            if(_conf_maxdn < 10 || _conf_maxdn > 50)
                _conf_maxdn = 28;
        }else if(!strcmp(conf_item[i].name,"每个终端最小虚屏数量")){
            _conf_minsn = atoi(conf_item[i].data.c_str());
            if(_conf_minsn < 1 || _conf_minsn > 3)
                _conf_minsn = 3;
        }else if(!strcmp(conf_item[i].name,"每个终端最大虚屏数量")){
            _conf_maxsn = atoi(conf_item[i].data.c_str());
            if(_conf_maxsn < 4 || _conf_maxsn > 16)
                _conf_maxsn = 10;
        }else if(!strcmp(conf_item[i].name,"删除日志文件")){
            _conf_newlog = !!atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name,"DEBUG设置")){
            _conf_debug = atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name,"DEBUG屏幕显示")){
            _conf_dprint= atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name, "是否为ARM")){
            _conf_isarm = atoi(conf_item[i].data.c_str());
        }else if(!strcmp(conf_item[i].name, "是否打印fork情况")){
            _conf_isforkprint = atoi(conf_item[i].data.c_str());
        }
    }
    return true;
}
//print_conf: 打印配置参数
void print_conf()
{
    printf("[%d] \n=========== 打印所有配置参数 ==========\n", getpid());
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

/******** 父进程信号处理 ********/
//子进程退出原因说明
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
    while ((pid = waitpid(0, &sub_status, WNOHANG)) > 0) {        
        _subproc_waitnum++;
        if (WIFEXITED(sub_status)){
            //printf("[%d] child %d exit with %d （%s）\n", getpid(), pid, WEXITSTATUS(sub_status), childexit_to_str(WEXITSTATUS(sub_status)));
            // subprocess_clear_num += 1;
        }
        else if (WIFSIGNALED(sub_status))
            printf("[%d] child %d killed by the %dth signal\n", getpid(), pid, WTERMSIG(sub_status));
        
        //printf("[%d] test _subproc_forknum = %d, _subproc_waitnum = %d\n",getpid(), _subproc_forknum, _subproc_waitnum);
        
        if(_subproc_forknum == _subproc_waitnum){
            _fork_end_bool = true; 
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
                printf("[%d]第 %d 次连接失败...\n", getpid(), con_time);
                continue;
            }
            else if(FD_ISSET(sockfd, &writefd_set)){//到这里不一定成功（接下来如何判断？选择再次调用connect函数来判断）
                n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                n = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
                if(errno != EISCONN){//连接失败
                    printf("[%d]第 %d 次连接失败...\n", getpid(), con_time);
                    continue;
                }
                else{//连接成功
                    break;
                }
            }
            else{
                printf("[%d]第 %d 次连接失败...\n",getpid(), con_time);
            }
        }

    }//end of while
    if(con_time >= 5){
        printf("[%d]error:%s(errno:%d)\n",getpid(), strerror(errno), errno);
        printf("[%d]连接超时，客户端退出。\n",getpid());
        return false;
    }
    //connect成功了！！！
    //printf("[%d]连接主机成功，主机ip : %s，端口：%d \n", getpid(), _conf_ip, _conf_port);
    errno = 0;
    return true;
}

/******** 拆解包主接口 ********/
void pack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, CLT_PACK_ACTION* PA)
{
    //printf("[%d] （pack检查前）当前sendbuf大小: %d\n", getpid(), sinfo->sendbuf_len);
    //检查是否需要封包
    for(int i = 0; CPINFO[i].no!=-99; ++i){
        //printf("PackName=%s, PackStatus=%d\n", CPINFO[i].str, CPINFO[i].status);
        if(CPINFO[i].status == PACK_UNDO){ //需要封包
            NETPACK pack(CPINFO[i].head);
            //printf("[%d] 检查pack%d\n", getpid(), CPINFO[i].no);
            //printf("[%d] test 检查netpack.head是否正确 head_type:0x%x head_index:0x%x\n", getpid(), pack.head.head_type, pack.head.head_index);
            if((PA->*(CPINFO[i].pack_fun))(sinfo, SPINFO, CPINFO, &pack))
                CPINFO[i].status = PACK_EMPTY; //已成功封包
        }
    }
    //printf("[%d] （pack检查后）当前sendbuf大小: %d\n", getpid(), sinfo->sendbuf_len);
    return;
}
void unpack(SOCK_INFO* sinfo, SEVPACK* SPINFO, CLTPACK* CPINFO, CLT_PACK_ACTION* PA)
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
        for(i = 0; SPINFO[i].no != -99; ++i)
            if(head == SPINFO[i].head)
                break;
        if(SPINFO[i].no == -99) //非法包，退出
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

        //printf("[%d] 解包测试完成: head:0x%x, pack_size:%d, data_size:%d\n", getpid(), head, pack_size, data_size);
        
        //若包完整，开始解包
        NETPACK pack;
        if(!pack.dwload(sinfo, pack_size, data_size))
            return; //解包失败
        //printf("[%d] 解包结束! 当前recvbuf大小: %d\n", getpid(), sinfo->recvbuf_len);
        
        //tolog
        ostringstream intfbuf;
        intfbuf << SPINFO[i].str;
        if(SPINFO[i].head == 0x0511 || SPINFO[i].head == 0x0a11 || SPINFO[i].head == 0x0b11)
            intfbuf << "(" << pack.head.pad <<")";
        if(_DEBUG_ENV)
            _mylog.pack_tolog(DIR_RECV, intfbuf.str().c_str());
        
        //解包完成，进行检查与处理
        if(!((PA->*(SPINFO[i].unpack_fun))(sinfo, SPINFO, CPINFO, &pack)))
            return;
        //printf("[%d] 处理包结束!\n", getpid());
    }
    return; //不可能的成功
}


/******** 资源控制 ********/
bool watch()
{
    struct sysinfo mysysif;
    while(true){
        if(sysinfo(&mysysif)!=0){
            printf("[%d] watch 资源监控失败！\n", getpid());
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

/******** 主/子进程控制 ********/
int sub(u_int devid)
{
    srand(devid);

    SEVPACK SERVER_PACK_INFO[] = {
    {1  ,2 , &CLT_PACK_ACTION::chkpack_auth  , 0x0111, "认证请求",        PACK_EMPTY},
    {2  ,3 , &CLT_PACK_ACTION::chkpack_fetch , 0x0211, "取系统信息",      PACK_EMPTY},
    {3  ,4 , &CLT_PACK_ACTION::chkpack_fetch , 0x0311, "取配置信息",      PACK_EMPTY},
    {4  ,5 , &CLT_PACK_ACTION::chkpack_fetch , 0x0411, "取进程信息",      PACK_EMPTY},
    {5  ,6 , &CLT_PACK_ACTION::chkpack_fetch  , 0x0511, "取以太口信息",    PACK_EMPTY},
    {6  ,7 , &CLT_PACK_ACTION::chkpack_fetch , 0x0711, "取USB口信息",     PACK_EMPTY},
    {7  ,8 , &CLT_PACK_ACTION::chkpack_fetch , 0x0c11, "取U盘上文件列表信息", PACK_EMPTY},
    {8  ,9 , &CLT_PACK_ACTION::chkpack_fetch , 0x0811, "取打印口信息",    PACK_EMPTY},
    {9  ,10, &CLT_PACK_ACTION::chkpack_fetch , 0x0d11, "取打印队列信息",  PACK_EMPTY},
    {10 ,11, &CLT_PACK_ACTION::chkpack_fetch , 0x0911, "取终端服务信息",  PACK_EMPTY},
    {11 ,12, &CLT_PACK_ACTION::chkpack_fetch , 0x0a11, "取哑终端信息",    PACK_EMPTY},
    {12 ,13, &CLT_PACK_ACTION::chkpack_fetch , 0x0b11, "取IP终端端信息",  PACK_EMPTY},
    {13 ,14 ,&CLT_PACK_ACTION::chkpack_fetch , 0xff11, "所有包均收到",    PACK_EMPTY},
    {-99,0 , &CLT_PACK_ACTION::chkpack_err   , 0x0000, "ERR",            PACK_EMPTY}
    };
    CLTPACK CLIENT_PACK_INFO[] = {
    {1  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_vno  , 0x0091, "发最低版本要求", PACK_EMPTY},
    {2  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_auth , 0x0191, "发认证串及基本配置信息", PACK_EMPTY},
    {3  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_sysif, 0x0291, "发系统信息", PACK_EMPTY},
    {4  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_str  , 0x0391, "发配置信息", PACK_EMPTY},
    {5  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_str  , 0x0491, "发进程信息", PACK_EMPTY},
    {6  ,1 ,NULL , &CLT_PACK_ACTION::mkpack_eth  , 0x0591, "发以太口信息", PACK_EMPTY},
    {7  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_usb  , 0x0791, "发USB口信息", PACK_EMPTY},
    {8  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_str  , 0x0c91, "发U盘文件列表信息", PACK_EMPTY},
    {9  ,0 ,NULL , &CLT_PACK_ACTION::mkpack_prn  , 0x0891, "发打印口信息", PACK_EMPTY},
    {10 ,0 ,NULL , &CLT_PACK_ACTION::mkpack_str  , 0x0d91, "发打印队列信息", PACK_EMPTY},
    {11 ,0 ,NULL , &CLT_PACK_ACTION::mkpack_tty  , 0x0991, "发终端服务信息", PACK_EMPTY},
    {12 ,1 ,NULL , &CLT_PACK_ACTION::mkpack_mtsn , 0x0a91, "发哑终端信息", PACK_EMPTY},
    {13 ,1 ,NULL , &CLT_PACK_ACTION::mkpack_itsn , 0x0b91, "发IP终端信息", PACK_EMPTY},
    {14 ,0 ,NULL , &CLT_PACK_ACTION::mkpack_done , 0xff91, "所有包均收到", PACK_EMPTY},
    {-99,0 ,NULL , &CLT_PACK_ACTION::mkpack_err  , 0x0000, "ERR",         PACK_EMPTY}
    };
    
    SOCK_INFO sinfo;
    sinfo.devid = devid;
    _mylog.set_devid(devid);

    CLT_PACK_ACTION CLTPA(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO);
    
    if(!create_socket(sinfo.sockfd, 1)){//创建socket，第2个参数代表是否非阻塞
        exit(1);
    }
    if(!connect_with_limit(sinfo.sockfd, 5)){//连接Server
        close(sinfo.sockfd);
        exit(5);
    }
    char buf[64] = {0};
    sprintf(buf,"Connected(%s:%d) OK.", _conf_ip, _conf_port);
    if(_DEBUG_ENV)
        _mylog.str_tolog(buf);

    //使用 select 模型进行读写控制
    sinfo.recvbuf_len = 0;
    sinfo.sendbuf_len = 0;
    fd_set rfd_set, wfd_set;
    int maxfd;
    int sel;
    int len;
    struct timeval timeout;
    while(1){
        //设置超时时间
        timeout.tv_sec  = 10;
        timeout.tv_usec = 0;
        //清空FDSET
        FD_ZERO(&rfd_set);
        FD_ZERO(&wfd_set);
        if(sinfo.recvbuf_len < RECV_BUFSIZE) //读缓冲区有空余时，置读fd
            FD_SET(sinfo.sockfd, &rfd_set);
        if(sinfo.sendbuf_len > 0) //写缓冲区有内容时，置写fd
            FD_SET(sinfo.sockfd, &wfd_set);
        bool test_rfd = FD_ISSET(sinfo.sockfd, &rfd_set);
        bool test_wfd = FD_ISSET(sinfo.sockfd, &wfd_set);
        maxfd = sinfo.sockfd + 1;
        sel = select(maxfd, &rfd_set, &wfd_set, NULL, &timeout/*暂时为无限长的等待时间*/);
        //printf("[%d] --- --- --- --- test select 返回 --- --- --- --- --- ---\n", getpid());
        //读
        if( sel>0 && FD_ISSET(sinfo.sockfd, &rfd_set)){
            len = recv(sinfo.sockfd, sinfo.recvbuf + sinfo.recvbuf_len, 
                                            RECV_BUFSIZE - sinfo.recvbuf_len, 0);
            if(len == 0){
                //写入统计信息
                MYLOG cntlog(CNT_PATH, devid);
                cntlog.set_std(0);
                cntlog.set_fle(1);
                cntlog.cnt_tolog(_sum_tty, _sum_scr);
                close(sinfo.sockfd);
                exit(4);//连接被对端关闭
            }
            if(len < 0){
                close(sinfo.sockfd);
                exit(2);//发生读错误
            }
            sinfo.recvbuf_len += len;
            if(_DEBUG_RPACK)
                _mylog.rw_tolog(DIR_RECV,len);
            if(_DEBUG_RDATA)
                _mylog.buf_tolog(DIR_RECV, sinfo.recvbuf + sinfo.recvbuf_len-len, len);
            unpack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO, &CLTPA);
        }
        //写
        if( sel>0 && FD_ISSET(sinfo.sockfd, &wfd_set)){
            len = send(sinfo.sockfd, sinfo.sendbuf, sinfo.sendbuf_len, 0);
            if(len <= 0){
                close(sinfo.sockfd);
                exit(3);//发生写错误
            }
            sinfo.sendbuf_len -= len;
            if(_DEBUG_SPACK)
                _mylog.rw_tolog(DIR_SEND, len);
            if(_DEBUG_SDATA)
                _mylog.buf_tolog(DIR_SEND, sinfo.sendbuf, len);
            if(sinfo.sendbuf_len > 0){//将剩余数据前移
                memmove(sinfo.sendbuf, sinfo.sendbuf + len, sinfo.sendbuf_len);
            }
            //printf("[%d] 发送字节数:%d，senbuf_len:%d\n", getpid(), len, sinfo.sendbuf_len);
        }
        //错误
        if(sel < 0){
            close(sinfo.sockfd);
            exit(6);//Fail to select;
        }
        if(sel == 0){
            printf("[%u] select 超时返回(rfd=%d wfd=%d)(devid=%u)\n", getpid(), test_rfd, test_wfd, devid);
        }
        //超时，继续
        pack(&sinfo, SERVER_PACK_INFO, CLIENT_PACK_INFO,&CLTPA);
    }
    close(sinfo.sockfd);
    exit(0);//exitcode;
}

int main(int argc, char** argv)
{
    srand(time(NULL));
    if(!chk_arg(argc, argv)){//检查并取得运行参数 _devid, _devnum
        return 0;
    }
    if(!read_conf()){//读取配置文件并取得配置参数 _conf_*
        return 0;
    }
    //test
    print_conf();
    //注册信号
    set_signal_catch();
    
    //是否删除原日志
    if(_conf_newlog){
        remove(LOG_PATH);
        remove(CNT_PATH);
    }
    //初始化log
    (_conf_dprint == 1)? _mylog.set_std(1) : _mylog.set_std(0);
    
    while(1){    
        //全局变量初始化
        _subproc_waitnum = 0;
        _subproc_forknum = 0;
        //log: fork子进程开始！
        
        if(!_DEBUG_ENV)
            _mylog.set_fle(0);
        if( _conf_isforkprint )
            _mylog.set_std(1);        
        fst_nSeconds = _mylog.fst_tolog();
        if(!_DEBUG_ENV)
            _mylog.set_fle(1);
        if( _conf_isforkprint )
            _mylog.set_std(_conf_dprint);
        
        //分裂 devnum 个子进程
        for(int i = 0; i < _devnum; ++i){
            pid_t pid = fork();
            if(pid == -1){//fork err
                printf("fork error: %s(errno: %d)\n",strerror(errno),errno);
                return 0;
            }
            else if(pid == 0){//child
                exit( sub(_devid + i) ); //子进程的运行及返回
            }

            if( _conf_isforkprint && !(i % _conf_isforkprint))
                printf("[%u] 当前已分裂/已回收进程数 = %lu/%lu\n", getpid(), _subproc_forknum, _subproc_waitnum);
            watch();
            
            _subproc_forknum++;
        }
        //主进程睡眠
        while(1){
            if(_fork_end_bool){
                //printf("[%d] test fork_end !!!\n", getpid());
                //log: fork&wait 情况反馈
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
            _mylog.str_tolog("一次数据发送结束，5秒后继续重复进行.....");        
        sleep(5);
    }
    return 0;
}


/************
struct sysinfo {
      long uptime;          // 启动到现在经过的时间 
      unsigned long loads[3];  
      // 1, 5, and 15 minute load averages 
      unsigned long totalram;  // 总的可用的内存大小 
      unsigned long freeram;   // 还未被使用的内存大小
      unsigned long sharedram; // 共享的存储器的大小
      unsigned long bufferram; // 共享的存储器的大小 
      unsigned long totalswap; // 交换区大小 
      unsigned long freeswap;  // 还可用的交换区大小 
      unsigned short procs;    // 当前进程数目 
      unsigned long totalhigh; // 总的高内存大小 
      unsigned long freehigh;  // 可用的高内存大小 
      unsigned int mem_unit;   // 以字节为单位的内存大小 
      char _f[20-2*sizeof(long)-sizeof(int)]; 
      // libc5的补丁
}
*/


