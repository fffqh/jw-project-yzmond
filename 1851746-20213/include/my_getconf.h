#ifndef MY_SOCKET_H
#define MY_SOCKET_H

#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
//#include <sys/socket.h>
using namespace std;

#define CFITEM_NONE -1
#define CFITEM_FIND 1

struct CONF_ITEM{
    int         status;
    const char* name;
    string      data;
};

//��ȡ�����ļ���Ϣ
class CONFINFO{
public:
    const char * conf_path;
    string dlim_mark;
    string note_mark;
    CONF_ITEM* conf_item;

    CONFINFO(const char* path, const char* dm, const char* nm, CONF_ITEM* cfitem)
    {
        conf_path = path;
        dlim_mark = string(dm);
        note_mark = string(nm);
        conf_item = cfitem;
    }
    //ȡ����Ԥ����������
    int getall(int rewrite = 0)
    {
        ifstream fin;
        fin.open(conf_path, ios::in);
        if(!fin.is_open()){
            // printf("[%d] getall conf failed���ļ���ʧ�ܣ�%s��\n", 
            //                                 getpid(), conf_path);
            return -1; //�ļ���ʧ��
        }
        string buf;
        int  getnum = 0;//��ȷȡ�õ�����
        while(getline(fin, buf))
        {
            int i = 0;
            for(i=0; conf_item[i].name != NULL; ++i){
                //ȥ��ע��
                if(buf.find(note_mark)!=buf.npos){
                    int st = buf.find(note_mark);
                    buf = buf.substr(0, st);
                }
                //�ж�name�Ƿ�ƥ��
                if(buf.find(conf_item[i].name) == buf.npos)
                    continue;//no name
                //ȥ��name֮ǰ��
                int name_st = buf.find(conf_item[i].name);
                int name_sz = buf.length() - name_st;
                buf = buf.substr(name_st, name_sz);
                //�Ƿ���ڷָ��
                int k = 0;
                for(k = 0; k < dlim_mark.length(); ++k)
                    if((buf.find(dlim_mark[k])) != buf.npos)
                        break;
                if(k == dlim_mark.length())
                    continue;//no dlim
                //�� name ��״̬���
                if(!rewrite && conf_item[i].status == CFITEM_FIND)
                    continue;//����ֵ
                //ȡ����
                int data_st = buf.find(dlim_mark[k]);
                int data_sz = buf.length() - data_st - 1;
                conf_item[i].data = buf.substr(data_st+1, data_sz);
                conf_item[i].status = CFITEM_FIND;
                getnum+=1;
            }////ѭ������
        }
        fin.close();
        return getnum;//������ȷȡ�õ�����
    }
    //ȡ����������
    string get(const char* getname) 
    {//ֻ��ȡ��һ��ƥ�䵽��
        string data = "";
        ifstream fin;
        fin.open(conf_path, ios::in);
        if(!fin.is_open()){
            // printf("[%d] get conf failed���ļ���ʧ�ܣ�%s��\n", 
            //                                 getpid(), conf_path);
            return ""; //�ļ���ʧ��
        }
        string buf;
        while(getline(fin,buf))
        {
            //ȥ��ע��
            if(buf.find(note_mark)!=buf.npos){
                int st = buf.find(note_mark);
                buf = buf.substr(0, st);
            }
            //�ж�name�Ƿ�ƥ��
            if(buf.find(getname) == buf.npos)
                continue;//no name
            //ȥ��name֮ǰ��
            int name_st = buf.find(getname);
            int name_sz = buf.length() - name_st;
            buf = buf.substr(name_st, name_sz);
            //�Ƿ���ڷָ��
            int k = 0;
            for(k = 0; k < dlim_mark.length(); ++k)
                if((buf.find(dlim_mark[k])) != buf.npos)
                    break;
            if(k == dlim_mark.length())
                continue;//no dlim
            //ȡ����
            int data_st = buf.find(dlim_mark[k]);
            int data_sz = buf.length() - data_st - 1;
            data = buf.substr(data_st+1, data_sz);
            break;
        }
        fin.close();
        return data;
    }
    
};

#endif