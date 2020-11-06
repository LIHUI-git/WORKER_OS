#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <strings.h>
#include <string.h>
#include <sqlite3.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>                                                             
#include <arpa/inet.h>

#define N 32

#define R 1 //user - regidter
#define L 2 //user - login
#define Q 3 //user - quit

#define SER_PORT 8888
#define SER_IP "0.0.0.0"

#define DATABASE "my.db"




//定义通信双方的信息结构体
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


void sig_child_handle(int signo);
int do_client(int acceptfd,sqlite3 *db,struct sockaddr_in cin);
void do_register(int acceptfd,Msg *msg,sqlite3 *db);
int do_login(int acceptfd ,Msg *msg,sqlite3 *db);
int do_info(int acceptfd,INFO *info,sqlite3 *db);

void do_add(int acceptfd,INFO *info,sqlite3 *db);
void do_delet(int acceptfd,INFO *info,sqlite3 *db);
void do_change(int acceptfd,INFO *info,sqlite3 *db);
void do_selet_print(int acceptfd,INFO *info,sqlite3 *db);
int callback(void *arg,int f_num,char **f_value,char **f_name);
void do_print(int acceptfd,INFO *info,sqlite3 *db);

void do_send_info(int acceptfd,INFO *info);

void sig_child_handle(int signo)
{
	if(SIGCHLD == signo)
		while(waitpid(-1,NULL,WNOHANG) > 0);
}




int main(int argc,const char *argv[])
{
	int sockfd;
	//	int n;
	//	Msg msg;
	sqlite3 *db;
	int acceptfd;
	pid_t pid;
	char *errmsg;

	/*	if (argc != 3)
		{
		printf("Usage:%s serverip port.\n",argv[0]);
		}
		*/
	//打开/创建数据库
	if (sqlite3_open(DATABASE,&db) != SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));
		return -1;
	}else{
		printf("open DATABASE success.\n");
	}
	//创建管理员登录表
	if (sqlite3_exec(db,"create table if not exists user(WorkNum text primary key,pass text)",
				NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
	}else
		printf("create or open user table success.\n");
	//创建普通员工信息表
	if (sqlite3_exec(db,"create table if not exists info(jobnum char,name char,age char,salary char,phnum char,addrs char);",
				NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf ("%s\n",errmsg);
	}else
		printf("create or open info success.\n");
	// 网络设置

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}
	//地址复用
	int reuse = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
	//填充
	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family 		= AF_INET;
	serveraddr.sin_port 		= htons(SER_PORT);
	serveraddr.sin_addr.s_addr 	= INADDR_ANY;//inet_addr(IP);
	puts("结构体填充成功！");
	socklen_t serlen = sizeof(serveraddr);


	if (bind(sockfd,(struct sockaddr *)&serveraddr,serlen) < 0)
	{
		printf("fail to bind.\n");
		return -1;
	}

	if (listen(sockfd,5) < 0)
	{
		printf("fail to listen.\n");
		return -1;
	}

	struct sockaddr_in cin;
	bzero(&cin,sizeof(cin));
	socklen_t addrlen = sizeof(cin);

	signal(SIGCHLD,sig_child_handle);

	while(1)
	{
		if ((acceptfd = accept(sockfd,(struct sockaddr *)&cin,&addrlen)) < 0)
		{
			perror("fail to accept");
			return -1;
		}
		if ((pid = fork()) < 0)
		{
			perror("fail to fork");
			return -1;
		}else if(pid == 0)
		{
			close(sockfd);
			do_client(acceptfd,db,cin);
		}else
			close(acceptfd);
	}
	close(sockfd);

	return 0;
}

