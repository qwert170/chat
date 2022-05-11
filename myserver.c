#include <sys/types.h>                                                   
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <mysql.h>
#include <stdlib.h>
#include"my_chat.h"
#include <sys/epoll.h>
#include <termios.h>
#include<time.h>
#include <sys/stat.h>
#include <fcntl.h> 

MYSQL mysql;

int sock_init()
{
    int sock_fd;
    struct sockaddr_in serv_addr;

    sock_fd=socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);
    if(sock_fd<0)
    {
        perror("socket failed");
        return -1;
    }

    int optval=1;
    if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(void *)&optval,sizeof(int))<0)
    {
        perror("setsockopt failed");
        return -1;
    }

    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(SERV_PORT);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(sock_fd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr_in))<0)
    {
        perror("bind failed");
        return -1;
    }

    if(listen(sock_fd,LISTENG)<0)
    {
        perror("listen failed");
        return -1;
    }
    printf("服务端监听中...\n");
    return sock_fd;
}

void mysql_init1()
{   //句柄
    if(NULL == mysql_init(&mysql)){
		perror("mysql_init failed");
        return;
	}

	//初始化数据库
	if(mysql_library_init(0, NULL, NULL) != 0){
        perror("mysql_library_init failed");
        return;
	}

	//连接数据库
	if(NULL == mysql_real_connect(&mysql, "127.0.0.1", "root", "hh123456789", "try", 0, NULL, 0)){
        perror("mysql_real_connect failed");
        return;
	}

	//设置中文字符集
	if(mysql_set_character_set(&mysql, "utf8") < 0){
        perror("mysql_set_character_set failed");
        return;
	}
	
	printf("连接mysql数据库成功!\n");
}

int login(bag *recv_bag)
{
    printf("检查信息中...\n");
    bag *bag1=recv_bag;
    int ret;
    char sql[1280]; 
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;

    memset(sql,0,1280);
    sprintf(sql,"select * from 身份信息 where id = '%s' and passwd ='%s' and line = 0",bag1->send_id,bag1->read_buf);
    ret=mysql_query(&mysql,sql);
   
    if(!ret)
    {
        result=mysql_store_result(&mysql);
        row=mysql_fetch_row(result);
        if(row)
        {
            memset(sql,0,1280);
            sprintf(sql,"update 身份信息 set line = 1 , sockfd = %d where id = '%s'",bag1->recv_fd,bag1->send_id);
            ret=mysql_query(&mysql,sql);

            memset(bag1->write_buf,0,1024);
            strcpy(bag1->write_buf,"login successfully");
            strcpy(bag1->send_name,row[1]);

            if(ret<0)
            {
                perror("login failed");
                exit(1);
            } 
            
            ret=send(bag1->recv_fd,bag1,sizeof(bag),0);
            if(ret<0)
            {
                perror("send failed");
                exit(1);
            }
            printf("login successfully\n");
            mysql_free_result(result);
            
            return 1;
        }else{
            strcpy(bag1->write_buf,"login failed");
            send(bag1->recv_fd,bag1,sizeof(bag),0);
            printf("login failed\n");
        }
    }else{
        strcpy(bag1->write_buf,"login failed");
        send(bag1->recv_fd,bag1,sizeof(bag),0);
        printf("login failed\n");
    }
    return 0;
}

