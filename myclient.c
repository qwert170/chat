#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h> 
#include<stdlib.h>
#include"my_chat.h"
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>

bag *recv_bag;
bag *send_bag;
int sing;
BOX *box;
pthread_t tid;
fri *list;
fri_mes *mes;
gro_mes *message;
group *lis;

int socket_init(struct sockaddr_in client_addr)
{
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);
    
    client_addr.sin_addr.s_addr=htonl(INADDR_ANY);//通信对象的IP地址，0.0.0.0表示所有IP地址
    //client_addr.sin_addr.s_addr=inet_addr("192.168.30.155");
    client_addr.sin_family=AF_INET; //IPv4协议
    client_addr.sin_port=htons(SERV_PORT);//应用层端口号
    //绑定套接字和结构体----主机自检端口号是否重复，IP是否准确
    if(connect(sock_fd,(struct sockaddr *)&client_addr,sizeof(client_addr))<0)
    {
        perror("connect failed");
        return -1;
    }else{
        printf("客户端初始化完成\n");
    }
    return sock_fd;

}

void getpasswd(char *passwd)
{
    struct termios oldt, newt;
    char ch;
    tcgetattr(0, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    int  i = 0;

    while(1)
    {
        tcsetattr(0, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(0, TCSANOW, &oldt);
        if(ch=='\n')
        {
            passwd[i]='\0';
            break;
        }
        passwd[i]=ch;
        i++;
        printf("*");
    }
    printf("\n");
}

int login(int fd)
{
    int ret;

    printf("--------------------------------------------\n");
    printf("                 请输入账号：\n");
    scanf("%s",send_bag->send_id);
    getchar();
    printf("                 请输入密码：\n");
    getpasswd(send_bag->read_buf);
    memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
    ret=send(fd,send_bag,sizeof(bag),0);//发帐号密码
    if(ret<0)
    {
        perror("send failed");
        exit(1);
    }
    pthread_mutex_lock(&mutex_cli);
    while(sing==0){
        pthread_cond_wait(&cond_cli, &mutex_cli);
    }
    pthread_mutex_unlock(&mutex_cli);
    sing=0;

}

int resign(int fd)
{
    int ret;
    char passwd[8];
    memset(passwd,0,sizeof(passwd));
    printf("--------------------------------------------\n");
    printf("                 请输入昵称：\n");
    scanf("%s",send_bag->send_name);
    getchar();
    printf("                 请输入密码：\n");
    getpasswd(send_bag->read_buf);
    printf("                 请再输入一次密码：\n");
    getpasswd(passwd);

    while(1){
        if(strcmp(send_bag->read_buf,passwd)==0)
        {
            printf("                 两次输入一致！\n");
            ret=send(fd,send_bag,sizeof(bag),0);//发送帐号密码
            if(ret<0)
            {
                perror("send failed");
                exit(1);
            }

            pthread_mutex_lock(&mutex_cli);
            pthread_cond_wait(&cond_cli, &mutex_cli);
            pthread_mutex_unlock(&mutex_cli);

            printf("                 注册成功！你的账号是%s\n",send_bag->send_id);
            printf("                 请重新登陆！\n");
            break;
        }else{
            printf("                 输入不正确！请再次输入！\n");
            printf("                 请输入：\n");
            getpasswd(passwd);
            continue;
        }
    }
   
}

int add_fri(int fd)
{
    int t;

    printf("                  输入要添加的账号：\n");
    scanf("%s",send_bag->recv_id);
    getchar();

    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    t=atoi(send_bag->write_buf);
    if(t==1)
    {
        printf("                  该人已经是你的好友！\n");
    }else if(t==-2)
    {
        printf("                  该账号不存在！\n");
    }else if(t==-1)
    {
        printf("                  该人已被你拖入黑名单！\n");
    }else if(t==0)
    {
        printf("                  已发送请求！\n");
    }else if(t==3){
        printf("                  已发送过申请！\n");
    }else{
        printf("                  添加错误！\n");
    }


    return 0;
}

int xiugai(int fd)
{
    int choice;
    
    printf("--------------------------------------------\n");
    printf("                1--修改昵称\n");
    printf("                2--修改密码\n");
    printf("                0--退出\n");
    printf("--------------------------------------------\n");
    printf("                请选择\n");
    scanf("%d",&choice);
    getchar();
    switch(choice)
    {
        case 1:
        {
            send_bag->type1=1;
            printf("                输入修改的名字：\n");
            scanf("%s",send_bag->read_buf);
            getchar();

            send(fd,send_bag,sizeof(bag),0);
            pthread_mutex_lock(&mutex_cli);
            pthread_cond_signal(&cond_cli);
            pthread_mutex_unlock(&mutex_cli);
            printf("修改完成！\n");
            break;
        }
        case 2:
        {
            send_bag->type1=2;
            printf("                输入修改的密码：\n");
            getpasswd(send_bag->read_buf);

            send(fd,send_bag,sizeof(bag),0);
            pthread_mutex_lock(&mutex_cli);
            pthread_cond_signal(&cond_cli);
            pthread_mutex_unlock(&mutex_cli);        
            printf("修改完成！\n");
            break;
        }
        case 0:
            return 0;
        default:
            printf("                  输入错误！返回！\n");
            return 0;
    }
}

void menu1()
{
    
        printf("--------------------------------------------\n");
        printf("                  菜单\n");
        printf("--------------------------------------------\n");
        printf("             1--开始私聊\n");
        printf("             2--添加好友\n");
        printf("             3--删除好友\n");
        printf("             4--显示好友列表\n");
        printf("             5--查看好友聊天记录\n");
        printf("             6--好友拖入黑名单\n");
        printf("             7--好友移出黑名单\n");
        printf("             8--开始群聊\n");
        printf("             9--创建群聊\n");
        printf("             10--解散群聊\n");
        printf("             11--申请加群\n");
        printf("             12--退出群聊\n");
        printf("             13--查看群聊天记录\n");
        printf("             14--管理群聊\n");
        printf("             15--修改个人信息\n");
        printf("             16--处理消息\n");
        printf("             17--文件传送\n");
        printf("             18--文件接收\n");
        printf("             19--查看群好友列表\n");
        printf("             20--查看个人信息\n");
        printf("             21--查看群列表\n");
        printf("             0--退出\n");
        printf("--------------------------------------------\n");
        printf("             请选择：\n");
}

void menu()
{
    printf("--------------------------------------------\n");
    printf("-----------------登录界面--------------------\n");
    printf("--------------------------------------------\n");
    printf("                 1--登陆\n");
    printf("                 2--注册\n");
    printf("                 0--退出\n");
    printf("--------------------------------------------\n");
    printf("--------------------------------------------\n");
    printf("                 请选择：\n");
}

int deal(int fd)
{
    int choose;
    int i;
    int choice;
    printf("--------------------------------------------\n");
    printf("                 1--处理好友申请\n");
    printf("                 2--查看好友消息\n");
    printf("                 3--查看群消息\n");
    printf("                 4--处理加群请求\n");
    printf("                 0--退出\n");
    printf("--------------------------------------------\n");
    printf("                 请选择：\n");
    scanf("%d",&choose);
    getchar();
    switch(choose)
    {
        case 1:
        {
            send_bag->type1=1;
            if(box->friend_number==0)
            {
                printf("暂无好友申请！\n");
                getchar();
                break;
            }else{
                for(i=0;i<box->friend_number;i++)
                {
                    printf("                 账号为%d请求加你为好友!\n",box->plz_id[i]);
                    printf("                 1--同意\n");
                    printf("                 2--拒绝\n");
                    printf("--------------------------------------------\n");
                    printf("                 请选择：\n");
                    scanf("%d",&choice);
                    getchar();
                    if(choice==1)
                    {
                        strcpy(send_bag->read_buf,"tongyi");
                        sprintf(send_bag->recv_id,"%d",box->plz_id[i]);
                        send(fd,send_bag,sizeof(bag),0);

                    }else if(choice ==2)
                    {
                        strcpy(send_bag->read_buf,"jujue");
                        sprintf(send_bag->recv_id,"%d",box->plz_id[i]);
                        send(fd,send_bag,sizeof(bag),0);

                    }else{
                        printf("                 输入错误！\n");
                        return 0;
                        //break;
                    }
                }
                box->friend_number=0;
                printf("                 处理完成！\n");
                memset(send_bag->read_buf,0,1024);
                break;
            }
        }
        case 2:
        {
            send_bag->type1=2;
            if(box->talk_number==0)
            {
                printf("暂无好友消息！\n");
                getchar();
                break;
            }else{
                for(i=0;i<box->talk_number;i++)
                {
                    printf("账号%d:%s\n", box->send_id[i], box->read_buf[i]);
                }
                box->talk_number=0;
                printf("处理完成！\n");
                break;
            }
        }
        case 3:
        {
            send_bag->type1=3;
            if(box->number==0)
            {
                printf("暂无群聊消息！\n");
                getchar();
                break;
            }else{
                for(i=0;i<box->number;i++)
                {
                    printf("群号为%d 账号%d:%s\n", box->group_id[i],box->send_id1[i], box->message[i]);
                }
                box->number=0;
                printf("处理完成！\n");
                break;
            }
        }
        case 4:
        {
            send_bag->type1=4;
            if(box->gro_number==0)
            {
                printf("暂无加群申请！\n");
                getchar();
                break;
            }else{
                for(i=0;i<box->gro_number;i++)
                {
                    printf("                 账号为%d请求加入群%d!\n",box->gro_id[i],box->gro_id1[i]);
                    printf("                 1--同意\n");
                    printf("                 2--拒绝\n");
                    printf("--------------------------------------------\n");
                    printf("                 请选择：\n");
                    scanf("%d",&choice);
                    getchar();
                    if(choice==1)
                    {
                        strcpy(send_bag->read_buf,"tongyi");
                        sprintf(send_bag->recv_id,"%d",box->gro_id[i]);
                        sprintf(send_bag->write_buf,"%d",box->gro_id1[i]);
                        send(fd,send_bag,sizeof(bag),0);
                    }else if(choice ==2)
                    {
                        printf("已经拒绝他进群！\n");
                        strcpy(send_bag->read_buf,"jujue");
                        sprintf(send_bag->recv_id,"%d",box->gro_id[i]);
                        sprintf(send_bag->write_buf,"%d",box->gro_id1[i]);
                        send(fd,send_bag,sizeof(bag),0);
                    }else{
                        printf("                 输入错误！\n");
                        return 0;
                    }
                }
                box->gro_number=0;
                printf("                 处理完成！\n");
                memset(send_bag->read_buf,0,1024);
                break;
            }
        }
        case 0:
            break;
        default:
        {
            printf("输入错误！\n");
            printf("按任意键返回！\n");
            getchar();
            break;
        }
    }
}

int del_fri(int fd)
{
    printf("--------------------------------------------\n");
    printf("             输入要删除的好友账号：\n");
    scanf("%s",send_bag->recv_id);
    getchar();
    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"delete success")==0)
    {
        printf("                 删除成功！\n");
    }else 
    {
        printf("                 删除失败！\n");
    }
    return 0;
}

