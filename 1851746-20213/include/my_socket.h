#ifndef MY_SOCKET_H
#define MY_SOCKET_H

#include <sys/socket.h>

#define RECV_BUFSIZE    8200
#define SEND_BUFSIZE    8200

typedef struct{
    int    sockfd;
    u_int  devid;
    int    recvbuf_len;
    int    sendbuf_len;
    u_char recvbuf[RECV_BUFSIZE];
    u_char sendbuf[SEND_BUFSIZE];

}SOCK_INFO;


#endif //MY_SOCKET_H