int resign(bag *recv_bag)
{
     bag *bag1=recv_bag;
    char sql[1280];
    int j=100;
    int ret;
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;

    memset(sql,0,1280);

    sprintf(sql,"select * from 身份信息");
    ret=mysql_query(&mysql,sql);
    if(ret<0)
    {
        perror("mysql_query failed");
        return -1;
    }

    result=mysql_store_result(&mysql);
    while(row=mysql_fetch_row(result))
    {
        j++;
    }
    sprintf(bag1->send_id,"%d",j);
    bag1->line=0;
    
    pthread_mutex_lock(&mutex);
    memset(sql,0,1280);
    sprintf(sql,"insert into 身份信息 values('%s','%s','%s',%d,0)",bag1->send_id,bag1->send_name,bag1->read_buf,bag1->line);
    mysql_query(&mysql,sql);
    ret=send(bag1->recv_fd,bag1,sizeof(bag),0);
    if(ret<0)
    {
        perror("send failed");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    printf("注册成功！\n");
    pthread_mutex_unlock(&mutex);
    return 0;
}

int add_fri(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row,row1;
    int t;
    int ret;
    BOX *box=box_head;
    memset(sql,0,1280);
    sprintf(sql,"select * from 身份信息 where id = '%s'",bag1->recv_id);
    ret=mysql_query(&mysql,sql);
            
    if(!ret){
        result=mysql_store_result(&mysql);
        row=mysql_fetch_row(result);
        if(row==NULL)
        {
            strcpy(bag1->write_buf,"-2");
             send(bag1->recv_fd,bag1,sizeof(bag),0);
        }else{

            memset(sql,0,1280);
            sprintf(sql,"select * from friend where id1 = '%s' and id2='%s' ",bag1->send_id,bag1->recv_id);

            ret=mysql_query(&mysql,sql);
            if(!ret){
                result=mysql_store_result(&mysql);
                row1=mysql_fetch_row(result);
                if(row1){
                    t=atoi(row1[2]);
                    if(t==1){
                        strcpy(bag1->write_buf,"1");
                    }else if(t==-1)
                    {
                        strcpy(bag1->write_buf,"-1");                        
                    }else if(t==0){
                        strcpy(bag1->write_buf,"3");
                    }else{
                        strcpy(bag1->write_buf,"4");
                    }
                    send(bag1->recv_fd,bag1,sizeof(bag),0);
                    return 0;
                }else{ 
                    printf("正在发送请求！\n");
                   
                    pthread_mutex_lock(&mutex);
                        
                    memset(sql,0,1280);
                    sprintf(sql,"insert into friend values('%s','%s',0)",bag1->send_id,bag1->recv_id);
                    ret=mysql_query(&mysql,sql);
                    if(ret<0)
                    {
                        perror("failed");
                        exit(1);
                    }

                    //strcpy(bag1->write_buf,"0");
                    pthread_mutex_unlock(&mutex);
                    //send(bag1->recv_fd,bag1,sizeof(bag),0);
                    if(atoi(row[3])==0){
                        while(box!=NULL)
                        {
                            if(box->recv_id==atoi(bag1->recv_id))
                            {
                                break;
                            }
                            box=box->next;
                        }

                        if(box==NULL)
                        {
                            box=(BOX *)malloc(sizeof(BOX));
                            box->friend_number=0;
                            box->number=0;
                            box->talk_number=0;
                            box->file_number=0;
                            box->next=NULL;
                            box->recv_id=atoi(bag1->recv_id);
                            box->plz_id[box->friend_number]=atoi(bag1->send_id);
                            box->friend_number++;
                            if(box_head==NULL)
                            {
                                box_head=box;
                                box_tail=box;
                            }else{
                                box_tail->next=box;
                                box_tail=box;
                            }
                        }else{
                            box->plz_id[box->friend_number]=atoi(bag1->send_id);
                            box->friend_number++;
                        }
                       strcpy(bag1->write_buf,"0");
                       send(bag1->recv_fd,bag1,sizeof(bag),0);
                    }else{
                        bag1->send_fd=atoi(row[4]);
                        strcpy(bag1->write_buf,"0");
                        send(bag1->recv_fd,bag1,sizeof(bag),0);
                        bag1->type=18;
                        bag1->type1=1;
                        send(bag1->send_fd,bag1,sizeof(bag),0);
                    }
                }       
            }else{
                strcpy(bag1->write_buf,"4");
                send(bag1->recv_fd,bag1,sizeof(bag),0);
            }
        }
    }else{
        strcpy(bag1->write_buf,"4");
         send(bag1->recv_fd,bag1,sizeof(bag),0);
    }
    return 0;
}

int xiugai(bag *recv_bag)
{
     bag *bag1=recv_bag;
    int t=bag1->type1;
    char sql[1280];
    int ret;
    pthread_mutex_lock(&mutex);
    if(t==1){
        memset(sql,0,1280);
        sprintf(sql,"update 身份信息 set name = '%s' where id = '%s'",bag1->read_buf,bag1->send_id);
        mysql_query(&mysql,sql);
        printf("修改中...\n");
    }else if(t==2){
        memset(sql,0,1280);
        sprintf(sql,"update 身份信息 set passwd = '%s' where id = '%s'",bag1->read_buf,bag1->send_id);
        mysql_query(&mysql,sql);
        printf("修改中...\n");
    }
    pthread_mutex_unlock(&mutex);
}

int deal(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;
    switch(bag1->type1)
    {
        case 1:
        {
            if(strcmp(bag1->read_buf,"tongyi")==0)
            {
                pthread_mutex_lock(&mutex);
                memset(sql,0,1280);
                sprintf(sql,"update friend set relation = 1 where id1 = '%s' and id2 = '%s'",bag1->recv_id,bag1->send_id);
                mysql_query(&mysql,sql);

                memset(sql,0,1280);
                sprintf(sql,"insert into friend values('%s','%s',1)",bag1->send_id,bag1->recv_id);
                mysql_query(&mysql,sql);
                pthread_mutex_unlock(&mutex);
            }else if(strcmp(bag1->read_buf,"jujue")==0){
                memset(sql,0,1280);
                sprintf(sql," delete from friend where relation = 0 and id1 ='%s' and id2 = '%s' ",bag1->recv_id,bag1->send_id);
                mysql_query(&mysql,sql);
            }
            break;
        }
        case 4:
        {
            if(strcmp(bag1->read_buf,"tongyi")==0)
            {
                memset(sql,0,sizeof(sql));
                sprintf(sql,"update gro set chmod =0 where gro_id =%d and gro_member_id=%d",atoi(bag1->write_buf),atoi(bag1->recv_id));
                mysql_query(&mysql,sql);
            }else if(strcmp(bag1->read_buf,"jujue")==0){
                memset(sql,0,sizeof(sql));
                sprintf(sql,"delete  from gro where chmod =-1 and gro_id =%d and gro_member_id=%d",atoi(bag1->write_buf),atoi(bag1->recv_id));
                mysql_query(&mysql,sql);
            }
            break;
        }
    }
}

int del_fri(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    char sql1[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;
    memset(sql,0,1280);
    sprintf(sql,"select * from friend where id1 ='%s' and id2 = '%s'",bag1->send_id,bag1->recv_id);
    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);

    if(row)
    {
        pthread_mutex_lock(&mutex);
        memset(sql,0,1280);
        sprintf(sql,"delete from friend where id1 = '%s' and id2 ='%s'",bag1->send_id,bag1->recv_id);
        mysql_query(&mysql,sql);
        memset(sql1,0,1280);
        sprintf(sql1,"delete from friend where id1 = '%s' and id2 ='%s'",bag1->recv_id,bag1->send_id);
        mysql_query(&mysql,sql1);
        pthread_mutex_unlock(&mutex);
        memset(bag1->write_buf,0,sizeof(bag1->write_buf));

        strcpy(bag1->write_buf,"delete success");
    }else{
        memset(bag1->write_buf,0,sizeof(bag1->write_buf));
        strcpy(bag1->write_buf,"no exist");
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    return 0;
}

int display_fri(bag *recv_bag)
{
    bag *bag1=recv_bag;
    fri *list;
    list=(fri *)malloc(sizeof(fri));
    char sql[1280];
    char sql1[1280];
    MYSQL_RES *result=NULL;
    MYSQL_RES *result1=NULL;
    MYSQL_ROW row,row1;
    int ret;
   

    list->fri_number=0;

    pthread_mutex_lock(&mutex);
    memset(sql,0,1280);
    sprintf(sql,"select * from friend where id1 ='%s' and relation =1",bag1->send_id);
    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);

    while(row=mysql_fetch_row(result))
    {
        list->fri_id[list->fri_number]=atoi(row[1]);
        
        memset(sql1,0,1280);
        sprintf(sql1,"select * from  身份信息 where id = '%s'",row[1]);
        mysql_query(&mysql,sql1);
        result1=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result1);

        list->fri_line[list->fri_number]=atoi(row1[3]);
        strcpy(list->fri_name[list->fri_number],row1[1]);

        list->fri_number++;
    }
    pthread_mutex_unlock(&mutex);
    memset(bag1->write_buf,0,sizeof(bag1->write_buf));

    if(list->fri_number==0)
    {
        strcpy(bag1->write_buf,"no exist");
    }else{
        strcpy(bag1->write_buf,"success");
    }

    send(bag1->recv_fd,bag1,sizeof(bag),0);
    send(bag1->recv_fd,list,sizeof(fri),0);
    return 0;
}