int display_fri(int fd)
{
    int i;
    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("好友列表：\n");
        for(i=0;i<list->fri_number;i++)
        {
            printf("id:%d     name:%s",list->fri_id[i],list->fri_name[i]);
            if(list->fri_line[i]==1)
            {
                printf("                    --在线\n");
            }else if(list->fri_line[i]==0)
            {
                printf("                    --掉线\n");
            }
        }
    }else{
        printf("你没有好友！\n");
    }

    return 0;
}

int chat_single(int fd)
{
    printf("--------------------------------------------\n");
    printf("              输入聊天的人的账号：\n");
    scanf("%s",send_bag->recv_id);
    getchar();
    printf("与%s开始聊天!\n",send_bag->recv_id);

    while(1)
    {
        memset(send_bag->read_buf,0,sizeof(send_bag->read_buf));
        scanf("%[^\n]",send_bag->read_buf);
        getchar();
        if(strcmp(send_bag->read_buf,"#obey")==0)
        {
            printf("与%s聊天结束！\n",send_bag->recv_id);
            break;
        }

        send(fd,send_bag,sizeof(bag),0);

        pthread_mutex_lock(&mutex_cli);
        pthread_cond_wait(&cond_cli, &mutex_cli);
        pthread_mutex_unlock(&mutex_cli);

        if(strcmp(send_bag->write_buf,"black")==0)
        {
            printf("你处于黑名单中！无法发送消息！\n");
            break;
        }else if(strcmp(send_bag->write_buf,"failed")==0)
        {
            printf("聊天失败！可能不是好友！或者暂无该账号！\n");
            break;
        }
    }
    memset(send_bag->recv_id,0,sizeof(send_bag->recv_id));
    return 0;
}

