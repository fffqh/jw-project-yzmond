#include<iostream>
#include<stdio.h>
#include<mysql.h>
#include<string.h>
#include<string>
#include<iomanip>


using namespace std;

//c++ mysql 的操作流程
    // 1、初始化 mysql 变量
    // 2、连接数据库
    // 3、设置字符集
    // 4、进行查询与插入
    // 5、释放 result
    // 6、关闭 mysql 连接

enum base {
    ENUM_base_devid         ,
    ENUM_base_devno         ,
    ENUM_base_time          ,
    ENUM_base_ipaddr        ,
    ENUM_base_sid           ,
    ENUM_base_type          ,
    ENUM_base_version       ,
    ENUM_base_cpu           ,
    ENUM_base_sdram         ,
    ENUM_base_flash         ,
    ENUM_base_ethnum        ,
    ENUM_base_syncnum       ,
    ENUM_base_asyncnum      ,
    ENUM_base_switchnum     ,
    ENUM_base_usbnum        ,
    ENUM_base_prnnum        ,
    ENUM_base_cpu_used      ,
    ENUM_base_sdram_used    ,
    ENUM_base_tty_configed  ,
    ENUM_base_tty_connected ,
    ENUM_base_eth0_ip       ,
    ENUM_base_eth0_mask     ,
    ENUM_base_eth0_mac      ,
    ENUM_base_eth0_state    ,
    ENUM_base_eth0_speed    ,
    ENUM_base_eth0_duplex   ,
    ENUM_base_eth0_autonego ,
    ENUM_base_eth0_txbytes  ,
    ENUM_base_eth0_txpackets,
    ENUM_base_eth0_rxbytes  ,
    ENUM_base_eth0_rxpackets,
    ENUM_base_eth1_ip       ,
    ENUM_base_eth1_mask     ,
    ENUM_base_eth1_mac      ,
    ENUM_base_eth1_state    ,
    ENUM_base_eth1_speed    ,
    ENUM_base_eth1_duplex   ,
    ENUM_base_eth1_autonego ,
    ENUM_base_eth1_txbytes  ,
    ENUM_base_eth1_txpackets,
    ENUM_base_eth1_rxbytes  ,
    ENUM_base_eth1_rxpackets,
    ENUM_base_usbstate      ,
    ENUM_base_usbfiles      ,
    ENUM_base_prnname       ,
    ENUM_base_prnstate      ,
    ENUM_base_prnfiles      ,
    ENUM_base_switchinfo    ,
    ENUM_base_config        ,
    ENUM_base_process       ,
    ENUM_base_sendreq       ,
    ENUM_base_END
};
const char * sql_base_colname[] = {
    "devstate_base_devid         ",
    "devstate_base_devno         ",
    "devstate_base_time          ",
    "devstate_base_ipaddr        ",
    "devstate_base_sid           ",
    "devstate_base_type          ",
    "devstate_base_version       ",
    "devstate_base_cpu           ",
    "devstate_base_sdram         ",
    "devstate_base_flash         ",
    "devstate_base_ethnum        ",
    "devstate_base_syncnum       ",
    "devstate_base_asyncnum      ",
    "devstate_base_switchnum     ",
    "devstate_base_usbnum        ",
    "devstate_base_prnnum        ",
    "devstate_base_cpu_used      ",
    "devstate_base_sdram_used    ",
    "devstate_base_tty_configed  ",
    "devstate_base_tty_connected ",
    "devstate_base_eth0_ip       ",
    "devstate_base_eth0_mask     ",
    "devstate_base_eth0_mac      ",
    "devstate_base_eth0_state    ",
    "devstate_base_eth0_speed    ",
    "devstate_base_eth0_duplex   ",
    "devstate_base_eth0_autonego ",
    "devstate_base_eth0_txbytes  ",
    "devstate_base_eth0_txpackets",
    "devstate_base_eth0_rxbytes  ",
    "devstate_base_eth0_rxpackets",
    "devstate_base_eth1_ip       ",
    "devstate_base_eth1_mask     ",
    "devstate_base_eth1_mac      ",
    "devstate_base_eth1_state    ",
    "devstate_base_eth1_speed    ",
    "devstate_base_eth1_duplex   ",
    "devstate_base_eth1_autonego ",
    "devstate_base_eth1_txbytes  ",
    "devstate_base_eth1_txpackets",
    "devstate_base_eth1_rxbytes  ",
    "devstate_base_eth1_rxpackets",
    "devstate_base_usbstate      ",
    "devstate_base_usbfiles      ",
    "devstate_base_prnname       ",
    "devstate_base_prnstate      ",
    "devstate_base_prnfiles      ",
    "devstate_base_switchinfo    ",
    "devstate_base_config        ",
    "devstate_base_process       ",
    "devstate_base_sendreq       ",
    NULL
};