//1 登录注册函数
int do_client(int acceptfd,sqlite3 *db,struct sockaddr_in cin)
{
	Msg msg;
	char Cli_IP[21] = {};
	while (recv(acceptfd,&msg,sizeof(msg),0) > 0)
	{
		printf("type:%d\n",msg.type);
		switch(msg.type)
		{
		case R:
			do_register(acceptfd,&msg,db);
			break;
		case L:
			do_login(acceptfd,&msg,db);
			break;

		default:
			printf("Invalid data msg.\n");

		}
	}

	if (inet_ntop(AF_INET,&cin.sin_addr.s_addr,Cli_IP,20) == NULL)
	{
		perror ("inet_ntop!");
		//	continue;
	}

	printf("IP:%s-client exit.\n",Cli_IP);
	close(acceptfd);
	exit(0);
	return 0;
}

//2 注册

void do_register(int acceptfd,Msg *msg,sqlite3 *db)
{
	char *errmsg;
	char sql[500];
	int ret = -1;
	sprintf(sql,"insert into user values('%s','%s');",msg->WorkNum,msg->data);
	printf("%s\n",sql);

	if (sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		strcpy(msg->data,"user WorkNum already exist.");
	}else
	{
		printf("client regidter ok!\n");
		strcpy(msg->data,"Register OK");
	}

	do{
		ret = send(acceptfd,msg,sizeof(Msg),0);
	}while(ret < 0 && EINTR == errno);

	if (ret < 0)
	{
		perror("fail to send");

	}
	return;
}
// 3登录
int do_login(int acceptfd,Msg *msg,sqlite3 *db)
{
	char sql[500] = {};
	char *errmsg;
	int nrow;
	int ncloumn;
	char **resultp;
	INFO info;

	sprintf(sql,"select * from user where WorkNum = '%s' and pass = '%s';",msg->WorkNum,msg->data);
	printf("%s\n",sql);
	if (sqlite3_get_table(db,sql,&resultp,&nrow,&ncloumn,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}else
	{
		printf("get_table ok!\n");
	}
	if (nrow == 1)
	{
		strcpy(msg->data,"OK");
		send(acceptfd,msg,sizeof(Msg),0);
		//	return 1;
		//	相关信息内容操作
		do_info(acceptfd,&info,db);
		

	}
	if (nrow == 0)
	{
		strcpy(msg->data,"usr/passwd wrong.");
		send(acceptfd,msg,sizeof(Msg),0);
	}
	return 0;
}
//员工信息
//增删改查操作*
int do_info(int acceptfd,INFO *info,sqlite3 *db)
{

	// 1 add, 2 delet, 3 change, 4 select_rint, 5 print 6 quit
	while (recv(acceptfd,info,sizeof(INFO),0) >0)
	{
		printf("recv client success\n");
		printf("n = %d\n",info->type);
		switch(info->type)
		{
		case 1:
			do_add(acceptfd,info,db);
			break;
		case 2:
			do_delet(acceptfd,info,db);
			break;
		case 3:
			do_change(acceptfd,info,db);
			break;
		case 4:
			do_selet_print(acceptfd,info,db);
			break;
		case 5:
			do_print(acceptfd,info,db);
			break;
		case 6:
			printf("********quit**********\n");
			bzero(info,sizeof(INFO));
			sprintf(info->errdata,"quit work OS success!");
			send(acceptfd,info,sizeof(INFO),0);
			return 0;
		default:
			printf("input error!\n");
		}

	}
	return 0;
}


void do_add(int acceptfd,INFO *info,sqlite3 *db)
{
	char sql[500] = {};
	char *errmsg;
	sprintf(sql,"insert into info values('%d','%s','%s','%s','%s','%s');",
			info->jobnum,info->name,info->age,info->salary,
			info->phnum,info->addrs);
	if (sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		strcpy(info->errdata,"fail add info.");
	}else
		strcpy(info->errdata,"add info success!");

	if (send(acceptfd,info,sizeof(INFO),0) < 0)
	{
		printf("fail send to client\n");
		exit(1);
	}

}


void do_delet(int acceptfd,INFO *info,sqlite3 *db)
{
	char sql[500] = {};
	char *errmsg;
//	printf("info->jobnum = %d\n",info->jobnum);

	sprintf(sql,"delete from info where jobnum='%d';",info->jobnum);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		sprintf(info->errdata,"fail delet");
	}else
	{
		bzero(info,sizeof(INFO));
		sprintf(info->errdata,"delete success!");


		if(send(acceptfd,info,sizeof(INFO),0) < 0)
		{
			printf("fail send delet msg\n");
		}else
			printf("send success\n");
	}

	return ;


}

