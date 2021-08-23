#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>

bool decrypt(u_char* key, u_int random_num, u_int & svr_time);
void encrypt(u_char* key, u_int & random_num, u_int & svr_time);