int black_fri(int fd)
{
    printf("--------------------------------------------\n");
    printf("              输入拉黑的人的账号：\n");
    scanf("%s",send_bag->recv_id);
    getchar();

    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("拉黑成功！\n");
    }else{
        printf("拉黑失败！\n");
    }
    return 0;
}

int check_fri(int fd)
{
    int i;
    printf("--------------------------------------------\n");
    printf("             查看聊天记录的好友账号（上限五百条）：\n");
    scanf("%s",send_bag->recv_id);
    getchar();

    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);


    for(i=0;i<mes->buf_num;i++)
    {
        if(mes->id[i]==atoi(send_bag->send_id))
        printf("\033[34mtime:%s   id:%d     message；%s\n\033[0m",mes->time[i],mes->id[i],mes->buf[i]);
        else
         printf("\033[32mtime:%s   id:%d     message；%s\n\033[0m",mes->time[i],mes->id[i],mes->buf[i]);
    }
    if(mes->buf_num==0)
    {
        printf("暂无与该好友的聊天记录或没有该账号！\n");
    }
    return 0;
}

int white_fri(int fd)
{
    printf("--------------------------------------------\n");
    printf("              输入移出的人的账号：\n");
    scanf("%s",send_bag->recv_id);
    getchar();

    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("移出成功！\n");
    }else{
        printf("移出失败！\n");
    }
    return 0;
}

int create_gro(int fd)
{
    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("创建成功！创建的群号为%s！\n",send_bag->recv_id);
    }else{
        printf("创建失败！\n");
    }
    return 0;
}

int dis_gro(int fd)
{
    printf("--------------------------------------------\n");
    printf("              输入解散的群聊的账号：\n");
    scanf("%s",send_bag->read_buf);
    getchar();

    send(fd,send_bag,sizeof(bag),0);
    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("解散成功！\n");
    }else if(strcmp(send_bag->write_buf,"no rights")==0)
    {
        printf("您没有权限！\n");
    }else {
        printf("解散失败！\n");
    }
    return 0;
}

