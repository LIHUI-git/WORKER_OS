#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define   N  32

#define  R  1   // user - register
#define  L  2   // user - login
#define  Q  3   // user - query

#define PORT 8888
#define IP "0.0.0.0"

#define KEY "key"




typedef struct 
{
	int type;
	char WorkNum[N];
	char data[256];

}__attribute__((packed)) Msg;

typedef struct
{
	int type;
	int jobnum;
	char name[N];
	char age[N];
	char salary[N];
	char addrs[BUFSIZ];
	char phnum[N];
	char errdata[500];
	char data[200];
}__attribute__((packed)) INFO;

int do_register(int sockfd,Msg *msg);
int do_login(int sockfd,Msg *msg);
int do_login_select(int sockfd, Msg *msg);
void do_add(int sockfd,INFO *info,int *n);
void do_selet_print(int sockfd,INFO *info,int *n);
void do_delet(int sockfd,INFO *info,int *n);
void do_change(int sockfd,INFO *info,int *n);
void do_print(int sockfd,INFO *info,int *n);
void do_worker_change(int sockfd,INFO *info,int *n);
void do_show_worker(int sockfd,INFO *info);
void do_up_select_work(int sockfd,INFO *info);

int main(int argc, const char *argv[])
{
	int sockfd;
	int n;
	Msg msg;
	INFO info;
	//	char buf[BUFSIZ];

#if 0
	if (argc != 3)
	{
		printf ("Usage:%s serverip port.\n",argv[0]);
		return -1;
	}
#endif
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.");
		return -1;
	}

	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family 		= AF_INET;
	serveraddr.sin_port 		= htons(PORT);
	serveraddr.sin_addr.s_addr 	= inet_addr(IP);

	if (connect(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}

	while(1)
	{
		puts("/*********************************************************************/");
		puts(" 	1.register 		2.login 		3.quit 				");
		puts("/*********************************************************************/");
		printf("Please choose>>");
		if (1 != scanf("%d",&n))
		{
			puts("input error !");
			continue;
		}
		while(getchar() != '\n');

		switch(n)
		{
		case 1:
			do_register(sockfd,&msg);
			break;
		case 2:
			//登录区分 管理员 普通用户

			if(do_login(sockfd,&msg) == 1)
				do_up_select_work(sockfd,&info);
			else
				printf("fail login\n");
			
			break;
		case 3:
			close(sockfd);
			return 0;
			break;
		default:
			puts("warning input");
		}

	}

	return 0;
}

