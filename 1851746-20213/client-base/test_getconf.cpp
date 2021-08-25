
#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>

#include "../include/my_getconf.h"

CONF_ITEM cfitem[] = {
{CFITEM_NONE, "服务器IP地址", ""},
{CFITEM_NONE, "进程接收成功后退出", ""},
{CFITEM_NONE, "最小配置终端数量", ""},
{CFITEM_NONE, "最大配置终端数量", ""},
{CFITEM_NONE, "每个终端最小虚屏数量", ""},
{CFITEM_NONE, "每个终端最大虚屏数量", ""},
{CFITEM_NONE, "删除日志文件", ""},
{CFITEM_NONE, "DEBUG设置", ""},
{CFITEM_NONE, NULL, ""}
};


int main()
{
    CONFINFO conf_info("./ts.conf", " \t", "#", cfitem);
    printf("the return of getall : %d\n", conf_info.getall());
    for(int i = 0; cfitem[i].name != NULL; ++i)
        printf("%s-%s\n", cfitem[i].name, cfitem[i].data.c_str());
    return 0;

}


