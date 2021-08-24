#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

using namespace std;

#include "my_socket.h"

// __HEAD_PACK
// +-----------+------------+----------------------+
// | head_type | head_index | pack_size            |          
// +-----------+------------+----------------------+
// | pad                    | data_size            |
// +------------------------+----------------------+

struct HEADPACK{
    u_char head_type;
    u_char head_index;
    u_short pack_size;
    u_short pad;
    u_short data_size;
};

class NETPACK{
public:
    /*包数据*/
    HEADPACK head;
    string data;
    
    /*构造函数*/
    NETPACK() //拆包
    {
        std::fill(&head, &head + sizeof(head), 0);
        data = "";
    };
    NETPACK(u_short head_info) //封包
    {
        memcopy(&(head.head_type), &head_info, 1);
        memcopy(&(head.head_index), (void*)(&head_info) + 1, 1);
        head.pad = 0x0000;
    }
    /*操作函数*/
    //计算包大小
    u_short size()
    {
        head.data_size = data.size();
        return head.pack_size = head.data_size + 8;
    }
    //装载包
    bool upload(SOCK_INFO* sinfo)
    {
        if( !head.pack_size || SEND_BUFSIZE-sinfo->sendbuf_len < head.pack_size)
            return false;
        u_short dsize = head.data_size;
        u_short psize = head.pack_size;
        //主机序转网络序
        head.data_size = htons(head.data_size);
        head.pack_size = htons(head.pack_size);
        //to sendbuf
        memcpy(sinfo->sendbuf + sinfo->sendbuf_len, &head, psize-dsize);
        memcpy(sinfo->sendbuf + sinfo->sendbuf_len + psize-dsize, data.c_str(), dsize);
        sinfo->sendbuf_len += psize;
        return true;
    }
    //卸载包
    bool dwload(SOCK_INFO* sinfo, u_short pack_size, u_short data_size )
    {
        if( !pack_size || sinfo->recvbuf_len < pack_size)
            return false;
        
        memcpy(&head, sinfo->recvbuf, pack_size - data_size);
        head.pack_size = ntohs(head.pack_size);
        head.data_size = ntohs(head.data_size);
        printf("[%d] 得到报文头 head_type:0x%02x head_index:0x%02x pack_size:%d data_size:%d\n"
                , getpid(), head.head_type, head.head_index, head.pack_size, head.data_size);

        u_char c = *(sinfo->recvbuf + pack_size);
        *(sinfo->recvbuf + pack_size) = '\0';
        data = string(sinfo->recvbuf + pack_size - data_size);
        *(sinfo->recvbuf + pack_size) = c;
        printf("[%d] 得到报文(len:%d) data:%s\n"
                , getpid(), data.length(), data.c_str());
        
        sinfo->recvbuf_len -= pack_size;
        if(sinfo->recvbuf_len > 0)
            memmove(sinfo->recvbuf, sinfo->recvbuf + pack_size, sinfo->recvbuf_len);
        return true;
    }
};





