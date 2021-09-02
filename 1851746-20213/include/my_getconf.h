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

//״̬
#define CFITEM_NONE -1
#define CFITEM_FIND 1
//����
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
            printf("[%d] getall conf failed���ļ���ʧ�ܣ�%s��\n", 
                                            getpid(), conf_path);
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
                int minpos = (int)buf.length(); //��¼��ǰ��Сλ��
                int k; //��Сλ�÷ָ���
                for(int i = 0; i < (int)dlim_mark.length(); ++i){
                    size_t pos = buf.find(dlim_mark[i]);
                    if(pos != buf.npos && (int)pos < minpos){
                        k = i;
                        minpos = pos;
                    }
                }
                if(minpos == (int)buf.length())
                    continue;//no dlim
                //�� name ��״̬���
                if(!rewrite && conf_item[i].status == CFITEM_FIND)
                    continue;//����ֵ
                //ȡ����
                int data_st = buf.find(dlim_mark[k]);
                int data_sz = buf.length() - data_st - 1;
                string bufstring = buf.substr(data_st+1, data_sz);
                istringstream datastring(bufstring);
                datastring >> conf_item[i].data;
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
            printf("[%d] get conf failed���ļ���ʧ�ܣ�%s��\n", 
                                            getpid(), conf_path);
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
            for(k = 0; k < (int)dlim_mark.length(); ++k)
                if((buf.find(dlim_mark[k])) != buf.npos)
                    break;
            if(k == (int)dlim_mark.length())
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
    
    //���ȡ�ò����Ƿ�Ϸ����Ϸ���ֵ��void*�����سɹ��Ĳ�������
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