void do_change(int acceptfd,INFO *info,sqlite3 *db)
{
	char sql[500] = {};
	char *errmsg;

	sprintf(sql,"update info set '%s'='%s' where jobnum='%d';",info->data,
			info->errdata,info->jobnum);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		bzero(info,sizeof(INFO));
		sprintf(info->errdata,"fail change msg!");
	}
	else
	{
		bzero(info,sizeof(INFO));
		sprintf(info->errdata,"change msg success!");
	}

	if (send(acceptfd,info,sizeof(INFO),0) < 0)
		printf("fail send\n");
	else
		printf("send success\n");


}


void do_selet_print(int acceptfd,INFO *info,sqlite3 *db)
{

	char sql[200] = {};
	char *errmsg;

	printf("select * from info where jobnum='%d';\n",info->jobnum);

	sprintf(sql,"select * from info where jobnum='%d';",info->jobnum);

	//查询员工数据
	if (sqlite3_exec(db,sql,callback,(void*)&acceptfd,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		sprintf(info->errdata,"worker not exist");
		send(acceptfd,info,sizeof(INFO),0);
	}
	else
		printf("query info success\n");

	return;
}
//打印数据表
int callback(void *arg,int f_num,char **f_value,char **f_name)
{
	int i = -1;
	int j = -1;
	INFO info;
	int acceptfd;
	acceptfd = *((int *)arg);
	bzero(&info,sizeof(INFO));
	for (i=0;i<f_num;i++)
	{
		printf("%-10s",f_name[i]);
		strcat(info.data,"\t|");
		strcat(info.data,f_name[i]);
	}
	puts("");
	for (j=0;j<f_num;j++)
	{
		printf("%-10s",f_value[j]);
		strcat(info.errdata,"\t|");
		strcat(info.errdata,f_value[j]);

	}
	puts("");

	if(send(acceptfd,&info,sizeof(INFO),0) < 0)
	{
		printf("fail send select\n");
		return 0;
	}
	else
	{
		bzero(&info,sizeof(INFO));
		printf("send select success\n");
	}

	return 0;
}

void do_print(int acceptfd,INFO *info,sqlite3 *db)
{
	char *errmsg;

	char **resultp = NULL;
	int nrow = -1;
	int ncolumn =-1;
	int i = -1;
	int j = -1;
	int index = 0;
	if(sqlite3_get_table(db,"select * from info",&resultp,
				&nrow,&ncolumn,&errmsg) != SQLITE_OK)
	{
		printf("fail select \n");
		exit(1);
	}

	bzero(info,sizeof(INFO));
	for(j=0;j<ncolumn;j++)
	{
		printf("%-10s",resultp[index]);
		strcat(info->errdata,"\t|");
		strcat(info->errdata,resultp[index]);
		index++;
	}
	puts("");
	do_send_info(acceptfd,info);
	
	
	for(i=1;i<=nrow;i++)
	{
		bzero(info,sizeof(INFO));
		for(j=0;j<ncolumn;j++)
		{
			printf("%-10s",resultp[index]);
			strcat(info->errdata,"\t|");
			strcat(info->errdata,resultp[index]);
			index++;
		}
	
		puts("");
		do_send_info(acceptfd,info);
	}
	sprintf(info->errdata,"OK");
	do_send_info(acceptfd,info);
}

void do_send_info(int acceptfd,INFO *info)
{

	if(send(acceptfd,info,sizeof(INFO),0) < 0)
		printf("fail send select\n");
	else
	{
		printf("send select success\n");
		bzero(info,sizeof(INFO));
	}

}