int black_fri(bag *recv_bag)
{
    bag *bag1;
    bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result;
    MYSQL_ROW row;

    pthread_mutex_lock(&mutex);
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select *from friend where id1 ='%s' and id2='%s' and relation =1 ",bag1->send_id,bag1->recv_id);
    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        memset(bag1->write_buf,0,sizeof(bag1->write_buf));
        strcpy(bag1->write_buf,"no exist");
    }else{
        memset(sql,0,sizeof(sql));
        sprintf(sql,"update friend set relation = -1 where id1 = '%s' and id2 ='%s' ",bag1->send_id,bag1->recv_id);
        mysql_query(&mysql,sql);

        memset(sql,0,sizeof(sql));
        sprintf(sql,"update friend set relation = -1 where id1 = '%s' and id2 ='%s' ",bag1->recv_id,bag1->send_id);
        mysql_query(&mysql,sql);
        strcpy(bag1->write_buf,"success");
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    pthread_mutex_unlock(&mutex);
    return 0;
}

int white_fri(bag *recv_bag)
{
    bag *bag1;
    bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result;
    MYSQL_ROW row;

    pthread_mutex_lock(&mutex);
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select *from friend where id1 ='%s' and id2='%s' and relation = -1 ",bag1->send_id,bag1->recv_id);
    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        memset(bag1->write_buf,0,sizeof(bag1->write_buf));
        strcpy(bag1->write_buf,"failed");
    }else{
        memset(sql,0,sizeof(sql));
        sprintf(sql,"update friend set relation = 1 where id1 = '%s' and id2 ='%s' ",bag1->send_id,bag1->recv_id);
        mysql_query(&mysql,sql);

        memset(sql,0,sizeof(sql));
        sprintf(sql,"update friend set relation = 1 where id1 = '%s' and id2 ='%s' ",bag1->recv_id,bag1->send_id);
        mysql_query(&mysql,sql);
        strcpy(bag1->write_buf,"success");
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    pthread_mutex_unlock(&mutex);
    return 0;
}

int chat_single(bag *recv_bag)
{
    bag *bag1;
    bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row,row1;
    BOX *box;
    time_t t;
    char shijian[100];

    memset(sql,0,1280);
    sprintf(sql,"select * from 身份信息 where id = '%s' ",bag1->recv_id);
    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);

    if(row==NULL)
    {
        strcpy(bag1->write_buf,"failed");
        send(bag1->recv_fd,bag1,sizeof(bag),0);
        return 0;
    }else{
        memset(sql,0,1280);
        sprintf(sql,"select * from friend where id1 ='%s' and id2 ='%s' ",bag1->send_id,bag1->recv_id);
        mysql_query(&mysql,sql);
        result=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result);
        if(row1!=NULL)
        {
            if(atoi(row1[2])==-1)//黑名单
            {
                strcpy(bag1->write_buf,"black");
                send(bag1->send_fd,bag1,sizeof(bag),0);
                return 0;   
            }else{
                bag1->send_fd=atoi(row[4]);
                pthread_mutex_lock(&mutex);
                strcpy(bag1->write_buf,"success");
                send(bag1->recv_fd,bag1,sizeof(bag),0);
                pthread_mutex_unlock(&mutex);

                if(atoi(row[3])==1){
                    bag1->type=19;
                    send(bag1->send_fd,bag1,sizeof(bag),0);//给收消息的那端发消息
                }else{
                    box=box_head;
                    while(box!=NULL)
                    {
                        if(box->recv_id==atoi(bag1->recv_id))
                        {
                            break;
                        }
                        box=box->next;
                    }

                    if(box==NULL)
                    {
                        box=(BOX *)malloc(sizeof(BOX));
                        box->friend_number=0;
                        box->recv_id=atoi(bag1->recv_id);
                        box->talk_number=0;
                        box->number=0;
                        box->file_number=0;
                        box->next=NULL;
                        box->send_id[box->talk_number]=atoi(bag1->send_id);
                        strcpy(box->read_buf[box->talk_number],bag1->read_buf);
                        box->talk_number++;
                        if(box_head==NULL)
                        {
                            box_head=box;
                            box_tail=box;
                        }else{
                            box_tail->next=box;
                            box_tail=box;
                        }
                    }else{
                        box->send_id[box->talk_number]=atoi(bag1->send_id);
                        strcpy(box->read_buf[box->talk_number],bag1->read_buf);
                        box->talk_number++;
                    }
                }
                memset(sql,0,sizeof(sql));
                time(&t);
                ctime_r(&t,shijian);
                sprintf(sql,"insert into friend_mes values('%s','%s','%s','%s')",bag1->send_id,bag1->recv_id,bag1->read_buf,shijian);
                mysql_query(&mysql,sql);
            }
        }else{
            strcpy(bag1->write_buf,"failed");
            send(bag1->send_fd,bag1,sizeof(bag),0);
            return 0;
        }
    }
}

