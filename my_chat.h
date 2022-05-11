#ifndef MY_CHAT_H
#define MY_CHAT_H
#include<pthread.h>
//倾听的最大数量
#define LISTENG 20
//端口
#define SERV_PORT 4508
#define MAX 1024

pthread_mutex_t mutex_cli; 
pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_cond_t cond_cli;

typedef struct{ 
    int t;
    long size;
    int cont;
    int type;
    int type1;
    int send_fd;
    int recv_fd;
    int line;
    int relation;//0 申请 1 好友 -1 黑名单
    char send_id[4];
    char recv_id[4];
    char send_name[50];
    char recv_name[50];
    char read_buf[1024];
    char write_buf[1024];
    char buf[1024];
}bag;

typedef struct{
    int id[500];
    int fri_id[500];
    int buf_num;
    char buf[500][1024];
    char time[500][100];
}fri_mes;

typedef struct{
    int gro_id;//群号
    int id[500];//发言人的账号
    int number;//记录条数
    char buf[500][1024];//聊天记录
    char time[500][100];//时间
}gro_mes;

typedef struct{
    int gro_id;
    char gro_name[50];
    int gro_mem_id;
    int gro_chmod;//群主 2 管理员 1 普通成员 0
}gro;

typedef struct friend{
    int fri_number;
    char fri_name[500][50];
    int fri_id[500];
    int fri_line[500];
}fri;

typedef struct {
    int id[500];
    int chmod[500];
    int number;
}group;

typedef struct{
    int event_fd;
    int conn_fd;
}FD;

typedef struct BOX {
    int gro_number;//群消息处理数量
    int gro_id[500];//请求进群的人的账号
    int gro_id1[500];//请求群的群号
    //文件数量
    int file_number;
    //文件名字
    char file_name[500][100];
    //文件发送者账号
    int file_id[500];
    //接收消息的人的账号
    int recv_id;
    //发送消息人的账号
    int send_id[500];
    //发送好友请求人的账号
    int plz_id[500];
    //发送的消息
    char read_buf[500][MAX];
    //发送的请求
    char write_buf[500][100];
    //消息数量
    int talk_number;
    //请求数量
    int friend_number;
    //群里发送消息的人
    int send_id1[500];
    //消息内容
    char message[500][MAX];
    //群消息数量
    int number;
    //群号
    int group_id[500];//消息的群号
    struct BOX *next;
} BOX;

BOX *box_head;
BOX *box_tail;

#endif