// 注册 
int do_register(int sockfd,Msg *msg)
{
	msg->type = R;
	printf("Input WorkNum>>");
	scanf("%s",msg->WorkNum);
	getchar();
	printf("Input passwd>>");
	scanf("%s",msg->data);
	getchar();

	if(send(sockfd,msg,sizeof(Msg),0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}
	if (recv(sockfd,msg,sizeof(Msg),0) < 0)
	{
		printf("fail to recv.\n");
		return -1;
	}

	printf("%s\n",msg->data);
	return 0;
}

//登录
int do_login(int sockfd,Msg *msg)
{
	msg->type = L;
	printf("Input WorkNum>>");
	scanf("%s",msg->WorkNum);
	getchar();

	printf("Input passwd>>");
	scanf("%s",msg->data);
	getchar();

	if(send(sockfd,msg,sizeof(Msg),0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}
	if(recv(sockfd,msg,sizeof(Msg),0) < 0)
	{
		printf("fail to recv.\n");
		return -1;
	}
	if (strncmp(msg->data,"OK",3) == 0)
	{
		printf("Login ok\n");
		return 1;
	}else
		printf("%s\n",msg->data);
	return 0;

}

int do_login_select(int sockfd, Msg *msg)
{
	int n = -1;
	INFO info;
	while(1)
	{
		printf("************************************************************\n");
		printf("* 1.add  2.delet  3.change  4.select_print  5.print  6.quit*\n");
		printf("*************************************************************\n");
		printf("Please choose:");
		if(1 != scanf("%d",&n)){
			puts("**printf*输入错误重新输入");
			continue;
		}
		while(getchar() != '\n'); 

		switch(n)
		{
		case 1:
			do_add(sockfd,&info,&n);
			break;
		case 2:
			do_delet(sockfd,&info,&n);
			break;
		case 3:
			do_change(sockfd,&info,&n);
			break;
		case 4:
			do_selet_print(sockfd,&info,&n);
			break;
		case 5:
			do_print(sockfd,&info,&n);
			break;
		case 6:
			return 0;
			break;
		default :
			printf("***输入非法.\n");
		}

	}
	return 0;
}


//实现功能行数
void do_add(int sockfd,INFO *info,int *n)
{
	info->type = *n;
	printf("n = %d\n",*n);
	printf("input jobnum>>");
	scanf("%d",&(info->jobnum));
	getchar();
	printf("input name>>");
	scanf("%s",info->name);
	getchar();
	printf("input age>>");
	scanf("%s",info->age);
	getchar();
	printf("input salary>>");
	scanf("%s",info->salary);
	getchar();
	printf("input phnum>>");
	scanf("%s",info->phnum);
	getchar();
	printf("input addrs>>");
	scanf("%s",info->addrs);
	getchar();


	if (send(sockfd,info,sizeof(INFO),0) < 0)
		printf("fail send to server!\n");
	else
	{
		if(recv(sockfd,info,sizeof(INFO),0) < 0)
			printf("fail recv server msg\n");
		else
			printf("%s\n",info->errdata);
		bzero(&info,sizeof(INFO));
	}
	return ;

}

void do_delet(int sockfd,INFO *info,int *n)
{
	bzero(info,sizeof(INFO));
	info->type = *n;
	printf("input delet jobnum>>");
	scanf("%d",&(info->jobnum));
	getchar();

	if (send(sockfd,info,sizeof(INFO),0) < 0)
		printf("fail delet send to server\n");
	else
	{
		printf("send success\n");
	}

	if (recv(sockfd,info,sizeof(INFO),0) < 0)
	{
		printf("fail recv delet func \n");
	}else
		printf("%s\n",info->errdata);

}

void do_change(int sockfd,INFO *info,int *n)
{
	info->type = *n;

	printf("input select jobnum>>");
	scanf("%d",&(info->jobnum));
	getchar();

	printf("input change type>>");
	scanf("%s",info->data);
	getchar();
	printf("input change result>>");
	scanf("%s",info->errdata);
	getchar();

	if(send(sockfd,info,sizeof(INFO),0) < 0)
		printf("fail send\n");
	else
		bzero(info,sizeof(INFO));


	if(recv(sockfd,info,sizeof(INFO),0) < 0)
	{
		printf("fail recv\n");
	}else
		printf("%s\n",info->errdata);
}

void do_worker_change(int sockfd,INFO *info,int *n)
{
	info->type = *n;

	printf("input select jobnum>>");
	scanf("%d",&(info->jobnum));
	getchar();

	printf("input change type>>");
	scanf("%s",info->data);
	getchar();
	if (strcmp(info->data,"addrs") == 0 || strcmp(info->data,"phnum") == 0)
	{
		printf("input change result>>");
		scanf("%s",info->errdata);
		getchar();

		if(send(sockfd,info,sizeof(INFO),0) < 0)
			printf("fail send\n");
		else
			bzero(info,sizeof(INFO));


		if(recv(sockfd,info,sizeof(INFO),0) < 0)
		{
			printf("fail recv\n");
		}else
			printf("%s\n",info->errdata);

	}else
	{
		printf("*---请重新输入----*\n");

	}

}


void do_selet_print(int sockfd,INFO *info,int *n)

{
	bzero(info,sizeof(INFO));
	info->type = *n;

	printf("input select jobnum>>");
	scanf("%d",&(info->jobnum));
	getchar();

	if (send(sockfd,info,sizeof(INFO),0) < 0)
		printf("fail print send to server\n");
	else
	{
		printf("send success\n");
		bzero(info,sizeof(INFO));
	}

	if(recv(sockfd,info,sizeof(INFO),0) > 0)
	{
		printf("%s\n",info->data);
		printf("%s\n",info->errdata);
	}else
	{
		printf("fail recv select_print\n");
	}
	return;
}


void do_print(int sockfd,INFO *info,int *n)
{

	info->type = *n;

	if(send(sockfd,info,sizeof(INFO),0) < 0)
	{
		printf("fail send\n");
	}else
		printf("send success\n");

	while(recv(sockfd,info,sizeof(INFO),0) > 0)
	{
		if (strcmp(info->errdata,"OK") != 0)
		{
			printf("%s\n",info->errdata);
		}else
			break;
	}

}

void do_show_worker(int sockfd,INFO *info)
{
	int n = 0;
	while (1)
	{
		puts("/*************************************************/");
		puts("/*	1.查询 	 2.修改 	 3.退出 	  */");
		puts("/*************************************************/");
		printf("input number select>>");
		scanf("%d",&n);
		getchar();
		switch(n)
		{
		case 1:
			n+=3;
			do_selet_print(sockfd,info,&n);
			break;
		case 2:
			n+=1; 
			puts("/*----------------------------------------------------*/");
			puts("/* 您只能修改您的电话(phnum)以及您的地址(addrs) */");
			puts("/*----------------------------------------------------*/");
			do_worker_change(sockfd,info,&n);
			break;
		case 3:
			return 0;
		default:
			printf("输入错误重新输入\n");
		}
	}
}



void do_up_select_work(int sockfd,INFO *info)
{
	int serial;
	Msg msg;
	char key[N];
	while(1)
	{
		puts("/***************************************************/");
		puts("/*******************用户阶层选择********************/");
		puts("/* 	9.管理员   0.普通用户  3.退出 */");
		puts("/***************************************************/");      
		printf("input serial number>>");
		scanf("%d",&serial);
		getchar();
		switch (serial)
		{
		case 9:
			printf("输入管理员秘钥>>");
			scanf("%s",key);
			getchar();
			if (strcmp(key,KEY) == 0)
			{
				do_login_select(sockfd,&msg);
			}else
				printf("-----重新选择------\n");
			break;
		case 0:
			do_show_worker(sockfd,info);
			break;
		case 3:
			bzero(info,sizeof(INFO));
			info->type = 6;
			if(send(sockfd,info,sizeof(INFO),0) < 0)
				printf("fail send quit msg\n");
			else
			{
				printf("send quit msg success\n");
			}

			if(recv(sockfd,info,sizeof(INFO),0) <0)
				printf("fail quit work OS!\n");
			else
				printf("%s\n",info->errdata);
			return 0;
		default:
			printf("---重新输入---\n");
		}
	}
}