int mes_fri(bag * recv_bag)
{
    bag * bag1=recv_bag;
    fri_mes *message;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;
    message=(fri_mes *)malloc(sizeof(fri_mes));

    message->buf_num=0;
   
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select *from friend_mes where ( id1 ='%s' and id2 ='%s') or (id1 ='%s' and id2 ='%s')",bag1->send_id,bag1->recv_id,bag1->recv_id,bag1->send_id);

    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);
    while(row=mysql_fetch_row(result))
    {
        message->id[message->buf_num]=atoi(row[0]);
        message->fri_id[message->buf_num]=atoi(row[1]);
        strcpy(message->buf[message->buf_num],row[2]);
        strcpy(message->time[message->buf_num],row[3]);
        message->buf_num++;
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    send(bag1->recv_fd,message,sizeof(fri_mes),0);
    return 0;
}

int create_gro(bag *recv_bag)
{
    bag *bag1=recv_bag;
    int j=999;
    MYSQL_RES *result=NULL;
    MYSQL_ROW row,row1;
    char sql[1280];
    int ret;
    gro *group;

    group=(gro *)malloc(sizeof(gro));
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select * from gro");
    pthread_mutex_lock(&mutex);
    ret=mysql_query(&mysql,sql);
    if(!ret)
    {
        result=mysql_store_result(&mysql);
        while(row=mysql_fetch_row(result))
        {
            if(j<atoi(row[0]))
            {
                j=atoi(row[0]);
            }
        }
        j++;
        group->gro_id=j;

        strcpy(bag1->write_buf,"success");
        sprintf(bag1->recv_id,"%d",group->gro_id);

        memset(sql,0,sizeof(sql));
        sprintf(sql,"select * from 身份信息 where id ='%s'",bag1->send_id);
        mysql_query(&mysql,sql);
        result=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result);
        
        strcpy(group->gro_name,row1[1]);
        group->gro_mem_id=atoi(bag1->send_id);

        memset(sql,0,sizeof(sql));
        sprintf(sql,"insert into gro values (%d, %d,'%s',2) ",group->gro_id,group->gro_mem_id,group->gro_name);
        mysql_query(&mysql,sql);
    }else{
        strcpy(bag1->write_buf,"failed");
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    pthread_mutex_unlock(&mutex);
    return 0;
    
}

int dis_gro(bag *recv_bag)
{
    bag * bag1=recv_bag;
    char sql[1280];
    int ret;
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;
    memset(sql,0,sizeof(sql));
    pthread_mutex_lock(&mutex);
    sprintf(sql,"select *from gro where gro_id = %d and gro_member_id = %d ",atoi(bag1->read_buf),atoi(bag1->send_id));
    ret=mysql_query(&mysql,sql);
    if(!ret){
        result=mysql_store_result(&mysql);
        row=mysql_fetch_row(result);
        if(row==NULL)
        {
            strcpy(bag1->write_buf,"failed");
        }else{
            if(atoi(row[3])==2)
            {
                strcpy(bag1->write_buf,"success");
                memset(sql,0,sizeof(sql));
                sprintf(sql,"delete from gro where gro_id = %d ",atoi(bag1->read_buf));
                mysql_query(&mysql,sql);
            }else{
                strcpy(bag1->write_buf,"no rights");
            }
        }
    }else{
        strcpy(bag1->write_buf,"failed");
    }

    send(bag1->recv_fd,bag1,sizeof(bag),0);
    pthread_mutex_unlock(&mutex);
    return 0;
}