int add_gro(int fd)
{
    printf("--------------------------------------------\n");
    printf("              输入申请加入的群聊的账号：\n");
    scanf("%s",send_bag->read_buf);
    getchar();

    send(fd,send_bag,sizeof(bag),0);
    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"sending")==0)
    {
        printf("已经发送申请！\n");
    }else if(strcmp(send_bag->write_buf,"sended")==0)
    {
        printf("已经发送过申请！\n");
    }else if(strcmp(send_bag->write_buf,"ed")==0)
    {
        printf("已经加入群聊！\n");
    }else{
        printf("进群失败！\n");
    }
    return 0;
}

int exit_gro(int fd)
{
    printf("--------------------------------------------\n");
    printf("              输入要退出的群聊的账号：\n");
    scanf("%s",send_bag->read_buf);
    getchar();

    send(fd,send_bag,sizeof(bag),0);
    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("退出成功！\n");
    }else if(strcmp(send_bag->write_buf,"no find")==0)
    {
        printf("您不在该群中！\n");
    }else{
        printf("退出出错！\n");
    }
    return 0;
}

int check_gro(int fd)
{
    int i;
    printf("--------------------------------------------\n");
    printf("             输入要查看聊天记录的群聊的账号：\n");
    scanf("%s",send_bag->read_buf);
    getchar();

    send(fd,send_bag,sizeof(bag),0);
    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli, &mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    for(i=0;i<message->number;i++)
    {
        if(message->id[i]==atoi(send_bag->send_id))
        printf("\033[34mtime:%s   id:%d     message；%s\n\033[0m",message->time[i],message->id[i],message->buf[i]);
        else
         printf("\033[32mtime:%s   id:%d     message；%s\n\033[0m",message->time[i],message->id[i],message->buf[i]);
    }
    return 0;
}

int chat_double(int fd)
{
    printf("--------------------------------------------\n");
    printf("              输入聊天的群聊的账号：\n");
    scanf("%s",send_bag->read_buf);
    getchar();

    printf("欢迎进入%s群聊！\n",send_bag->read_buf);
    while(1)
    {
        memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
        scanf("%[^\n]",send_bag->write_buf);
        getchar();
        if(strcmp(send_bag->write_buf,"#obey")==0)
        {
            printf("退出%s群聊！\n",send_bag->read_buf);
            break;
        }
        
        
        send(fd,send_bag,sizeof(bag),0);
        
        pthread_mutex_lock(&mutex_cli);
        pthread_cond_wait(&cond_cli, &mutex_cli);
        pthread_mutex_unlock(&mutex_cli);

        if(strcmp(send_bag->buf,"no find")==0)
        {
            printf("你未加入该群聊！\n");
            break;
        }
        if(strcmp(send_bag->buf,"failed")==0)
        {
            printf("进入失败！\n");
            break;
        }
    }

    memset(send_bag->read_buf,0,sizeof(send_bag->read_buf));
    return 0;
}

int deal_gro(int fd)
{
    int choose;

    printf("--------------------------------------------\n");
    printf("                 1--设置管理员：\n");
    printf("                 2--取消管理员\n");
    printf("                 3--群踢人\n");
    printf("                 4--返回\n");
    scanf("%d",&choose);
    getchar();
    switch(choose)
    {
        case 1:
        {
            send_bag->type1=1;
            printf("请输入要设置的管理员账号：\n");
            scanf("%s",send_bag->recv_id);
            getchar();
            printf("请输入设置群聊的账号：\n");
            scanf("%s",send_bag->read_buf);
            getchar();

            send(fd,send_bag,sizeof(bag),0);
            pthread_mutex_lock(&mutex_cli);
            pthread_cond_wait(&cond_cli, &mutex_cli);
            pthread_mutex_unlock(&mutex_cli);

            break;
        }
        case 2:
        {
            send_bag->type1=2;
            printf("请输入要取消管理的管理员账号：\n");
            scanf("%s",send_bag->recv_id);
            getchar();
            printf("请输入设置群聊的账号：\n");
            scanf("%s",send_bag->read_buf);
            getchar();

            send(fd,send_bag,sizeof(bag),0);
            pthread_mutex_lock(&mutex_cli);
            pthread_cond_wait(&cond_cli, &mutex_cli);
            pthread_mutex_unlock(&mutex_cli);
            break;
        }
        case 3:
        {
            send_bag->type1=3;
            printf("请输入被踢出人的账号：\n");
            scanf("%s",send_bag->recv_id);
            getchar();
            printf("请输入设置群聊的账号：\n");
            scanf("%s",send_bag->read_buf);
            getchar();

            send(fd,send_bag,sizeof(bag),0);
            pthread_mutex_lock(&mutex_cli);
            pthread_cond_wait(&cond_cli, &mutex_cli);
            pthread_mutex_unlock(&mutex_cli);
            break;
        }
        case 4:
        {
            printf("返回上一级！\n");
            return 0;
        }
        default:
        {
            printf("输入错误！按任意键返回！");
            getchar();
            return 0;
        }
    }

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("成功修改！\n");
    }else if(strcmp(send_bag->write_buf,"dealed")==0)
    {
        printf("已经是管理员了！\n");
    }else if(strcmp(send_bag->write_buf,"no admin")==0)
    {
        printf("该用户还不是管理员！\n");
    }else if(strcmp(send_bag->write_buf,"no possible")==0)
    {
        printf("该人是群主！\n");
    }else if(strcmp(send_bag->write_buf,"no chmod")==0)
    {
        printf("没有权限！\n");
    }else if(strcmp(send_bag->write_buf,"no find id")==0)
    {
        printf("该人不在该群！\n");
    }else if(strcmp(send_bag->write_buf,"no find gro")==0)
    {
        printf("该群不存在！\n");
    }else{
        printf("修改失败！\n");
    }
    return 0;
}

