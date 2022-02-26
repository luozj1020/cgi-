#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <malloc.h>
#include "cgic.h"
#include <mysql/mysql.h>

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

void show_error(char *msg)
{
    cgiHeaderContentType("text/html;charset=utf-8");
    fprintf(cgiOut,"<html><head><title>Error!</title></head><body>");
	fprintf(cgiOut,"<font color='red'>%s</font>",msg);
	fprintf(cgiOut,"</body></html>");
}

static int is_digit_str(char *str)
{
    return (strspn(str, "0123456789")==strlen(str));
}

int cmd(char* cmd, char* result)
{
    char buffer[10240];
    FILE* pipe = popen(cmd, "r");
    if (!pipe)
    return -1;
    while(!feof(pipe)) {
        if(fgets(buffer, 4096, pipe)){
            strcat(result, buffer);
        }
    }
    pclose(pipe);
    return 0;
}

void export_data(char *filename, char *table_name, char *host, char *user, char *pass, char *db )
{
    char sql[4096]={0};

    sprintf(sql, "SELECT * FROM %s INTO OUTFILE '/tmp/%s' FIELDS TERMINATED BY '|' LINES TERMINATED BY '\n'", table_name, filename);
    executeNonQuery(sql, host, user, pass, db);
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
  char code[20] = {0};
  char mod[20] = {0};
  char action[20] = {0};
  char *host = "localhost"; //ip地址
  char *user = "root";      //数据库用户名
  char *pass = "";          //自己数据库密码
  char *db   = "study";     //表名
  char day_week[7][10] = {"一", "二", "三", "四", "五", "六", "日"};
  int sum_major;
  time_t tmpcal_ptr;
  struct tm *tmp_ptr = NULL;
  int flag;
  int i;

  MYSQL_RES *result_ = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
  MYSQL_ROW row_;
  //统计主修课总门数
  while(row_=mysql_fetch_row(result_)){if(strcmp(row_[2],"主修")==0){sum_major+=1;}}
  if(cgiFormString("code", code, sizeof(code))!=cgiFormSuccess)
  {
      show_error("没有提供code参数");
      return 0;
  }
  if(cgiFormString("mod", mod, sizeof(mod))!=cgiFormSuccess)
  {
      show_error("没有提供mod参数");
      return 0;
  }
  if(cgiFormString("action", action, sizeof(action))!=cgiFormSuccess)
  {
      show_error("没有提供action参数");
      return 0;
  }
  MYSQL_RES *result1 = executeQuery("select code from T_Users", host, user, pass, db);
  MYSQL_ROW row1;
  while(row1=mysql_fetch_row(result1))
  {
    if(strcmp(code,row1[0])==0 && strcmp(code,"6")!=0){flag=1;}
  }
  mysql_free_result(result1);
  if(flag)
  {
    if(strcmp(mod, "course")==0)
    {
      if(strcmp(action, "list")==0)
      {
        char filename[1024]={0};

        cgiHeaderContentType("text/html;charset=utf-8");
        fprintf(cgiOut,"<font color='orange'>请选择你要进行的操作：</font>");
        fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=find'>查找课程</a>&nbsp;&nbsp;&nbsp;",code);
        fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=sort'>课程排序</a>&nbsp;&nbsp;&nbsp;",code);
        fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=view'>浏览已选课程</a>&nbsp;&nbsp;&nbsp;",code);
        fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=select'>开始选课</a>&nbsp;&nbsp;&nbsp;",code);
        fprintf(cgiOut, "<a href='login.cgi'>退出登录</a>");
        fprintf(cgiOut,"<html><head><title>课程列表</title></head><body>");
        fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>课程列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
        fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课老师1</td><td>任课老师2</td><td>任课老师3</td><td>任课老师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
        fprintf(cgiOut,"<tbody style='text-align:center'>");
        MYSQL_RES *result_user = executeQuery("select selected_course from T_Users", host, user, pass, db);
        MYSQL_ROW row_user;
        char select_courses[10240]={0};
        while(row_user=mysql_fetch_row(result_user))
        {
          strcat(select_courses,row_user[0]);
          strcat(select_courses,",");
        }
        MYSQL_RES *result = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
        MYSQL_ROW row;
        while(row=mysql_fetch_row(result))
        {
          int num=0;

          fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td>"\
              ,row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10]);
          //计算选课人数并修改记录
          char selected_list[50][20];
          int pos=0,count=0;
          for(i=0;i<=strlen(select_courses);i++)
          {
              if(select_courses[i]!=','){selected_list[count][pos]=select_courses[i];pos+=1;}
              else{pos=0;count+=1;}
          }
          for(i=0;i<count;i++)
          {
              if(strcmp(row[0],selected_list[i])==0){num+=1;}
          }
          fprintf(cgiOut,"<td>%d</td></tr>",num);
          char sql[1024]={0};
          sprintf(sql,"update T_Courses set num=%d where code='%s'",num,row[0]);
          executeNonQuery(sql, host, user, pass, db);
        }
        fprintf(cgiOut,"</tbody>");
        fprintf(cgiOut,"</table>");
        fprintf(cgiOut,"</body></html>");
        time(&tmpcal_ptr);tmp_ptr = gmtime(&tmpcal_ptr);tmp_ptr = localtime(&tmpcal_ptr);
        sprintf(filename,"course_data_%d%d%d-%d:%d:%d.txt",(1900+tmp_ptr->tm_year), (1+tmp_ptr->tm_mon), tmp_ptr->tm_mday, tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec);
        //export_data(filename, "T_Courses", host, user, pass, db);
        mysql_free_result(result);
      }
      else if (strcmp(action, "view")==0)
      {
        //浏览已选课程
        MYSQL_RES *result;
        MYSQL_ROW row;
        char sql[4096] = {0};
        sprintf(sql, "select code,username,score,selected_course,num_major,is_enough,teachers from T_Users where code=%s", code);
        result = executeQuery(sql, host, user, pass, db);
        // 因为code为主键，所以至多有一条记录
        if(row=mysql_fetch_row(result))
        {
          cgiHeaderContentType("text/html;charset=utf-8");
          fprintf(cgiOut,"<html><head><title>选课页面</title></head><body>");
          fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s号用户数据</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
          fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>任课老师</td></tr></thead>");
          fprintf(cgiOut,"<tbody style='text-align:center'>");
          fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",row[0],row[1],row[2],row[3],row[4],row[6]);
          fprintf(cgiOut,"</tbody>");
          fprintf(cgiOut,"</table>");
          char course[256]={0};
          strcpy(course,row[3]);
          char selected_course[50][20];
          int pos=0,count=0;
          for(i=0;i<=strlen(course);i++)
          {
              if(course[i]!=','){selected_course[count][pos]=course[i];pos+=1;}
              else{pos=0;count+=1;}
          }
          char teacher[256]={0};
          strcpy(teacher,row[6]);
          char teachers[50][20];
          int pos_=0,count_=0;
          for(i=0;i<=strlen(teacher);i++)
          {
              if(teacher[i]!=','){teachers[count_][pos_]=teacher[i];pos_+=1;}
              else{pos_=0;count_+=1;}
          }
          fprintf(cgiOut,"<font color='orange'>温馨提示：</font></br>");
          if(atoi(row[2])>=24)
          {
            fprintf(cgiOut,"<font color='green'>已选课程学分已达标</font></br>");
          }
          else
          {
            fprintf(cgiOut,"<font color='red'>已选课程学分未达标</font></br>");
          }
          if(strcmp(row[5],"是")==0)
          {
            fprintf(cgiOut,"<font color='green'>必修课已全选</font></br>");
          }
          else
          {
            fprintf(cgiOut,"<font color='red'>必修课未全选</font></br>");
          }
          fprintf(cgiOut,"<font color='blue'>请选择你要进行的操作：</font>");
          fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=find'>查找课程</a>&nbsp;&nbsp;&nbsp;",code);
          fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=sort'>课程排序</a>&nbsp;&nbsp;&nbsp;",code);
          fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=select'>开始选课</a>&nbsp;&nbsp;&nbsp;",code);
          fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=list'>返回课程列表</a>",code);
          fprintf(cgiOut,"<html><head><title>课程列表</title></head><body>");
          fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s号用户已选课程列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
          fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课老师1</td><td>任课老师2</td><td>任课老师3</td><td>任课老师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td><td>已选任课老师</td></tr></thead>");
          fprintf(cgiOut,"<tbody style='text-align:center'>");
          MYSQL_RES *result = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
          MYSQL_ROW row;
          count = 0;
          count_ = 0;
          while(row=mysql_fetch_row(result))
          {
            if(strcmp(selected_course[count],row[0])==0)
            {
              fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td>",\
                  row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
              count+=1;
              fprintf(cgiOut,"<td>%s</td></tr>",teachers[count_]);
              count_+=1;
            }
          }
          fprintf(cgiOut,"</tbody>");
          fprintf(cgiOut,"</table>");
          fprintf(cgiOut,"</body></html>");
          return 0;
        }
      }
      else if (strcmp(action, "find")==0)
      {
          cgiHeaderContentType("text/html;charset=utf-8");
          fprintf(cgiOut,"<html><head><title>课程查找</title></head><body>");
          fprintf(cgiOut, "<form action='student_manage.cgi' method='post'>");
          fprintf(cgiOut, "<input type='hidden' name='code' value='%s'/>",code);
          fprintf(cgiOut, "<input type='hidden' name='mod' value='course'/>");
          fprintf(cgiOut, "<input type='hidden' name='action' value='find_submit'/>");
          fprintf(cgiOut, "<font color='blue'>请选择查找的字段：</font>");
          fprintf(cgiOut,"<select name='find'>");
          fprintf(cgiOut,"<option value='code'>课程代码</option>");
          fprintf(cgiOut,"<option value='name'>课程名称</option>");
          fprintf(cgiOut,"</select>");
          fprintf(cgiOut,"&nbsp;&nbsp;&nbsp;<input type='text' name='value'/>");
          fprintf(cgiOut,"<input type='submit' name='btnFind' value='查找'/>");
          fprintf(cgiOut, "</form>");
          fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>课程列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
          fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课老师1</td><td>任课老师2</td><td>任课老师3</td><td>任课老师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
          fprintf(cgiOut,"<tbody style='text-align:center'>");
          MYSQL_RES *result = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
          MYSQL_ROW row;
          while(row=mysql_fetch_row(result))
          {
              fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",\
                  row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
          }
          fprintf(cgiOut,"</tbody>");
          fprintf(cgiOut,"</table>");
          fprintf(cgiOut,"</body></html>");
          mysql_free_result(result);
          return 0;
      }
      else if (strcmp(action, "find_submit")==0)
      {
          char sql[4096]={0}, select[256]={0}, value[256]={0};

          cgiFormString("find",select,sizeof(select));
          if(cgiFormString("value",value,sizeof(value))!=cgiFormSuccess)
          {
              show_error("请提供要查找的字段值！");
              return 0;
          }
          sprintf(sql,"SELECT * FROM T_Courses WHERE LOCATE('%s',%s)>0",value,select);
          MYSQL_RES *result = executeQuery(sql, host, user, pass, db);
          MYSQL_ROW row;

          cgiHeaderContentType("text/html;charset=utf-8");
          fprintf(cgiOut,"<html><head><title>查找结果</title></head><body>");
          fprintf(cgiOut,"<a href='student_manage.cgi?code=%s&mod=course&action=list'>返回课程列表</a>&nbsp;&nbsp;&nbsp;",code);
          fprintf(cgiOut,"<a href='student_manage.cgi?code=%s&mod=course&action=find'>返回课程查找</a>",code);
          fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>");
          if(strcmp(select,"code")==0){fprintf(cgiOut,"课程代码");}
          else if(strcmp(select,"name")==0){fprintf(cgiOut,"课程名称");}
          fprintf(cgiOut,"查找结果</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
          fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课老师1</td><td>任课老师2</td><td>任课老师3</td><td>任课老师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
          fprintf(cgiOut,"<tbody style='text-align:center'>");
          while(row=mysql_fetch_row(result))
          {
              fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",\
              row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
          }
          mysql_free_result(result);
          return 0;
      }
      else if (strcmp(action, "sort")==0)
      {
          cgiHeaderContentType("text/html;charset=utf-8");
          fprintf(cgiOut,"<html><head><title>课程排序</title></head><body>");
          fprintf(cgiOut, "<form action='student_manage.cgi' method='post'>");
          fprintf(cgiOut, "<input type='hidden' name='code' value='%s'/>",code);
          fprintf(cgiOut, "<input type='hidden' name='mod' value='course'/>");
          fprintf(cgiOut, "<input type='hidden' name='action' value='sort_submit'/>");
          fprintf(cgiOut, "<font color='blue'>请选择排序的字段：</font>");
          fprintf(cgiOut,"<select name='sort'>");
          fprintf(cgiOut,"<option value='code'>课程代码</option>");
          fprintf(cgiOut,"<option value='name'>课程名称</option>");
          fprintf(cgiOut,"<option value='classification'>种类</option>");
          fprintf(cgiOut,"<option value='score'>学分</option>");
          fprintf(cgiOut,"<option value='time'>上课时间</option>");
          fprintf(cgiOut,"<option value='teacher1'>任课老师1</option>");
          fprintf(cgiOut,"<option value='teacher2'>任课老师2</option>");
          fprintf(cgiOut,"<option value='teacher3'>任课老师3</option>");
          fprintf(cgiOut,"<option value='teacher4'>任课老师4</option>");
          fprintf(cgiOut,"<option value='classroom'>上课教室</option>");
          fprintf(cgiOut,"<option value='capacity'>课程容量</option>");
          fprintf(cgiOut,"<option value='num'>选课人数</option>");
          fprintf(cgiOut,"</select>");
          fprintf(cgiOut,"<input type='radio' name='order' value='ASC' id='order_asc' checked='true'/>升序</label>");
          fprintf(cgiOut,"<input type='radio' name='order' value='DESC' id='order_desc'/>降序</label>");
          fprintf(cgiOut,"&nbsp;&nbsp;&nbsp;<input type='submit' name='btnSort' value='排序'/>");
          fprintf(cgiOut, "</form>");
          fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>课程列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
          fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课老师1</td><td>任课老师2</td><td>任课老师3</td><td>任课老师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
          fprintf(cgiOut,"<tbody style='text-align:center'>");
          MYSQL_RES *result = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
          MYSQL_ROW row;
          while(row=mysql_fetch_row(result))
          {
              fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",\
                  row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
          }
          fprintf(cgiOut,"</tbody>");
          fprintf(cgiOut,"</table>");
          fprintf(cgiOut,"</body></html>");
          mysql_free_result(result);
          return 0;
      }
      else if (strcmp(action, "sort_submit")==0)
      {
          char sql[4096]={0}, select[256]={0}, order[256]={0};

          cgiFormString("sort",select,sizeof(select));
          cgiFormString("order",order,sizeof(order));
          sprintf(sql,"SELECT * from T_Courses ORDER BY %s %s",select,order);
          MYSQL_RES *result = executeQuery(sql, host, user, pass, db);
          MYSQL_ROW row;

          cgiHeaderContentType("text/html;charset=utf-8");
          fprintf(cgiOut,"<html><head><title>排序结果</title></head><body>");
          fprintf(cgiOut,"<a href='student_manage.cgi?code=%s&mod=course&action=list'>返回课程列表</a>&nbsp;&nbsp;&nbsp;",code);
          fprintf(cgiOut,"<a href='student_manage.cgi?code=%s&mod=course&action=sort'>返回课程排序</a>",code);
          fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>");
          if(strcmp(select,"code")==0){fprintf(cgiOut,"课程代码");}
          else if(strcmp(select,"name")==0){fprintf(cgiOut,"课程名称");}
          else if(strcmp(select,"classification")==0){fprintf(cgiOut,"种类");}
          else if(strcmp(select,"score")==0){fprintf(cgiOut,"学分");}
          else if(strcmp(select,"time")==0){fprintf(cgiOut,"上课时间");}
          else if(strcmp(select,"teacher1")==0){fprintf(cgiOut,"任课老师1");}
          else if(strcmp(select,"teacher2")==0){fprintf(cgiOut,"任课老师2");}
          else if(strcmp(select,"teacher3")==0){fprintf(cgiOut,"任课老师3");}
          else if(strcmp(select,"teacher4")==0){fprintf(cgiOut,"任课老师4");}
          else if(strcmp(select,"classroom")==0){fprintf(cgiOut,"上课教室");}
          else if(strcmp(select,"capacity")==0){fprintf(cgiOut,"课程容量");}
          else if(strcmp(select,"num")==0){fprintf(cgiOut,"选课人数");}
          if(strcmp(order,"ASC")==0){fprintf(cgiOut,"升序");}
          else if(strcmp(order,"DESC")==0){fprintf(cgiOut,"降序");}
          fprintf(cgiOut,"排序结果</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
          fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课老师1</td><td>任课老师2</td><td>任课老师3</td><td>任课老师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
          fprintf(cgiOut,"<tbody style='text-align:center'>");
          while(row=mysql_fetch_row(result))
          {
              fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",\
              row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
          }
          mysql_free_result(result);
          return 0;
        }
      else if (strcmp(action, "select")==0)
      {
        MYSQL_RES *result;
        MYSQL_ROW row;
        char sql[4096] = {0};
        sprintf(sql, "select code,username,score,selected_course,num_major,is_enough,teachers from T_Users where code=%s", code);
        result = executeQuery(sql, host, user, pass, db);
        // 因为code为主键，所以至多有一条记录
        if(row=mysql_fetch_row(result))
        {
            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<html><head><title>选课页面</title></head><body>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s号用户原始数据</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
            fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>任课老师</td></tr></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",row[0],row[1],row[2],row[3],row[4],row[6]);
            fprintf(cgiOut,"</tbody>");
            fprintf(cgiOut,"</table>");
            char course[256]={0};
            strcpy(course,row[3]);
            char selected_course[50][20];
            int pos=0,count=0;
            for(i=0;i<=strlen(course);i++)
            {
                if(course[i]!=','){selected_course[count][pos]=course[i];pos+=1;}
                else{pos=0;count+=1;}
            }
            char teacher[256]={0};
            strcpy(teacher,row[6]);
            char teachers[50][20];
            int pos_=0,count_=0;
            for(i=0;i<=strlen(teacher);i++)
            {
                if(teacher[i]!=','){teachers[count_][pos_]=teacher[i];pos_+=1;}
                else{pos_=0;count_+=1;}
            }
            fprintf(cgiOut,"<font color='orange'>温馨提示：</font></br>");
            if(atoi(row[2])>=24)
            {
              fprintf(cgiOut,"<font color='green'>已选课程学分已达标</font></br>");
            }
            else
            {
              fprintf(cgiOut,"<font color='red'>已选课程学分未达标</font></br>");
            }
            if(strcmp(row[5],"是")==0)
            {
              fprintf(cgiOut,"<font color='green'>必修课已全选</font></br>");
            }
            else
            {
              fprintf(cgiOut,"<font color='red'>必修课未全选</font></br>");
            }
            fprintf(cgiOut,"<font color='orange'>请进行选课：</font></br>");
            fprintf(cgiOut,"<form action='student_manage.cgi' method='post'>\
                            <input type='hidden' name='code' value='%s'/>\
                            <input type='hidden' name='mod' value='course'/>\
                            <input type='hidden' name='action' value='select_submit'/>",code);
            fprintf(cgiOut,"<font color='blue'>请选择你要进行的操作：</font>");
            fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=find'>查找课程</a>&nbsp;&nbsp;&nbsp;",code);
            fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=sort'>课程排序</a>&nbsp;&nbsp;&nbsp;",code);
            fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=view'>浏览已选课程</a>&nbsp;&nbsp;&nbsp;",code);
            fprintf(cgiOut, "<a href='student_manage.cgi?code=%s&mod=course&action=list'>返回课程列表</a>&nbsp;&nbsp;&nbsp;",code);
            fprintf(cgiOut,"<input type='submit' name='btnAddnew' value='确认选课'/></br>");
            fprintf(cgiOut,"<html><head><title>课程列表</title></head><body>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s号用户选课列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
            fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课老师1</td><td>任课老师2</td><td>任课老师3</td><td>任课老师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td><td><font color='blue'>是否选择</font></td><td><font color='blue'>选择任课老师</font></td></tr></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            MYSQL_RES *result = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
            MYSQL_ROW row;
            count = 0;
            count_ = 0;
            while(row=mysql_fetch_row(result))
            {
              fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td>",\
                  row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
              if(strcmp(selected_course[count],row[0])!=0)
              {
                  fprintf(cgiOut,"<td><input type='radio' name='course%s' value='%s' id='choose_yes'/><font color='green'>✓</font></label>",row[0],row[0]);
                  fprintf(cgiOut,"<input type='radio' name='course%s' value='' id='choose_no' checked/><font color='red'>×</font></label></td>",row[0]);
              }
              else
              {
                  fprintf(cgiOut,"<td><input type='radio' name='course%s' value='%s' id='choose_yes' checked/><font color='green'>✓</font></label>",row[0],row[0]);
                  fprintf(cgiOut,"<input type='radio' name='course%s' value='' id='choose_no'/><font color='red'>×</font></label></td>",row[0]);
                  count+=1;
              }
              fprintf(cgiOut,"<td><select name='teachers%s'>",row[0]);
              if(strcmp(row[5],"")!=0)
              {
                  if(strcmp(teachers[count_],row[5])!=0){fprintf(cgiOut,"<option value='%s'>%s</option>",row[5],row[5]);}
                  else{fprintf(cgiOut,"<option value='%s' selected>%s</option>",row[5],row[5]);count_+=1;}
              }
              if(strcmp(row[6],"")!=0)
              {
                  if(strcmp(teachers[count_],row[6])!=0){fprintf(cgiOut,"<option value='%s'>%s</option>",row[6],row[6]);}
                  else{fprintf(cgiOut,"<option value='%s' selected>%s</option>",row[6],row[6]);count_+=1;}
              }
              if(strcmp(row[7],"")!=0)
              {
                  if(strcmp(teachers[count_],row[7])!=0){fprintf(cgiOut,"<option value='%s'>%s</option>",row[7],row[7]);}
                  else{fprintf(cgiOut,"<option value='%s' selected>%s</option>",row[7],row[7]);count_+=1;}
              }
              if(strcmp(row[8],"")!=0)
              {
                  if(strcmp(teachers[count_],row[8])!=0){fprintf(cgiOut,"<option value='%s'>%s</option>",row[8],row[8]);}
                  else{fprintf(cgiOut,"<option value='%s' selected>%s</option>",row[8],row[8]);count_+=1;}
              }
              fprintf(cgiOut,"</select></td></tr>");   
          }
          fprintf(cgiOut,"</tbody>");
          fprintf(cgiOut,"</table>");
          fprintf(cgiOut,"</form>");
          fprintf(cgiOut,"</body></html>");
          return 0;
        }
      }
      else if (strcmp(action, "select_submit")==0)
      {
        char course[2048]={0}, selected_course[2048]={0}, teacher[2048]={0}, teachers[2048]={0};
        
        MYSQL_RES *result_course = executeQuery("select code,time,capacity,num from T_Courses", host, user, pass, db);
        MYSQL_ROW row_course;
        char times[50][256];
        int count=1;
        while(row_course=mysql_fetch_row(result_course))
        {
          char radio[20]={0};
          char tmp[10]={0};
          sprintf(radio,"course%s",row_course[0]);
          if(cgiFormString(radio,tmp,sizeof(tmp))==cgiFormSuccess);
          {
            if(strcmp(tmp,"")!=0)  //选择该课程
            {
              strcat(course,tmp);strcat(course,",");
              //判断是否超出课程容量
              if(strcmp(row_course[2],row_course[3])==0)
              {
                char error[1024]={0};
                sprintf(error,"课程代码为%s的课程选课人数已达最大容量%s",row_course[0],row_course[2]);
                show_error(error);
                return 0;
              }
              //教师信息
              char select[20]={0};
              char tmp_[256]={0};
              sprintf(select,"teachers%s",row_course[0]);
              if(cgiFormString(select,tmp_,sizeof(tmp_))==cgiFormSuccess);
              {
                  strcat(teacher,tmp_);strcat(teacher,",");
              }
              //判断课程时间是否冲突
              for(i=0;i<count;i++)
              {
                if(strcmp(times[i],row_course[1])==0)
                {
                  char error[1024]={0};
                  sprintf(error,"课程代码为%s的课程与已选课程时间冲突",row_course[0]);
                  show_error(error);
                  return 0;
                }
              }
              strcpy(times[count-1],row_course[1]);
              count += 1;
            }    
          }
        }
        for(i=0;i<strlen(course)-1;i++){selected_course[i]=course[i];} //删除末尾','
        for(i=0;i<strlen(teacher)-1;i++){teachers[i]=teacher[i];} //删除末尾','
        //修改用户记录
        char sql[4096]={0};
        sprintf(sql,"update T_Users set selected_course='%s',teachers='%s' where code='%s'",\
                    selected_course,teachers,code);
        executeNonQuery(sql, host, user, pass, db);
        
        //计算学分和主修专业数，并修改记录
        int score=0, num_major=0;
        char *tmp=strtok(selected_course,",");
        while(tmp != NULL)
        {
            char sql[1024]={0};
            
            sprintf(sql, "select score,classification from T_Courses where code=%s",tmp);
            MYSQL_RES *res = executeQuery(sql, host, user, pass, db);
            MYSQL_ROW ro;
            while(ro=mysql_fetch_row(res))
            {
                score += atoi(ro[0]);
                if(strcmp(ro[1],"主修")==0){num_major+=1;}
            }
            tmp = strtok(NULL, ",");
            mysql_free_result(res);
        }
        char is_enough[10]={0};
        if(num_major>=sum_major){strcpy(is_enough,"是");}else{strcpy(is_enough,"否");}
        sprintf(sql,"update T_Users set score='%d',num_major='%d',is_enough='%s' where code='%s'",score,num_major,is_enough,code);
        executeNonQuery(sql, host, user, pass, db);
        char url[1024]={0};
        sprintf(url,"student_manage.cgi?code=%s&mod=course&action=list",code);
        cgiHeaderLocation(url);
        mysql_free_result(result_course);
        return 0;
      }
      else
      {
        show_error("未知的action参数");
        return 0;
      }
    }
    else
    {
      show_error("未知的mod参数");
      return 0;
    }
  }
  else
  {
    show_error("用户不存在");
    return 0;
  }

}