int add_gro(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row,row1,row2,row3,row4;
    int ret;
    BOX *box;

    box=box_head;
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select * from gro where gro_id ='%d'", atoi(bag1->read_buf));
    ret=mysql_query(&mysql,sql);
    if(!ret){
        result=mysql_store_result(&mysql);
        row=mysql_fetch_row(result);
        if(row!=NULL)
        {   
            pthread_mutex_lock(&mutex);

            memset(sql,0,sizeof(sql));
            sprintf(sql,"select *from gro where gro_id = %d and gro_member_id = %d",atoi(bag1->read_buf),atoi(bag1->send_id));
            mysql_query(&mysql,sql);
            result=mysql_store_result(&mysql);
            row2=mysql_fetch_row(result);

            if(row2!=NULL){
                if(atoi(row2[3])==-1)
                {
                    strcpy(bag1->write_buf,"sended");
                    send(bag1->recv_fd,bag1,sizeof(bag),0);
                    pthread_mutex_unlock(&mutex);
                }else{
                    strcpy(bag1->write_buf,"ed");
                    send(bag1->recv_fd,bag1,sizeof(bag),0);
                    pthread_mutex_unlock(&mutex);
                    }
            }else{
                memset(sql,0,sizeof(sql));
                sprintf(sql,"select * from gro where gro_id =%d and chmod =2",atoi(bag1->read_buf));
                mysql_query(&mysql,sql);
                result=mysql_store_result(&mysql);
                row1=mysql_fetch_row(result);
                if(row1==NULL)
                {
                    strcpy(bag1->write_buf,"sended");
                    send(bag1->recv_fd,bag1,sizeof(bag),0);
                    pthread_mutex_unlock(&mutex);
                    return 0;
                }else{
                    strcpy(bag1->recv_id,row[1]);
                    memset(sql,0,sizeof(sql));
                    sprintf(sql,"select * from 身份信息 where id ='%s'",row1[1]);
                    mysql_query(&mysql,sql);

                    result=mysql_store_result(&mysql);
                    row3=mysql_fetch_row(result);

                    memset(sql,0,sizeof(sql));
                    sprintf(sql,"select *from 身份信息 where id = '%s'",bag1->send_id);
                    mysql_query(&mysql,sql);
                    result=mysql_store_result(&mysql);
                    row4=mysql_fetch_row(result);
                    strcpy(bag1->send_name,row4[1]);

                    memset(sql,0,sizeof(sql));
                    sprintf(sql,"insert into gro values(%d,%d,'%s',-1)",atoi(bag1->read_buf),atoi(bag1->send_id),bag1->send_name);
                    mysql_query(&mysql,sql);

                    if(atoi(row3[3])==0)
                    {
                        while(box!=NULL)
                        {
                            if(box->recv_id==atoi(bag1->recv_id))
                            {
                                break;
                            }
                            box=box->next;
                        }

                        if(box==NULL)
                        {
                            box=(BOX *)malloc(sizeof(BOX));
                            box->gro_number=0;
                            box->friend_number=0;
                            box->number=0;
                            box->talk_number=0;
                            box->file_number=0;
                            box->next=NULL;
                            box->recv_id=atoi(bag1->recv_id);
                            box->gro_id[box->gro_number]=atoi(bag1->send_id);
                            box->gro_id1[box->gro_number]=atoi(bag1->read_buf);
                            box->gro_number++;
                            if(box_head==NULL)
                            {
                                box_head=box;
                                box_tail=box;
                            }else{
                                box_tail->next=box;
                                box_tail=box;
                            }
                        }else{
                            box->gro_id[box->gro_number]=atoi(bag1->send_id);
                            box->gro_number++;
                        }
                        strcpy(bag1->write_buf,"sending");
                        send(bag1->recv_fd,bag1,sizeof(bag),0);
                        pthread_mutex_unlock(&mutex);
                    }else{
                        bag1->send_fd=atoi(row3[4]);
                        strcpy(bag1->write_buf,"sending");
                        send(bag1->recv_fd,bag1,sizeof(bag),0);
                    
                        bag1->type=18;
                        bag1->type1=4;
                        send(bag1->send_fd,bag1,sizeof(bag),0);
                        pthread_mutex_unlock(&mutex);
                    }
                }
            }
        }else{
            strcpy(bag1->write_buf,"failed");
            send(bag1->recv_fd,bag1,sizeof(bag),0);
        }
    }else{
        strcpy(bag1->write_buf,"failed");
        send(bag1->recv_fd,bag1,sizeof(bag),0);
    }
    
    return 0;
}

int exit_gro(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;
    int ret;

    memset(sql,0,sizeof(sql));

    sprintf(sql,"select *from gro where gro_id = %d and gro_member_id= %d ",atoi(bag1->read_buf),atoi(bag1->send_id));
    ret=mysql_query(&mysql,sql);
    if(!ret)
    {
        result=mysql_store_result(&mysql);
        row=mysql_fetch_row(result);
        if(row!=NULL)
        {
            if(atoi(row[3])!=2){
                memset(sql,0,sizeof(sql));
                sprintf(sql,"delete from gro where gro_id = %d and gro_member_id= %d",atoi(bag1->read_buf),atoi(bag1->send_id));
                mysql_query(&mysql,sql);
                
            }else{
                memset(sql,0,sizeof(sql));
                sprintf(sql,"delete from  gro where gro_id = %d",atoi(bag1->read_buf));
                mysql_query(&mysql,sql);
            }
            strcpy(bag1->write_buf,"success");
        }else{
            strcpy(bag1->write_buf,"no find");
        }
    }else{
        strcpy(bag1->write_buf,"failed");
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    return 0;
}

int check_mes(bag *recv_bag)
{
    bag *bag1=recv_bag;
    gro_mes *message;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;

    message=(gro_mes *)malloc(sizeof(gro_mes));
    message->number=0;
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select * from gro_mes where gro_id =%d",atoi(bag1->read_buf));
    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);
    while(row=mysql_fetch_row(result))
    {
        message->gro_id=atoi(bag1->read_buf);
        strcpy(message->buf[message->number],row[2]);
        message->id[message->number]=atoi(row[1]);
        strcpy(message->time[message->number],row[3]);
        message->number++;
    }
    send(recv_bag->recv_fd,bag1,sizeof(bag),0);
    send(recv_bag->recv_fd,message,sizeof(gro_mes),0);
    return 0;
}

