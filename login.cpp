#include <stdio.h>
#include <string.h>
#include "cgic.h"
#include "mysql/mysql.h"

void executeNonQuery(char *sql, char *host, char *user, char *pass, char *db )
{
  MYSQL *mysql = mysql_init(0);
  
  if (!mysql_real_connect(mysql, host, user, pass, db, 0, NULL, 0)) 
  {
    goto error;
  }
  else 
  {
    //printf("连接成功\n");
    if (mysql_query(mysql, sql))
    {
      goto error;
    }
  }
  goto exit;
  error:
    printf("执行失败, %s\n", mysql_error(mysql));
  exit:
    mysql_close(mysql);
}

MYSQL_RES *executeQuery(char *sql, char *host, char *user, char *pass, char *db )
{
  MYSQL *mysql = mysql_init(0);
  
  if (!mysql_real_connect(mysql, host, user, pass, db, 0, NULL, 0)) 
  {
    goto error;
  }
  else 
  {
    //printf("连接成功\n");
    if (mysql_query(mysql, sql))
    {
      goto error;
    }
    else
    {
      MYSQL_RES *result = mysql_store_result(mysql);
      mysql_close(mysql);
      return result;
    }
  }
  error:
    printf("执行失败, %s\n", mysql_error(mysql));
}

void printError(char* msg,char* code,char *password)
{
	fprintf(cgiOut,"<html><head><title>%s</title></head><body>", msg);
	fprintf(cgiOut,"<form action='login.cgi' method='post'>");
	fprintf(cgiOut," 学号：<input type='text' name='code' value='%s'/></br>",code);
	fprintf(cgiOut,"密码：<input type='password' name='password' value='%s'/></br>",password);
	fprintf(cgiOut,"<input type='submit' name='btnLogin' value='登录'/>");
	fprintf(cgiOut,"<font color='red'>%s</font>",msg);
	fprintf(cgiOut,"</form>");
	fprintf(cgiOut,"</body></html>");
}

void import_data(char *filename, char *host, char *user, char *pass, char *db )
{
    char sql[4096]={0};

    executeNonQuery("truncate table T_Courses", host, user, pass, db);
    sprintf(sql, "load data infile '/tmp/%s' into table T_Courses fields terminated by '|' lines terminated by'\n'", filename);
    executeNonQuery(sql, host, user, pass, db);
}

int cgiMain()
{
  char *host = "localhost"; //ip地址
  char *user = "root";      //数据库学号
  char *pass = "";          //自己数据库密码
  char *db   = "study";     //表名

	char btnLogin[256]={0};

	if(cgiFormString("btnLogin",btnLogin,sizeof(btnLogin))==cgiFormSuccess)// cgiFormSubmitClicked();
	{
		char code[256]={0};
		char password[256]={0};
		//点击登录按钮，要登录
		cgiHeaderContentType("text/html;charset=utf-8");
		
		if(cgiFormString("code",code,sizeof(code))!=cgiFormSuccess)
		{
			printError("学号必须填写",code,password);
			return 0;
		}
		if(cgiFormString("password",password,sizeof(password))!=cgiFormSuccess)
		{
			printError("密码必须填写",code,password);
			return 0;
		}
    	MYSQL_RES *result = executeQuery("select code,password from T_Users", host, user, pass, db);
		MYSQL_ROW row;
		int flag = 0;

		while(row=mysql_fetch_row(result))
		{
			if(strcmp(code, row[0])==0 && strcmp(password, row[1])==0)
			{
				flag = 1;
			}
		}
		if (flag)
		{
			import_data("course_data.txt", host, user, pass, db);
			if (strcmp(code, "admin")==0)
			{
				fprintf(cgiOut,"<html><head><title>管理员界面</title></head><body>");
				fprintf(cgiOut,"课程管理：</br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=course&action=list'>查看课程列表</a></br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=course&action=init'>重置课程</a></br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=course&action=addnew'>添加课程</a></br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=course&action=find'>查找课程</a></br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=course&action=sort'>课程排序</a></br>");
				fprintf(cgiOut,"用户管理：</br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=student&action=list'>查看用户列表</a></br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=student&action=addnew'>添加用户</a></br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=student&action=find'>查找用户</a></br>");
				fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;<a href='admin_manage.cgi?mod=student&action=sort'>用户排序</a></br>");
				fprintf(cgiOut,"</body></html>");
			}
			else
			{
				fprintf(cgiOut,"<html><head><title>学生界面</title></head><body>");
				fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=list'>查看所有课程信息</a></br>",code);
				fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=find'>查找课程</a></br>",code);
				fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=sort'>课程排序</a></br>",code);
				fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=view'>浏览已选课程信息</a></br>",code);
				fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=select'>开始选课</a>",code);
				
				fprintf(cgiOut,"</body></html>");
			}
		}
		else
		{
			printError("学号或密码错误",code,password);
		}
	}
	else //进入登录页面
	{
		cgiHeaderContentType("text/html;charset=utf-8");
		fprintf(cgiOut,"<html><head><title>登录界面</title></head><body>");
		fprintf(cgiOut,"<form action='login.cgi' method='post'>");
		fprintf(cgiOut,"学号：<input type='text' name='code'/></br>");
		fprintf(cgiOut,"密码：<input type='password' name='password'/></br>");
		fprintf(cgiOut,"<input type='submit' name='btnLogin' value='登录'/>");
		fprintf(cgiOut,"</form>");
		fprintf(cgiOut,"</body></html>");
	}
	return 0;
}