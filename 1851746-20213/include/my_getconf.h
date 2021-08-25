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

//读取配置文件信息
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
    //取所有预定义配置项
    int getall(int rewrite = 0)
    {
        ifstream fin;
        fin.open(conf_path, ios::in);
        if(!fin.is_open()){
            // printf("[%d] getall conf failed！文件打开失败（%s）\n", 
            //                                 getpid(), conf_path);
            return -1; //文件打开失败
        }
        string buf;
        int  getnum = 0;//正确取得的项数
        while(getline(fin, buf))
        {
            int i = 0;
            for(i=0; conf_item[i].name != NULL; ++i){
                //去掉注释
                if(buf.find(note_mark)!=buf.npos){
                    int st = buf.find(note_mark);
                    buf = buf.substr(0, st);
                }
                //判断name是否匹配
                if(buf.find(conf_item[i].name) == buf.npos)
                    continue;//no name
                //去掉name之前的
                int name_st = buf.find(conf_item[i].name);
                int name_sz = buf.length() - name_st;
                buf = buf.substr(name_st, name_sz);
                //是否存在分割符
                int k = 0;
                for(k = 0; k < dlim_mark.length(); ++k)
                    if((buf.find(dlim_mark[k])) != buf.npos)
                        break;
                if(k == dlim_mark.length())
                    continue;//no dlim
                //该 name 的状态检查
                if(!rewrite && conf_item[i].status == CFITEM_FIND)
                    continue;//已有值
                //取内容
                int data_st = buf.find(dlim_mark[k]);
                int data_sz = buf.length() - data_st - 1;
                conf_item[i].data = buf.substr(data_st+1, data_sz);
                conf_item[i].status = CFITEM_FIND;
                getnum+=1;
            }////循环所有
        }
        fin.close();
        return getnum;//返回正确取得的项数
    }
    //取单个配置项
    string get(const char* getname) 
    {//只能取第一次匹配到的
        string data = "";
        ifstream fin;
        fin.open(conf_path, ios::in);
        if(!fin.is_open()){
            // printf("[%d] get conf failed！文件打开失败（%s）\n", 
            //                                 getpid(), conf_path);
            return ""; //文件打开失败
        }
        string buf;
        while(getline(fin,buf))
        {
            //去掉注释
            if(buf.find(note_mark)!=buf.npos){
                int st = buf.find(note_mark);
                buf = buf.substr(0, st);
            }
            //判断name是否匹配
            if(buf.find(getname) == buf.npos)
                continue;//no name
            //去掉name之前的
            int name_st = buf.find(getname);
            int name_sz = buf.length() - name_st;
            buf = buf.substr(name_st, name_sz);
            //是否存在分割符
            int k = 0;
            for(k = 0; k < dlim_mark.length(); ++k)
                if((buf.find(dlim_mark[k])) != buf.npos)
                    break;
            if(k == dlim_mark.length())
                continue;//no dlim
            //取内容
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