int deal_gro(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    int ret;
    MYSQL_RES *result=NULL;
    MYSQL_ROW row,row1;

    memset(sql,0,sizeof(sql));
    sprintf(sql,"select * from gro where gro_id = %d and chmod =2 ",atoi(bag1->read_buf));//找群主
    ret=mysql_query(&mysql,sql);
    if(!ret)
    {
        result=mysql_store_result(&mysql);
        row=mysql_fetch_row(result);
        if(row!=NULL)
        {
            memset(sql,0,sizeof(sql));
            sprintf(sql,"select * from gro where gro_id =%d and gro_member_id =%d",atoi(bag1->read_buf),atoi(bag1->recv_id));//找人
            mysql_query(&mysql,sql);
            result=mysql_store_result(&mysql);
            row1=mysql_fetch_row(result);

            if(row1!=NULL){
                if(strcmp(row[1],bag1->send_id)==0){
                    switch(bag1->type1)
                    {
                        case 1:
                        {   
                            if(atoi(row1[3])==1)
                            {
                                strcpy(bag1->write_buf,"dealed");
                            }else{
                                memset(sql,0,sizeof(sql));
                                sprintf(sql,"update gro set chmod=1 where gro_id=%d and gro_member_id =%d",atoi(bag1->read_buf),atoi(bag1->recv_id)); 
                                mysql_query(&mysql,sql);
                                strcpy(bag1->write_buf,"success");
                            }                                
                            break;
                        }
                        case 2:
                        {   
                            if(atoi(row1[3])==1)
                            {
                                memset(sql,0,sizeof(sql));
                                sprintf(sql,"update gro set chmod=0 where gro_id=%d and gro_member_id =%d",atoi(bag1->read_buf),atoi(bag1->recv_id));
                                mysql_query(&mysql,sql);
                                strcpy(bag1->write_buf,"success");
                            }else{
                                strcpy(bag1->write_buf,"no admin");
                            }
                            break;
                        }
                        case 3:
                        {
                            if(atoi(row1[3])!=2)
                            {
                                memset(sql,0,sizeof(sql));
                                sprintf(sql,"delete from gro where gro_id =%d and gro_member_id =%d ",atoi(bag1->read_buf),atoi(bag1->recv_id));
                                mysql_query(&mysql,sql);
                                strcpy(bag1->write_buf,"success");
                            }else{
                                strcpy(bag1->write_buf,"no possible");
                            }
                            break;
                        }
                    }
                }else{
                    strcpy(bag1->write_buf,"no chmod");
                }
            }else{
                strcpy(bag1->write_buf,"no find id");
            }
        }else{
            strcpy(bag1->write_buf,"no find gro");
        }
    }else{
        strcpy(bag1->write_buf,"failed");
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    return 0;
}

int chat_double(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    char sql1[1280];
    int ret;
    MYSQL_RES *result=NULL;
    MYSQL_RES *result1=NULL;
    MYSQL_ROW row,row1,row2;
    BOX *box;
    char shijian[100];
    time_t t;

    
    box=box_head;
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select * from gro where gro_id = %d and gro_member_id = %d",atoi(bag1->read_buf),atoi(bag1->send_id));
    ret=mysql_query(&mysql,sql);
    if(!ret)
    {
        result=mysql_store_result(&mysql);
        row=mysql_fetch_row(result);
        if(row!=NULL)
        {
            memset(sql,0,sizeof(sql));
            sprintf(sql,"select * from gro where gro_id = %d",atoi(bag1->read_buf));
            mysql_query(&mysql,sql);
            result=mysql_store_result(&mysql);

            while(row1=mysql_fetch_row(result))
            {
                if(strcmp(row1[1],bag1->send_id)==0){
                    continue;
                }
                    strcpy(bag1->recv_id,row1[1]);
                    
                    memset(sql1,0,sizeof(sql));
                    sprintf(sql1,"select *from 身份信息 where id = '%s'",bag1->recv_id);
                    mysql_query(&mysql,sql1);
                    result1=mysql_store_result(&mysql);
                    row2=mysql_fetch_row(result1);
                    strcpy(bag1->buf,"success");
                    bag1->send_fd=atoi(row2[4]);
                    send(bag1->recv_fd,bag1,sizeof(bag),0);
                    
                    if(atoi(row2[3])==1)//在线
                    {
                        bag1->type=20;
                        send(bag1->send_fd,bag1,sizeof(bag),0);
                    }else{
                        while(box!=NULL)
                        {
                            if(box->recv_id==atoi(bag1->recv_id))
                            {
                                break;
                            }
                            box=box->next;
                        }
                        
                        if(box==NULL)
                        {
                            box=(BOX *)malloc(sizeof(BOX));
                            box->recv_id=atoi(bag1->recv_id);
                            box->friend_number=0;
                            box->number=0;
                            box->talk_number=0;
                            box->file_number=0;
                            box->next=NULL;
                            box->send_id1[box->number]=atoi(bag1->send_id);
                            box->group_id[box->number]=atoi(bag1->read_buf);
                            strcpy(box->message[box->number],bag1->write_buf);
                            box->number++;
                            if(box_head==NULL)
                            {
                                box_head=box;
                                box_tail=box;
                            }else{
                                box_tail->next=box;
                                box_tail=box;
                            }
                        }else{
                            box->send_id1[box->number]=atoi(bag1->send_id);
                            box->group_id[box->number]=atoi(bag1->read_buf);
                            strcpy(box->message[box->number],bag1->write_buf);
                            box->number++;
                        }
                    }
            }   
            memset(sql,0,sizeof(sql));
            time(&t);
            ctime_r(&t,shijian);
            sprintf(sql,"insert into gro_mes values(%d,%d,'%s','%s')",atoi(bag1->read_buf),atoi(bag1->send_id),bag1->write_buf,shijian);
            mysql_query(&mysql,sql);    
        }else{
            strcpy(bag1->buf,"no find");
            send(bag1->recv_fd,bag1,sizeof(bag),0);
        }
    }else{
        strcpy(bag1->buf,"failed");
        send(bag1->recv_fd,bag1,sizeof(bag),0);
    }
    return 0;
}

int send_file(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char file[100];
    strcpy(file,bag1->read_buf);
    int fd=open(file, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IXUSR);
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;
    int ret;
    int len;

    if(strcmp(bag1->buf,"ok")==0)
    {
        memset(sql,0,sizeof(sql));
        sprintf(sql,"select * from 身份信息 where id= '%s'",bag1->recv_id);
        ret=mysql_query(&mysql,sql);
        if(!ret){
            result=mysql_store_result(&mysql);
            row=mysql_fetch_row(result);
            if(row!=NULL){
                if(atoi(row[3])==0)
                {
                    memset(bag1->buf,0,sizeof(bag1->buf));
                    strcpy(bag1->buf,"no online");
                    send(bag1->recv_fd,bag1,sizeof(bag),0);
                }else{
                    memset(bag1->buf,0,sizeof(bag1->buf));
                    strcpy(bag1->buf,"success");

                    send(bag1->recv_fd,bag1,sizeof(bag),0);
                    bag1->send_fd=atoi(row[4]);
                    bag1->type=22;
                    send(bag1->send_fd,bag1,sizeof(bag),0);
                }
            }else{
                memset(bag1->buf,0,sizeof(bag1->buf));
                strcpy(bag1->buf,"no find");
                send(bag1->recv_fd,bag1,sizeof(bag),0);
            }
        }
    }else{
        write(fd,bag1->write_buf,bag1->size);
        close(fd);
        send(bag1->recv_fd,bag1,sizeof(bag),0);
    }
    return 0;
}

int recv_file(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char file[100];
    strcpy(file,bag1->read_buf);
    int fd=open(file,O_RDONLY);
    int len;

    memset(bag1->write_buf,0,sizeof(bag1->write_buf));
    memset(bag1->buf,0,sizeof(bag1->buf));
    lseek(fd,(-1)*bag1->t,SEEK_END);
   
    while((len=read(fd,bag1->write_buf,1023))>0)
    {
        bag1->size=len;
        send(bag1->recv_fd,bag1,sizeof(bag),0);
    }
    close(fd);
    return 0;
}

int check_fri_fro(bag * recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;
    int ret;
    fri *list;

    list=(fri *)malloc(sizeof(fri));
    list->fri_number=0;
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select* from gro where gro_id= %d order by chmod desc",atoi(bag1->read_buf));
    ret=mysql_query(&mysql,sql);

    if(!ret)
    {
        result=mysql_store_result(&mysql);
        while(row=mysql_fetch_row(result))
        {
            list->fri_id[list->fri_number]=atoi(row[1]);
            strcpy(list->fri_name[list->fri_number],row[2]);
            list->fri_line[list->fri_number]=atoi(row[3]);
            list->fri_number++;
        }
        if(list->fri_number==0)
        {
            strcpy(bag1->write_buf,"check");
        }else{
            strcpy(bag1->write_buf,"success");
        }
    }else{
        strcpy(bag1->write_buf,"failed");
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    send(bag1->recv_fd,list,sizeof(fri),0);
    return 0;
}

int check_info(bag *recv_bag)
{
    bag *bag1=recv_bag;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;

    memset(sql,0,sizeof(sql));
    sprintf(sql,"select * from 身份信息 where id =%s",bag1->send_id);
    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);

    if(row==NULL)
    {
        strcpy(bag1->write_buf,"failed");
    }else{
        strcpy(bag1->write_buf,"success");
        strcpy(bag1->send_name,row[1]);
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    return 0;
}

int check_group(bag *recv_bag)
{
    bag *bag1=recv_bag;
    group *lis;
    char sql[1280];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;

    lis=(group *)malloc(sizeof(group));
    lis->number=0;
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select * from gro where gro_member_id =%d order by chmod desc",atoi(bag1->send_id));
    mysql_query(&mysql,sql);
    result=mysql_store_result(&mysql);

    while(row=mysql_fetch_row(result))
    {
        lis->id[lis->number]=atoi(row[0]);
        lis->chmod[lis->number]=atoi(row[3]);
        lis->number++;
    }
    if(lis->number!=0){
        strcpy(bag1->write_buf,"success");
    }else{
        strcpy(bag1->write_buf,"failed");
    }
    send(bag1->recv_fd,bag1,sizeof(bag),0);
    send(bag1->recv_fd,lis,sizeof(group),0);
    return 0;
}

void *func(void *bag1)
{
    pthread_detach(pthread_self());
    BOX *box=box_head;
    bag *recv_bag;
    recv_bag=(bag *)bag1;
    
    int t;
    int ret;
   

    switch(recv_bag->type)
    {   
        case 1://登陆
        {
            t=login(recv_bag);
            if(t==1)
            {
                while(box!=NULL)
                {
                    if(box->recv_id==atoi(recv_bag->send_id)){
                        break;
                    }
                    box=box->next;
                }
                if(box!=NULL)
                {
                    send(recv_bag->recv_fd,box,sizeof(BOX),0);
                    box->friend_number=0;
                    box->talk_number=0;
                    box->number=0;
                    box->gro_number=0;
                }else{
                    box=(BOX *)malloc(sizeof(BOX));
                    box->recv_id=atoi(recv_bag->send_id);
                    box->friend_number=box->talk_number=0;
                    box->number=0;
                    box->gro_number=0;
                    box->next=NULL;
                    if(box_head==NULL)
                    {
                        box_head=box;
                        box_tail=box;
                    }else{
                        box_tail->next=box;
                        box_tail=box;
                    }
                    send(recv_bag->recv_fd,box,sizeof(BOX),0);
                }
            }
            break;
        }
        case 2://注册
        {
            resign(recv_bag);
            break;
        }
        case 3://私聊
        {
            chat_single(recv_bag);
            break;
        }
        case 4://加好友
        {
            add_fri(recv_bag);
            break;
        }
        case 5://删好友
        {
            del_fri(recv_bag);
            break;
        }
        case 6://显示好友
        {
            display_fri(recv_bag);
            break;
        }
        case 7://聊天记录 
        {
            mes_fri(recv_bag);
            break;
        }
        case 8://拉黑好友
        {
            black_fri(recv_bag);
            break;
        }
        case 9://移出好友
        {
            white_fri(recv_bag);
            break;
        }
        case 10://开始群聊
        {
            chat_double(recv_bag);
            break;
        }
        case 11://创建群聊
        {
            create_gro(recv_bag);
            break;
        }
        case 12://解散群聊
        {
            dis_gro(recv_bag);
            break;
        }
        case 13://申请加群
        {
            add_gro(recv_bag);
            break;
        }
        case 14://退出群聊
        {
            exit_gro(recv_bag);
            break;
        }
        case 15://查看群聊天记录
        {
            check_mes(recv_bag);
            break;
        }
        case 16://管理群事务 
        {
            deal_gro(recv_bag);
            break;
        }
        case 17://修改信息
        {
            xiugai(recv_bag);
            break; 
        } 
        case 18://处理消息
        {
            deal(recv_bag);
            break;
        }
        case 21://传送文件
        {
            send_file(recv_bag);
            break;
        }
        case 23://接收文件
        {
            recv_file(recv_bag);
            break;
        }
        case 24://查看群好友
        {
            check_fri_fro(recv_bag);
            break;
        }
        case 25://查看个人信息
        {
            check_info(recv_bag);
            break;
        }
        case 26://查看群好友
        {
            check_group(recv_bag);
            break;
        }
    }
}
/*
void *thread_heart(void *arg)
{
    int ret;
    FD fd=*(FD *)arg;
    char id[5];
    char sql[1280];
    while(1)
    {
        ret=recv(fd.event_fd,id,sizeof(id),0);
        sleep(3);
        if(ret<0||ret==0)
        {
            printf("sockfd:%d 挂机！\n",fd.conn_fd);
            memset(sql,0,sizeof(sql));
            sprintf(sql,"update 身份信息 set line = 0 where sockfd = %d and line =1 ",fd.conn_fd);
            mysql_query(&mysql,sql);
            break;
        }
    }
     pthread_exit(0);
}
*/
void service(int sock_fd)
{
    printf("服务器启动...\n");
    bag recv_bag;
    bag *bag1;
    bag *try_bag;
    struct sockaddr_in client_addr;
    socklen_t len;
    struct epoll_event epfd;
    struct epoll_event ev[20];
    char choice[2];

    int conn_fd;
    int epollfd;
    int maxevents;
    int ready;
    int i;
    int ret;
    char sql[128];
    MYSQL_RES *result=NULL;
    MYSQL_ROW row;
    pthread_t tid1;

    
    len=sizeof(struct sockaddr_in);

    epollfd=epoll_create(1024);

    epfd.data.fd=sock_fd;
    epfd.events=EPOLLIN | EPOLLET;//读 et

    if((epoll_ctl(epollfd, EPOLL_CTL_ADD, sock_fd, &epfd))<0)
    {
        perror("epoll_ctl failed");
        return ;
    }

    maxevents=1;
    while(1)
    {
        if(maxevents==0)
        {
            maxevents++;
        }
        if((ready=epoll_wait(epollfd,ev,maxevents,-1))<=0)
        {
            perror("epoll_wait failed");
            continue;
        }

        for(i=0;i<ready;i++)
        {
            if(ev[i].data.fd==sock_fd)
            {
                if((conn_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &len))<0)
                {
                    perror("accept failed");
                    continue;
                }
                printf("连接成功，套接字为%d\n",conn_fd);

                epfd.data.fd=conn_fd;
                epfd.events=EPOLLIN | EPOLLET;

                if((epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_fd, &epfd))<0)
                {
                    perror("epoll_ctl failed");
                    continue;
                }
                maxevents++;
                //FD fd;
                //fd.conn_fd=conn_fd;
               // fd.event_fd=ev[i].data.fd;
               // pthread_create(&tid1,NULL,thread_heart,(void *)&fd);
                continue;
            }else if(ev[i].events & EPOLLIN)
            {
                memset(&recv_bag,0,sizeof(bag));
                ret=recv(ev[i].data.fd,&recv_bag,sizeof(bag),MSG_WAITALL);
                
                if(ret<0)
                {
                    close(ev[i].data.fd);
                    perror("recv failed");
                    continue;
                }

                if(recv_bag.type==0)
                {
                    send(ev[i].data.fd,&recv_bag,sizeof(bag),0);
                    memset(sql,0,128);
                    sprintf(sql,"update 身份信息 set line = 0 , sockfd = 0 where line =1 and id ='%s' ",recv_bag.send_id);
                    mysql_query(&mysql,sql);
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_fd, &epfd);
                    maxevents--;
                    continue;
                }
                
                pthread_t tid;
                recv_bag.recv_fd=ev[i].data.fd;
                bag1 = (bag*)malloc(sizeof(bag));
                memcpy(bag1, &recv_bag, sizeof(bag));
                pthread_create(&tid,NULL,func,(void *)bag1);
               // pthread_join(tid,NULL);
                //pthread_join(tid1,NULL);
            }
        }    
    }
}

void close_mysql()
{
    mysql_close(&mysql);
    mysql_library_end();
}

int main()
{
    int sock_fd=sock_init();
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
    mysql_init1();
    service(sock_fd);
    close_mysql();
} 