struct devstate_base{
    bool   isok                 ;
    string data[ENUM_base_END]  ;
    devstate_base * next        ;
};

class MYMDB{
public:
    char * db_ip;
    char * db_user;
    char * db_pwd;
    char * db_name;
    devstate_base * base_insertlst;
    MYMDB()
    {
        db_ip           = NULL;
        db_user         = NULL;
        db_pwd          = NULL;
        db_name         = NULL;
        base_insertlst  = NULL;
    }
    void set(char* ip, char * user, char* pwd, char* name)
    {
        db_ip = ip;
        db_user = user;
        db_pwd = pwd;
        db_name = name;
        base_insertlst = NULL;
    }
    bool new_base(string devid) //使用主键new，其余信息置空
    {
        //base_insertlst 
        struct devstate_base * p = base_insertlst;
        struct devstate_base * pp = p;
        while( p != NULL){
            pp = p;
            p = pp->next;
        }

        p = new(nothrow) devstate_base;
        if(p == NULL){
            printf("[%d] new devstate_base 失败！动态空间申请失败！\n", getpid());
            return false;
        }
        pp->next = p;
        p->isok = 0;
        p->next = NULL;
        for(int i = 0; base(i)!=base(ENUM_base_END); ++i)
            p->data[i] = "";
        
        p->data[ENUM_base_devid] = devid;
        p->data[ENUM_base_devno] = "1";
        return true;    
    
    }
    bool add_base(string devid, int st, int ed, string* datalst) //add所有信息
    {
        struct devstate_base * p = base_insertlst;
        while(p!=NULL){
            if(p->data[ENUM_base_devid] == devid)
                break;
            p = p->next;
        }
        if(!p){
            return false;
        }
        for(int i = 0; (i <= ed-st) && (st+i != base(ENUM_base_END)); ++i){
            p->data[st + i] = datalst[i];
        }
        return true;
    }
    bool chk_base(string devid)
    {
        struct devstate_base * p = base_insertlst;
        while(p!=NULL){
            if(p->data[ENUM_base_devid] == devid)
                break;
            p = p->next;
        }
        if(!p){
            return false;
        }

        for(int i = 0; i < base(ENUM_base_END); ++i){
            if(p->data[i] == "")
                return false;
        }
        
        p->isok = true;
        return true;
    }
    int  base_todb()
    {
        int succ = 0;
        struct devstate_base * p = base_insertlst;
        while(p!=NULL){
            if(p->isok){
                string insert_sql = "insert into `devstate_base`";
                insert_sql += "(";
                for(int i = 0; i < base(ENUM_base_END); ++i){
                    if(i) insert_sql += ",";
                    insert_sql += sql_base_colname[i];
                }
                insert_sql += ")";
                insert_sql += "values(";
                for(int i = 0; i < base(ENUM_base_END); ++i){
                    if(i) insert_sql += ",";
                    insert_sql += "'";
                    insert_sql += p->data[i];
                    insert_sql += "'";
                }
                insert_sql += ")";

                if(! todb(insert_sql.c_str()))
                    succ++;
            
            }
            p = p->next;
        }
        return succ;
    }
    void close()
    {
        struct devstate_base* p = base_insertlst;
        while(p!=NULL){
            struct devstate_base * temp = p->next;
            delete p;
            p = temp;
        }
    }
private:
    int todb(const char* sql)
    {
        MYSQL *mysql;

        /* 初始化 mysql 变量，失败返回NULL */
        if ((mysql = mysql_init(NULL))==NULL) {
            printf("[%d] mysql_init failed!\n", getpid());
            return -1;
        }
        /* 连接数据库，失败返回NULL
        1、mysqld没运行
        2、没有指定名称的数据库存在 */
        if (mysql_real_connect(mysql, db_ip, db_user, db_pwd, db_name, 0, NULL, 0)==NULL) {
            printf("[%d] mysql_real_connect failed(%s)\n", getpid(), mysql_error(mysql));
            return -1;
        }
        /* 设置字符集，否则读出的字符乱码，即使/etc/my.cnf中设置也不行 */
        mysql_set_character_set(mysql, "gbk");

        if(mysql_query(mysql, sql)){
            printf("[%d] insert to db failed(%s)\n", getpid(), mysql_error(mysql));
            return -1;
        }

        /* 关闭整个连接 */
        mysql_close(mysql);        
        return 0;
    }
};