int send_file(int fd)
{
    struct stat buf;
    int pd;
    int i,j;
    char name[100];
    int len;
    j=0;
    printf("要发送好友的账号：\n");
    scanf("%s",send_bag->recv_id);
    getchar();
    printf("要发送的文件：（绝对路径）\n");
    scanf("%s",send_bag->read_buf);
    getchar();

    if((lstat(send_bag->read_buf,&buf))<0)
    {
        printf("获取文件失败！按任意键返回！\n");
        getchar();
        return 0;
    }

    if((pd=open(send_bag->read_buf,O_RDONLY))<0)
    {
        printf("无法打开文件！按任意键返回！\n");
        getchar();
        return 0;
    }

    sing=0;
    send_bag->cont=0;

    memset(send_bag->buf,0,sizeof(send_bag->buf));
    memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));

    for(i=0;i<=strlen(send_bag->read_buf);i++)
    {
        if(send_bag->read_buf[i]=='/')
        {
            j=0; 
            i++;                       
            memset(name,0,sizeof(name));
        }
        name[j]=send_bag->read_buf[i];
        j++;
    }
    name[i]='\0';
   
    strcpy(send_bag->read_buf,name);
    send_bag->t=buf.st_size;

    while((len=read(pd,send_bag->write_buf,1023))>0)//1023
    {
        send_bag->size=len;
        send(fd,send_bag,sizeof(bag),0);
        memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
        printf("cont=%d\n",send_bag->cont);
        pthread_mutex_lock(&mutex_cli);
        while(sing==0){
            pthread_cond_wait(&cond_cli, &mutex_cli);
        }
        pthread_mutex_unlock(&mutex_cli);
        send_bag->cont++;
        sing=0;
    }
    close(pd);

    sing=0;
    strcpy(send_bag->buf,"ok");
    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    while(sing==0){
        pthread_cond_wait(&cond_cli, &mutex_cli);
    }
    pthread_mutex_unlock(&mutex_cli);
    sing=0;
    
    if(strcmp(send_bag->buf,"success")==0)
    {
        printf("传送完成！\n");
    }else if(strcmp(send_bag->buf,"no online")==0)
    {
        printf("该账号不在线！\n");
    }else if(strcmp(send_bag->buf,"no find")==0)
    {
        printf("该账号不存在！\n");
    }else{
        printf("%s",send_bag->buf);
        printf("发送错误！\n");
    }
   
    return 0;
}

int recv_file(int fd)
{
    int i;
    int choice;
    send_bag->cont=0;

    if(box->file_number==0)
    {
        printf("暂无文件需要接收！按任意键退出！\n");
        getchar();
        return 0;
    }else{
        for(i=0;i<box->file_number;i++)
        {
            printf("                 %d给你发送了一个文件%s，是否接收!\n",box->file_id[i],box->file_name[i]);
            printf("                 1--同意\n");
            printf("                 2--拒绝\n");
            printf("--------------------------------------------\n");
            printf("                 请选择：\n");
            scanf("%d",&choice);
            getchar();
            if(choice==1)
            {
                    memset(send_bag->buf,0,sizeof(send_bag->buf));
                    send(fd,send_bag,sizeof(bag),0);

                    pthread_mutex_lock(&mutex_cli);
                    while(sing==0){
                        pthread_cond_wait(&cond_cli, &mutex_cli);
                    }
                    pthread_mutex_unlock(&mutex_cli);
                    sing=0;
                   
                    printf("接收完成！\n");
            }else if(choice ==2)
            {
                printf("已拒绝接收！\n");
            }else{
                printf("输入错误！\n");
                return 0;
            }
        }
       box->file_number=0;
    }
    return 0;      
}

int check_gro_fri(int fd)
{
    int i=0;

    printf("要查看的群账号：\n");
    scanf("%s",send_bag->read_buf);
    getchar();
    
    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli,&mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("群好友列表：\n");
        for(i=0;i<list->fri_number;i++)
        {
            if(list->fri_line[i]==2)
            {
                printf("群主:     id:%d     name:%s\n",list->fri_id[i],list->fri_name[i]);
            }else if(list->fri_line[i]==1){
                printf("管理员：   id:%d     name:%s\n",list->fri_id[i],list->fri_name[i]);
            }else if(list->fri_line[i]==0){
                printf("普通成员： id:%d     name:%s\n",list->fri_id[i],list->fri_name[i]);
            }
        }
    }else if(strcmp(send_bag->write_buf,"check")==0){
        printf("该群暂无聊天记录！或该群暂未成立！\n");
    }else{
        printf("查询失败！\n");
    } 
    return 0;
}

