
#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>

#include "../include/my_getconf.h"

CONF_ITEM cfitem[] = {
{CFITEM_NONE, "������IP��ַ", ""},
{CFITEM_NONE, "���̽��ճɹ����˳�", ""},
{CFITEM_NONE, "��С�����ն�����", ""},
{CFITEM_NONE, "��������ն�����", ""},
{CFITEM_NONE, "ÿ���ն���С��������", ""},
{CFITEM_NONE, "ÿ���ն������������", ""},
{CFITEM_NONE, "ɾ����־�ļ�", ""},
{CFITEM_NONE, "DEBUG����", ""},
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


