#ifndef MY_GETCONF_H
#define MY_GETCONF_H

#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
using namespace std;

//状态
#define CFITEM_NONE -1
#define CFITEM_FIND 1
//类型
#define CFTYPE_STR      1
#define CFTYPE_INT8     2
#define CFTYPE_UINT8    3
#define CFTYPE_INT16    4
#define CFTYPE_UINT16   5
#define CFTYPE_INT32    6
#define CFTYPE_UINT32   7

struct CONF_ITEM{
    int         status;
    const char* name;
    void      * var ;
    int         min ;
    int         max ;
    u_char      type;
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
            printf("[%d] getall conf failed！文件打开失败（%s）\n", 
                                            getpid(), conf_path);
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
                int minpos = (int)buf.length(); //记录当前最小位置
                int k; //最小位置分隔符
                for(int i = 0; i < (int)dlim_mark.length(); ++i){
                    size_t pos = buf.find(dlim_mark[i]);
                    if(pos != buf.npos && (int)pos < minpos){
                        k = i;
                        minpos = pos;
                    }
                }
                if(minpos == (int)buf.length())
                    continue;//no dlim
                //该 name 的状态检查
                if(!rewrite && conf_item[i].status == CFITEM_FIND)
                    continue;//已有值
                //取内容
                int data_st = buf.find(dlim_mark[k]);
                int data_sz = buf.length() - data_st - 1;
                string bufstring = buf.substr(data_st+1, data_sz);
                istringstream datastring(bufstring);
                datastring >> conf_item[i].data;
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
            printf("[%d] get conf failed！文件打开失败（%s）\n", 
                                            getpid(), conf_path);
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
            for(k = 0; k < (int)dlim_mark.length(); ++k)
                if((buf.find(dlim_mark[k])) != buf.npos)
                    break;
            if(k == (int)dlim_mark.length())
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
    
    //检查取得参数是否合法，合法则赋值给void*，返回成功的参数数量
    int chkvar()
    {
        int chk_succ = 0;
        for(int i = 0; conf_item[i].name != NULL; ++i){
            if(conf_item[i].status == CFITEM_FIND){
                if(conf_item[i].data == "")
                    continue;
                
                istringstream datain(conf_item[i].data);
                if(conf_item[i].type == CFTYPE_STR){
                    chk_succ++;
                    memcpy(conf_item[i].var, conf_item[i].data.c_str(), conf_item[i].data.length()+1);
                }
                else if(conf_item[i].type == CFTYPE_INT8){
                    int8_t temp;
                    datain >> temp;
                    if(temp < (int8_t)conf_item[i].min || temp > (int8_t)conf_item[i].max)
                        continue;

                    chk_succ++;
                    memcpy(conf_item[i].var, &temp, sizeof(int8_t));
                }
                else if(conf_item[i].type == CFTYPE_UINT8){
                    uint8_t temp;
                    datain >> temp;
                    if(temp < (uint8_t)conf_item[i].min || temp > (uint8_t)conf_item[i].max)
                        continue;
                    
                    chk_succ++;
                    memcpy(conf_item[i].var, &temp, sizeof(uint8_t));
                }
                else if(conf_item[i].type == CFTYPE_INT16){
                    int16_t temp;
                    datain >> temp;
                    if(temp < (int16_t)conf_item[i].min || temp > (int16_t)conf_item[i].max)
                        continue;

                    chk_succ++;
                    memcpy(conf_item[i].var, &temp, sizeof(int16_t));
                }
                else if(conf_item[i].type == CFTYPE_UINT16){
                    uint16_t temp;
                    datain >> temp;
                    if(temp < (uint16_t)conf_item[i].min || temp > (uint16_t)conf_item[i].max)
                        continue;

                    chk_succ++;
                    memcpy(conf_item[i].var, &temp, sizeof(uint16_t));
                }
                else if(conf_item[i].type == CFTYPE_INT32){
                    int32_t temp;
                    datain >> temp;
                    if(temp < (int32_t)conf_item[i].min || temp > (int32_t)conf_item[i].max)
                        continue;
                    
                    chk_succ++;
                    memcpy(conf_item[i].var, &temp, sizeof(int32_t));
                }
                else if(conf_item[i].type == CFTYPE_UINT32){
                    uint32_t temp;
                    datain >> temp;
                    if(temp < (uint32_t)conf_item[i].min || temp > (uint32_t)conf_item[i].max)
                        continue;
                    chk_succ++;
                    memcpy(conf_item[i].var, &temp, sizeof(uint32_t));
                }
            }
        }
        return chk_succ;
    }

private:
    size_t getsize(int type)
    {
        switch(type){
            case CFTYPE_INT8:
                return sizeof(int8_t);
            case CFTYPE_UINT8:
                return sizeof(uint8_t);
            case CFTYPE_INT16:
                return sizeof(int16_t);
            case CFTYPE_UINT16:
                return sizeof(uint16_t);
            case CFTYPE_INT32:
                return sizeof(int32_t);
            case CFTYPE_UINT32:
                return sizeof(uint32_t);
            default:
                return 0;
        }
        return 0;
    }
};
#endif