int check_info(int fd)
{
    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli,&mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        printf("id:%s         name:%s\n",send_bag->send_id,send_bag->send_name);
    }else{
        printf("显示错误！\n");
    }
    return 0;
}

int check_group(int fd)
{
    int i;
    send(fd,send_bag,sizeof(bag),0);

    pthread_mutex_lock(&mutex_cli);
    pthread_cond_wait(&cond_cli,&mutex_cli);
    pthread_mutex_unlock(&mutex_cli);

    if(strcmp(send_bag->write_buf,"success")==0)
    {
        for(i=0;i<lis->number;i++)
        {
            if(lis->chmod[i]==2)
            {
                printf("你拥有的群：   id:%d\n",lis->id[i]);
            }else if(lis->chmod[i]==1)
            {
                printf("你管理的群：   id:%d\n",lis->id[i]);
            }else if(lis->chmod[i]==0)
            {
                printf("你加入的群：   id:%d\n",lis->id[i]);
            }
        }
    }else{
        printf("暂未加入群！\n");
    }
    return 0;
}

void *thread_send(void *arg)
{
    int fd=*(int *)arg;
    int ret;
    int t;
    int choose;
    send_bag=(bag *)malloc(sizeof(bag));
    send_bag->send_fd=fd;
    while(1){
        menu();
        scanf("%d",&choose);
        getchar();
        switch (choose){
            case 1://登陆
            {
                send_bag->type=1;
                login(fd);
                break;
            }
            case 2://注册
            {
                send_bag->type=2;
                resign(fd);
                break;
            }
            case 0://退出
            {
                send_bag->type=0;
                ret=send(fd,send_bag,sizeof(bag),0);
                if(ret<0){
                    perror("send failed");
                    exit(1);
                }
                printf("即将退出！\n");
                pthread_exit(0);
            }
            default:
            {
                printf("输入错误！重新输入！\n");
                break;
            }
        }

        if(choose<0||choose>2)
        {
            continue;
        }else if(choose==1){               
            if(strcmp(send_bag->write_buf,"login successfully")==0)
            {
                printf("%s欢迎登陆~\n",send_bag->send_name);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                break;
            }else if(strcmp(send_bag->write_buf,"login failed")==0)
            {   
                printf("登陆失败！请重新登陆！\n");
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                continue;
            }else{
                printf("对比出错！请返回！\n");
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                continue;
            }
        }else if(choose==2){
            printf("按任意键继续！\n");
            getchar();
            system("clear");
            continue;
        }
    }
    
    while(1)
    {
        menu1();
        scanf("%d",&choose);
        getchar();
        switch(choose){
            case 1://私聊
            {
                send_bag->type=3;
                chat_single(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                
                break;
            }
            case 2://加好友
            {
                send_bag->type=4;
                add_fri(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
    
                break;
            }
            case 3://删好友
            {
                send_bag->type=5;
                del_fri(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                
                break;
            }
            case 4://显示好友列表
            {
                send_bag->type=6;
                display_fri(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                
                break;
            }
            case 5://查看聊天记录
            {
                send_bag->type=7;
                check_fri(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
            
                break;
            }
            case 6://拉黑好友
            {
                send_bag->type=8;
                black_fri(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
        
                break;
            }
            case 7://移出好友
            {
                send_bag->type=9;
                white_fri(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
        
                break;
            }
            case 8://开始群聊
            {
                send_bag->type=10;
                chat_double(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                break;
            }
            case 9://创建群聊
            {
                send_bag->type=11;
                create_gro(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                
                break;
            }
            case 10://解散群聊
            {
                send_bag->type=12;
                dis_gro(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                
                break;
            }
            case 11://申请加群
            {
                send_bag->type=13;
                add_gro(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
            
                break;
            }
            case 12://退出群聊
            {
                send_bag->type=14;
                exit_gro(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                break;
            }
            case 13://查看群聊天记录
            {
                send_bag->type=15;
                check_gro(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
            
                break;
            }
            case 14://管理群事务
            {
                send_bag->type=16;
                deal_gro(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                break;
            } 
            case 0://退出
            {
                send_bag->type=0;
                send(fd,send_bag,sizeof(bag),0);
                pthread_exit(0);
                return 0;
            }
            case 15://修改信息
            {
                send_bag->type=17;
                xiugai(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
    
                break;
            }
            case 16://处理消息
            {
                send_bag->type=18;
                deal(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                
                break;
            }
            case 17://发送文件
            {
                send_bag->type=21;
                send_file(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
        
                break;
            }   
            case 18://接收文件
            {
                send_bag->type=23;
                recv_file(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                
                break;
            }
            case 19://查看群好友列表
            {
                send_bag->type=24;
                check_gro_fri(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                break;
            }
            case 20://查看个人信息
            {
                send_bag->type=25;
                check_info(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                break;
            }
            case 21://查看群列表
            {
                send_bag->type=26;
                check_group(fd);
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                break;
            }
            default:
            {
                send_bag->type1=30;
                printf("输入错误！重新输入！\n");
                printf("按任意键继续！\n");
                getchar();
                system("clear");
                break;
            }
        }
    }
}

void *recv_box(void *arg)
{
    int fd=*(int *)arg;
    recv(fd,box,sizeof(BOX),MSG_WAITALL);
    pthread_exit(0);
}

void *recv_fri(void *arg)
{
    int fd=*(int *)arg;
    recv(fd,list,sizeof(fri),MSG_WAITALL);
    pthread_exit(0);
}

void *recv_frimes(void *arg)
{
    int fd=*(int *)arg;
    if(strcmp(recv_bag->send_id,send_bag->recv_id)==0)
    {
        printf("\033[34mid:%s     %s\n\033[0m",recv_bag->send_id,recv_bag->read_buf);
    }else{
        printf("来了一条消息！\n");
        box->send_id[box->talk_number]=atoi(recv_bag->send_id);
        strcpy(box->read_buf[box->talk_number],recv_bag->read_buf);
        box->talk_number++;
    }
    pthread_exit(0);
}

void *display_frimes(void *arg)
{
    int fd=*(int *)arg;
    recv(fd,mes,sizeof(fri_mes),MSG_WAITALL);
    pthread_exit(0);
}

void *recv_gromes(void *arg)
{
    int fd=*(int *)arg;
    if(strcmp(recv_bag->write_buf,send_bag->write_buf)==0)
    {
        ;
    }else if(strcmp(recv_bag->read_buf,send_bag->read_buf)==0)
    {
        printf("\033[34mid:%s      %s\n\033[0m",recv_bag->send_id,recv_bag->write_buf);
    }else{
        printf("来了一条群消息！\n");
        box->send_id1[box->number]=atoi(recv_bag->send_id);
        box->group_id[box->number]=atoi(recv_bag->read_buf);
        strcpy(box->message[box->number],recv_bag->write_buf);
        box->number++;
    }
    pthread_exit(0);
}

void *display_gromes(void *arg)
{
    int fd=*(int *)arg;
    recv(fd,message,sizeof(gro_mes),MSG_WAITALL);
    pthread_exit(0);
}

void *recv_group(void *arg)
{
    int fd=*(int *)arg;
    recv(fd,lis,sizeof(group),MSG_WAITALL);
    pthread_exit(0);
}

void *thread_recv(void *arg)
{
    int ret;
    int fd=*(int *)arg;

    recv_bag=(bag *)malloc(sizeof(bag));
    box=(BOX *)malloc(sizeof(BOX));
    list=(fri *)malloc(sizeof(fri));
    mes=(fri_mes *)malloc(sizeof(fri_mes));
    message=(gro_mes *)malloc(sizeof(gro_mes));
    lis=(group *)malloc(sizeof(group));

    while(1)
    {
        memset(recv_bag,0,sizeof(bag));
        ret=recv(fd,recv_bag,sizeof(bag),MSG_WAITALL);
    
        if(ret<0)
        {
            perror("recv failed");
            exit(1);
        }
        
        switch(recv_bag->type)
        {
            case 0://退出
            {
                printf("退出成功！\n");
                pthread_exit(0);
                return 0;
            }
            case 1://登陆
            {
                strcpy(send_bag->send_name, recv_bag->send_name);
                memset(send_bag->write_buf, 0, sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf, recv_bag->write_buf);
                send_bag->send_fd = recv_bag->recv_fd;

                if(strcmp(send_bag->write_buf,"login successfully")==0)
                {
                pthread_create(&tid,NULL,recv_box,(void *)&fd);
                pthread_join(tid,NULL);
                printf("离线时好友信息数：%d\n",box->talk_number);
                printf("离线时好友请求数：%d\n",box->friend_number);
                printf("离线时群聊信息数：%d\n",box->number);
                 printf("离线时群聊请求数：%d\n",box->gro_number);
                }
                pthread_mutex_lock(&mutex_cli);
                sing=1;
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                
                break;
            }
            case 2://注册
            {
                strcpy(send_bag->send_id,recv_bag->send_id);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 3://私聊
            {
                strcpy(send_bag->write_buf,recv_bag->write_buf);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 19://在线时私聊收消息
            {
                pthread_create(&tid,NULL,recv_frimes,(void *)&fd);
                pthread_join(tid,NULL);
                break;
            }
            case 4://加好友
            {
                strcpy(send_bag->write_buf,recv_bag->write_buf);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 5://删好友
            {
                strcpy(send_bag->write_buf,recv_bag->write_buf);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 6://显示好友
            {
                memset(send_bag->write_buf, 0, sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf, recv_bag->write_buf);
                memset(list,0,sizeof(fri));

                pthread_create(&tid,NULL,recv_fri,(void *)&fd);
                pthread_join(tid,NULL);
                
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 7://查看聊天记录
            {
                pthread_create(&tid,NULL,display_frimes,(void *)&fd);
                pthread_join(tid,NULL);

                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 8://拉黑好友
            {
                strcpy(send_bag->write_buf,recv_bag->write_buf);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 9://移出好友
            {
                strcpy(send_bag->write_buf,recv_bag->write_buf);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 10://开始群聊
            {
                strcpy(send_bag->buf,recv_bag->buf);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 20://收群聊消息
            {
                pthread_create(&tid,NULL,recv_gromes,(void *)&fd);
                pthread_join(tid,NULL);
                break;
            }
            case 11://创建群聊
            {
                memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf,recv_bag->write_buf);
                strcpy(send_bag->recv_id,recv_bag->recv_id);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 12://解散群聊
            {
                memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf,recv_bag->write_buf);
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 13://加入群聊
            {
                memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf,recv_bag->write_buf);

                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 14://退出群聊
            {
                memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf,recv_bag->write_buf);

                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 15://查看群聊天记录
            {
                pthread_create(&tid,NULL,display_gromes,(void *)&fd);
                pthread_join(tid,NULL);

                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 16://管理群事务
            {
                memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf,recv_bag->write_buf);

                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 17://修改
            {
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 18://处理消息
            {
                if(recv_bag->type1==1)
                {
                    pthread_mutex_lock(&mutex_cli);
                    box->plz_id[box->friend_number]=atoi(recv_bag->send_id);
                    box->friend_number++;
                    printf("有一条好友申请！\n");
                    pthread_mutex_unlock(&mutex_cli);
                }else if(recv_bag->type1==4)
                {
                    pthread_mutex_lock(&mutex_cli);
                    strcpy(send_bag->read_buf,recv_bag->read_buf);
                    box->gro_id[box->gro_number]=atoi(recv_bag->send_id);
                    box->gro_id1[box->gro_number]=atoi(recv_bag->read_buf);
                    box->gro_number++;
                    printf("有一条加群申请！\n");
                    pthread_mutex_unlock(&mutex_cli);
                }
                break;
            }
            case 21://传送文件
            {
                strcpy(send_bag->buf,recv_bag->buf);
                 
                pthread_mutex_lock(&mutex_cli);
                sing=1;
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 22://收到文件
            {
                strcpy(send_bag->recv_id,recv_bag->send_id);
                strcpy(send_bag->read_buf,recv_bag->read_buf);
                send_bag->size=recv_bag->size;
                send_bag->t=recv_bag->t;

                printf("%s给你发送了一个文件%s，快去接收吧！\n",send_bag->recv_id,send_bag->read_buf);
                box->file_id[box->file_number]=atoi(send_bag->recv_id);
                strcpy(box->file_name[box->file_number],send_bag->read_buf);
                box->file_number++;

                pthread_mutex_lock(&mutex_cli);
                sing=1;
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 23://接收文件
            {
                char file[100];

                memset(send_bag->buf,0,sizeof(send_bag->buf));
                memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));

                strcpy(send_bag->read_buf,recv_bag->read_buf);
               // strcpy(file,"1");
                //strcat(file,recv_bag->read_buf);
                strcpy(file,recv_bag->read_buf);
                strcpy(send_bag->buf,recv_bag->buf);

                send_bag->size=recv_bag->size;
                
                int pd=open(file, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IXUSR);
                write(pd,recv_bag->write_buf,send_bag->size);
                close(pd);

                pthread_mutex_lock(&mutex_cli);
                sing=1;
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 24://查看群好友列表
            {
                memset(send_bag->write_buf, 0, sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf, recv_bag->write_buf);
                memset(list,0,sizeof(fri));

                pthread_create(&tid,NULL,recv_fri,(void *)&fd);
                pthread_join(tid,NULL);
                
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 25://查看个人信息
            {
                memset(send_bag->write_buf,0,sizeof(send_bag->write_buf));
                memset(send_bag->send_name,0,sizeof(send_bag->send_name));
                strcpy(send_bag->send_name,recv_bag->send_name);
                strcpy(send_bag->write_buf,recv_bag->write_buf);

                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
            case 26://查看群列表
            {
                memset(send_bag->write_buf, 0, sizeof(send_bag->write_buf));
                strcpy(send_bag->write_buf, recv_bag->write_buf);
                memset(lis,0,sizeof(group));

                pthread_create(&tid,NULL,recv_group,(void *)&fd);
                pthread_join(tid,NULL);
                
                pthread_mutex_lock(&mutex_cli);
                pthread_cond_signal(&cond_cli);
                pthread_mutex_unlock(&mutex_cli);
                break;
            }
        }
    }
}
/*
void *thread_heart(void *arg)
{
    int fd=*(int *)arg;
    while(1)
    {
        send(fd,"exit",5,0);
        sleep(3);
    }
     pthread_exit(0);
}
*/
int main()
{
    struct sockaddr_in client_addr;
    int sockfd=socket_init(client_addr);
    if(sockfd<0) return 0;
    pthread_t tid1,tid2,tid3;
    
    sing=0;
    pthread_mutex_init(&mutex_cli,NULL);
    pthread_cond_init(&cond_cli,NULL);

    pthread_create(&tid1,NULL,thread_send,(void *)&sockfd);
    pthread_create(&tid2,NULL,thread_recv,(void *)&sockfd);
  //  pthread_create(&tid3,NULL,thread_heart,(void *)&sockfd);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
  //  pthread_join(tid3,NULL);
   
    return 0;
}