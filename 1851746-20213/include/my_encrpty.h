#ifndef MY_ENCRPTY_H
#define MY_ENCRPTY_H

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>

bool decrypt(u_char* key, u_int random_num, u_int & svr_time);
void encrypt(u_char* key, u_int & random_num, u_int & svr_time);

void encrypt_client_auth(u_char* content, u_int & random_num);
void decrypt_client_auth(u_char* content, u_int random_num);

#endif