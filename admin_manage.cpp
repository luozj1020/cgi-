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
    int i;

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
    if(strcmp(mod, "course")==0)
    {
        if(strcmp(action, "list")==0)
        {
            char filename[1024]={0};
            sum_major = 0;

            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<font color='orange'>请选择你要进行的操作：</font>");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=course&action=init'>重置课程</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=course&action=addnew'>添加课程</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=course&action=find'>查找课程</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=course&action=sort'>课程排序</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=student&action=list'>查看用户列表</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='login.cgi'>退出登录</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut,"<html><head><title>课程列表</title></head><body>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>课程列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td><td></td><td></td></tr></thead>");
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
                fprintf(cgiOut,"<td>%d</td>",num);
                char sql[1024]={0};
                sprintf(sql,"update T_Courses set num=%d where code='%s'",num,row[0]);
                executeNonQuery(sql, host, user, pass, db);
                fprintf(cgiOut, "<td><a href='admin_manage.cgi?mod=course&action=delete&code=%s'>删除</td><td><a href='admin_manage.cgi?mod=course&action=edit&code=%s'>修改</td></tr>",row[0],row[0]);
            }
            fprintf(cgiOut,"</tbody>");
            fprintf(cgiOut,"</table>");
            fprintf(cgiOut,"</body></html>");
            time(&tmpcal_ptr);tmp_ptr = gmtime(&tmpcal_ptr);tmp_ptr = localtime(&tmpcal_ptr);
            sprintf(filename,"course_data_%d%d%d-%d:%d:%d.txt",(1900+tmp_ptr->tm_year), (1+tmp_ptr->tm_mon), tmp_ptr->tm_mday, tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec);
            //export_data(filename, "T_Courses", host, user, pass, db);
            mysql_free_result(result);
            return 0;
        }
        else if (strcmp(action, "init")==0)
        {
            import_data("course_data.txt", host, user, pass, db);
            cgiHeaderLocation("admin_manage.cgi?mod=course&action=list");
        }
        else if (strcmp(action, "addnew")==0)
        {
            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<html><head><title>课程添加</title></head><body>\
                            <form action='admin_manage.cgi' method='post'>\
                                <input type='hidden' name='mod' value='course'/>\
                                <input type='hidden' name='action' value='addnew_submit'/>\
                                课程代码：<input type='text' name='code'/></br>\
                                课程名称：<input type='text' name='name'/></br>\
                                种类：<input type='radio' name='classification' value='主修' id='class_main'/>主修</label>\
                                    <input type='radio' name='classification' value='辅修' id='class_sub_main'/>辅修</label></br>\
                                学分：<input type='text' name='score' value='0'/></br>");
            fprintf(cgiOut,"上课时间：");
            fprintf(cgiOut,"<select name='day'>");
            for(i=0;i<7;i++){fprintf(cgiOut,"<option value='星期%s'>星期%s</option>",day_week[i],day_week[i]);}
            fprintf(cgiOut,"</select>/");
            fprintf(cgiOut,"<select name='num_class'>");
            for(i=0;i<9;i++){fprintf(cgiOut,"<option value='第%d节'>第%d节</option>",i+1,i+1);}
            fprintf(cgiOut,"</select></br>");
            fprintf(cgiOut,"任课教师1：<input type='text' name='teacher1'/></br>\
                                任课教师2：<input type='text' name='teacher2'/></br>\
                                任课教师3：<input type='text' name='teacher3'/></br>\
                                任课教师4：<input type='text' name='teacher4'/></br>\
                                上课教室：<input type='text' name='classroom'/></br>\
                                课程容量：<input type='text' name='capacity' value='0'/></br>\
                                选课人数：<input type='text' name='num' value='0'/></br>\
                                <input type='submit' name='btnAddnew' value='添加'/>\
                            </form>");
            return 0;
        }
        else if (strcmp(action, "addnew_submit")==0)
        {
            char code[256]={0}, name[256]={0}, classification[256]={0}, score[256]={0}, day[256]={0}, num_class[256]={0}, teacher1[256]={0}, teacher2[256]={0}, 
                teacher3[256]={0}, teacher4[256]={0}, classroom[256]={0}, capacity[256]={0}, num[256]={0};
            
            if(cgiFormString("code",code,sizeof(code))!=cgiFormSuccess)
            {
                show_error("课程代码不能为空！");
                return 0;
            }
            MYSQL_RES *result = executeQuery("select code from T_Courses", host, user, pass, db);
            MYSQL_ROW row;

            while(row=mysql_fetch_row(result))
            {
                if(strcmp(code, row[0])==0)
                {
                    show_error("课程代码重复！");
                    return 0;
                }
            }

            cgiFormString("name",name,sizeof(name));
            cgiFormString("classification",classification,sizeof(classification));
            cgiFormString("score",score,sizeof(score));
            cgiFormString("day",day,sizeof(day));
            cgiFormString("num_class",num_class,sizeof(num_class));
            cgiFormString("teacher1",teacher1,sizeof(teacher1));
            cgiFormString("teacher2",teacher2,sizeof(teacher2));
            cgiFormString("teacher3",teacher3,sizeof(teacher3));
            cgiFormString("teacher4",teacher4,sizeof(teacher4));
            cgiFormString("classroom",classroom,sizeof(classroom));
            cgiFormString("capacity",capacity,sizeof(capacity));
            cgiFormString("num",num,sizeof(num));
            if(strcmp(teacher1,"")==0 && strcmp(teacher2,"")==0 && strcmp(teacher3,"")==0 && strcmp(teacher4,"")==0)
            {
                show_error("至少要有一名任课教师！");
                return 0;
            }
            
            if(!is_digit_str(score)){show_error("请正确输入学分！");return 0;}
            if(!is_digit_str(capacity)){show_error("请正确输入课程容量！");return 0;}
            if(!is_digit_str(num)){show_error("请正确输入选课人数！");return 0;}
            char sql[4096]={0};
            sprintf(sql,"Insert into T_Courses(code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num) \
                    values('%s','%s', '%s', '%s', '%s/%s', '%s', '%s', '%s', '%s', '%s', %s, %s)",\
                    code,name,classification,score,day,num_class,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num);
            executeNonQuery(sql, host, user, pass, db);
            cgiHeaderLocation("admin_manage.cgi?mod=course&action=list"); //重定向返回教师列表
            mysql_free_result(result);
            return 0;
        }
        else if (strcmp(action, "edit")==0)
        {
            char code[256]={0};

            if(cgiFormString("code",code,sizeof(code))!=cgiFormSuccess)
            {
                show_error("请提供要修改的课程代码！");
                return 0;
            }
            MYSQL_RES *result;
            MYSQL_ROW row;
            char sql[4096] = {0};
            sprintf(sql, "select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses where code=%s", code);
            result = executeQuery(sql, host, user, pass, db);
            // 因为code为主键，所以至多有一条记录
            if(row=mysql_fetch_row(result))
            {
                char day[256]={0}, num_class[256]={0};
                cgiHeaderContentType("text/html;charset=utf-8");
                fprintf(cgiOut,"<html><head><title>课程修改</title></head><body>");
                fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s原始数据</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
                fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
                fprintf(cgiOut,"<tbody style='text-align:center'>");
                fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",\
                        row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
                fprintf(cgiOut,"</tbody>");
                fprintf(cgiOut,"</table>");
                fprintf(cgiOut,"<font color='orange'>请进行你的修改：</font></br>");
                fprintf(cgiOut,"<html><head><title>课程修改</title></head><body>\
                                <form action='admin_manage.cgi' method='post'>\
                                    <input type='hidden' name='mod' value='course'/>\
                                    <input type='hidden' name='action' value='edit_submit'/>\
                                    <input type='hidden' name='code' value='%s'/>\
                                    课程名称：<input type='text' name='name' value='%s'/></br>"\
                                ,row[0],row[1]);
                if(strcmp(row[2],"主修")==0)
                {
                    fprintf(cgiOut,"种类：<input type='radio' name='classification' value='主修' id='class_main' checked/>主修</label>\
                                    <input type='radio' name='classification' value='辅修' id='class_sub_main'/>辅修</label></br>");
                }
                else
                {
                    fprintf(cgiOut,"种类：<input type='radio' name='classification' value='主修' id='class_main'/>主修</label>\
                                    <input type='radio' name='classification' value='辅修' id='class_sub_main' checked/>辅修</label></br>");
                }
                fprintf(cgiOut,"学分：<input type='text' name='score' value='%s'/></br>",row[3]);
                
                char time[256]={0};
                strcpy(time, row[4]);
                char *tmp=strtok(time,"/");
                strcpy(day, tmp);
                tmp = strtok(NULL,"/");
                strcpy(num_class, tmp);
                fprintf(cgiOut,"上课时间：");
                
                fprintf(cgiOut,"<select name='day'>");
                for(i=0;i<7;i++)
                {
                    sprintf(tmp,"星期%s",day_week[i]);
                    if(strcmp(tmp,day)==0){fprintf(cgiOut,"<option value='%s' selected>%s</option>",tmp,tmp);}
                    else{fprintf(cgiOut,"<option value='%s'>%s</option>",tmp,tmp);}
                }
                fprintf(cgiOut,"</select>/");
                fprintf(cgiOut,"<select name='num_class'>");
                for(i=0;i<9;i++)
                {
                    sprintf(tmp,"第%d节",i+1);
                    if(strcmp(tmp,num_class)==0){fprintf(cgiOut,"<option value='%s' selected>%s</option>",tmp,tmp);}
                    else{fprintf(cgiOut,"<option value='%s'>%s</option>",tmp,tmp);}
                }
                fprintf(cgiOut,"</select></br>");
                printf("%c",row[5][1]);
                fprintf(cgiOut,"任课教师1：<input type='text' name='teacher1' value='%s'/></br>\
                                    任课教师2：<input type='text' name='teacher2' value='%s'/></br>\
                                    任课教师3：<input type='text' name='teacher3' value='%s'/></br>\
                                    任课教师4：<input type='text' name='teacher4' value='%s'/></br>\
                                    上课教室：<input type='text' name='classroom' value='%s'/></br>\
                                    课程容量：<input type='text' name='capacity' value='%s'/></br>\
                                    选课人数：<input type='text' name='num' value='%s'/></br>\
                                    <input type='submit' name='btnEdit' value='修改'/>\
                                </form>"\
                        ,row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
                fprintf(cgiOut,"</body></html>");
                mysql_free_result(result);
                return 0;
            }
            else
            {
                show_error("找不到这个课程代码对应的课程信息");
                return 0;
            }
        }
        else if (strcmp(action, "edit_submit")==0)
        {
            char code[256]={0}, name[256]={0}, classification[256]={0}, score[256]={0}, day[256]={0}, num_class[256]={0}, teacher1[256]={0}, teacher2[256]={0}, 
                teacher3[256]={0}, teacher4[256]={0}, classroom[256]={0}, capacity[256]={0}, num[256]={0};
            
            cgiFormString("code",code,sizeof(code));
            cgiFormString("name",name,sizeof(name));
            cgiFormString("classification",classification,sizeof(classification));
            cgiFormString("score",score,sizeof(score));
            cgiFormString("day",day,sizeof(day));
            cgiFormString("num_class",num_class,sizeof(num_class));
            cgiFormString("teacher1",teacher1,sizeof(teacher1));
            cgiFormString("teacher2",teacher2,sizeof(teacher2));
            cgiFormString("teacher3",teacher3,sizeof(teacher3));
            cgiFormString("teacher4",teacher4,sizeof(teacher4));
            cgiFormString("classroom",classroom,sizeof(classroom));
            cgiFormString("capacity",capacity,sizeof(capacity));
            cgiFormString("num",num,sizeof(num));
            if(strcmp(teacher1,"")==0 && strcmp(teacher2,"")==0 && strcmp(teacher3,"")==0 && strcmp(teacher4,"")==0)
            {
                show_error("至少要有一名任课教师！");
                return 0;
            }
            if(!is_digit_str(score)){show_error("请正确输入学分！");return 0;}
            if(!is_digit_str(capacity)){show_error("请正确输入课程容量！");return 0;}
            if(!is_digit_str(num)){show_error("请正确输入选课人数！");return 0;}

            char sql[4096]={0};
            sprintf(sql,"update T_Courses set\
            name='%s',classification='%s',score='%s',time='%s/%s',teacher1='%s',teacher2='%s',teacher3='%s',teacher4='%s',classroom='%s',capacity=%s,num=%s\
            where code='%s'"\
            ,name,classification,score,day,num_class,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num,code);
            executeNonQuery(sql, host, user, pass, db);
            cgiHeaderLocation("admin_manage.cgi?mod=course&action=list");
            return 0;
        }
        else if (strcmp(action, "delete")==0)
        {
            char code[256]={0};

            if(cgiFormString("code",code,sizeof(code))!=cgiFormSuccess)
            {
                show_error("请提供要删除的课程代码！");
                return 0;
            }
            MYSQL_RES *result;
            MYSQL_ROW row;
            char sql[4096] = {0};
            sprintf(sql, "select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses where code=%s", code);
            result = executeQuery(sql, host, user, pass, db);
            // 因为code为主键，所以至多有一条记录
            if(row=mysql_fetch_row(result))
            {
                cgiHeaderContentType("text/html;charset=utf-8");
                fprintf(cgiOut,"<html><head><title>课程删除</title></head><body>");
                fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s原始数据</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
                fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
                fprintf(cgiOut,"<tbody style='text-align:center'>");
                fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",\
                        row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
                fprintf(cgiOut,"</tbody>");
                fprintf(cgiOut,"</table>");
                fprintf(cgiOut, "<form action='admin_manage.cgi' method='post'>");
                fprintf(cgiOut, "<input type='hidden' name='mod' value='course'/>");
                fprintf(cgiOut, "<input type='hidden' name='action' value='delete_submit'/>");
                fprintf(cgiOut, "<input type='hidden' name='code' value='%s'/>",row[0]);
                fprintf(cgiOut, "<font color='red'>你确定要删除吗？（删除操作是不可逆的）</font>");
                fprintf(cgiOut,"<input type='radio' name='delete_or_not' value='yes' id='delete_yes'/>是</label>");
                fprintf(cgiOut,"<input type='radio' name='delete_or_not' value='no' id='delete_no'/>否</label>");
                fprintf(cgiOut,"<input type='submit' name='btnDelete' value='确定'/>");
                fprintf(cgiOut, "</form>");
                fprintf(cgiOut,"</body></html>");
                mysql_free_result(result);
                return 0;
            }
            else
            {
                show_error("找不到这个课程代码对应的课程信息");
                return 0;
            }
        }
        else if (strcmp(action, "delete_submit")==0)
        {
            char sql[4096]={0}, code[256]={0}, judge[256]={0};
            
            cgiFormString("code",code,sizeof(code));
            if(cgiFormString("delete_or_not",judge,sizeof(judge))!=cgiFormSuccess)
            {
                show_error("请确认是否删除！");
                return 0;
            }
            if(strcmp(judge,"yes")==0)
            {
                sprintf(sql,"delete from T_Courses where code='%s'", code);
                executeNonQuery(sql, host, user, pass, db);
            }
            cgiHeaderLocation("admin_manage.cgi?mod=course&action=list");
            return 0;
        }
        else if (strcmp(action, "find")==0)
        {
            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<html><head><title>课程查找</title></head><body>");
            fprintf(cgiOut, "<form action='admin_manage.cgi' method='post'>");
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
            fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
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
            fprintf(cgiOut,"<a href='admin_manage.cgi?mod=course&action=list'>返回课程列表</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut,"<a href='admin_manage.cgi?mod=course&action=find'>返回课程查找</a>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>");
            if(strcmp(select,"code")==0){fprintf(cgiOut,"课程代码");}
            else if(strcmp(select,"name")==0){fprintf(cgiOut,"课程名称");}
            fprintf(cgiOut,"查找结果</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
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
            fprintf(cgiOut, "<form action='admin_manage.cgi' method='post'>");
            fprintf(cgiOut, "<input type='hidden' name='mod' value='course'/>");
            fprintf(cgiOut, "<input type='hidden' name='action' value='sort_submit'/>");
            fprintf(cgiOut, "<font color='blue'>请选择排序的字段：</font>");
            fprintf(cgiOut,"<select name='sort'>");
            fprintf(cgiOut,"<option value='code'>课程代码</option>");
            fprintf(cgiOut,"<option value='name'>课程名称</option>");
            fprintf(cgiOut,"<option value='classification'>种类</option>");
            fprintf(cgiOut,"<option value='score'>学分</option>");
            fprintf(cgiOut,"<option value='time'>上课时间</option>");
            fprintf(cgiOut,"<option value='teacher1'>任课教师1</option>");
            fprintf(cgiOut,"<option value='teacher2'>任课教师2</option>");
            fprintf(cgiOut,"<option value='teacher3'>任课教师3</option>");
            fprintf(cgiOut,"<option value='teacher4'>任课教师4</option>");
            fprintf(cgiOut,"<option value='classroom'>上课教室</option>");
            fprintf(cgiOut,"<option value='capacity'>课程容量</option>");
            fprintf(cgiOut,"<option value='num'>选课人数</option>");
            fprintf(cgiOut,"</select>");
            fprintf(cgiOut,"<input type='radio' name='order' value='ASC' id='order_asc' checked='true'/>升序</label>");
            fprintf(cgiOut,"<input type='radio' name='order' value='DESC' id='order_desc'/>降序</label>");
            fprintf(cgiOut,"&nbsp;&nbsp;&nbsp;<input type='submit' name='btnSort' value='排序'/>");
            fprintf(cgiOut, "</form>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>课程列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
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
            fprintf(cgiOut,"<a href='admin_manage.cgi?mod=course&action=list'>返回课程列表</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut,"<a href='admin_manage.cgi?mod=course&action=sort'>返回课程排序</a>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>");
            if(strcmp(select,"code")==0){fprintf(cgiOut,"课程代码");}
            else if(strcmp(select,"name")==0){fprintf(cgiOut,"课程名称");}
            else if(strcmp(select,"classification")==0){fprintf(cgiOut,"种类");}
            else if(strcmp(select,"score")==0){fprintf(cgiOut,"学分");}
            else if(strcmp(select,"time")==0){fprintf(cgiOut,"上课时间");}
            else if(strcmp(select,"teacher1")==0){fprintf(cgiOut,"任课教师1");}
            else if(strcmp(select,"teacher2")==0){fprintf(cgiOut,"任课教师2");}
            else if(strcmp(select,"teacher3")==0){fprintf(cgiOut,"任课教师3");}
            else if(strcmp(select,"teacher4")==0){fprintf(cgiOut,"任课教师4");}
            else if(strcmp(select,"classroom")==0){fprintf(cgiOut,"上课教室");}
            else if(strcmp(select,"capacity")==0){fprintf(cgiOut,"课程容量");}
            else if(strcmp(select,"num")==0){fprintf(cgiOut,"选课人数");}
            if(strcmp(order,"ASC")==0){fprintf(cgiOut,"升序");}
            else if(strcmp(order,"DESC")==0){fprintf(cgiOut,"降序");}
            fprintf(cgiOut,"排序结果</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td></tr></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            while(row=mysql_fetch_row(result))
            {
                fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",\
                row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
            }
            mysql_free_result(result);
            return 0;
        }
        else
        {
            show_error("未知的action参数");
            return 0;
        }
    }
    else if(strcmp(mod, "student")==0)
    {
        MYSQL_RES *result1 = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
        MYSQL_ROW row1;
        //统计主修课总门数
        while(row1=mysql_fetch_row(result1)){if(strcmp(row1[2],"主修")==0){sum_major+=1;}}
        if(strcmp(action, "list")==0)
        {
            char filename[1024]={0};

            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<font color='orange'>请选择你要进行的操作：</font>");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=student&action=addnew'>添加用户</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=student&action=find'>查找用户</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=student&action=sort'>用户排序</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=course&action=list'>查看课程列表</a>");
            fprintf(cgiOut,"<html><head><title>用户列表</title></head><body>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>用户列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>主修是否已修满</td><td>任课教师</td><td></td><td></td></tr></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            MYSQL_RES *res = executeQuery("select code,username,selected_course from T_Users", host, user, pass, db);
            MYSQL_ROW ro;
            //计算学分，并修改记录
            char selected_course[1024]={0};
            while(ro=mysql_fetch_row(res))
            {
                int score=0;
                strcpy(selected_course, ro[2]);
                char *tmp=strtok(selected_course,",");
                while(tmp != NULL)
                {
                    char sql[1024]={0};
                    
                    sprintf(sql, "select score from T_Courses where code=%s",tmp);
                    MYSQL_RES *res_course = executeQuery(sql, host, user, pass, db);
                    MYSQL_ROW row_score;
                    while(row_score=mysql_fetch_row(res_course))
                    {
                        score += atoi(row_score[0]);
                    }
                    tmp = strtok(NULL, ",");
                    mysql_free_result(res_course);
                }
                char sql[1024]={0};
                sprintf(sql,"update T_Users set score='%d' where code='%s'",score,ro[0]);
                executeNonQuery(sql, host, user, pass, db);
            }
            mysql_free_result(res);
            //显示数据
            MYSQL_RES *result = executeQuery("select code,username,score,selected_course,num_major,is_enough,teachers from T_Users", host, user, pass, db);
            MYSQL_ROW row;
            while(row=mysql_fetch_row(result))
            {
                if(strcmp(row[1],"admin")!=0)
                {
                    fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td>",
                        row[0],row[1],row[2],row[3]);
                    char course[256]={0};
                    strcpy(course,row[3]);
                    char selected_course[50][20];
                    int pos=0,count=0;
                    for(i=0;i<=strlen(course);i++)
                    {
                        if(course[i]!=','){selected_course[count][pos]=course[i];pos+=1;}
                        else{pos=0;count+=1;}
                    }
                    //查找主修课数量，并写入数据
                    int num_major=0;
                    for(i=0;i<=count;i++)
                    {
                        char sql[1024]={0};
                        sprintf(sql, "select classification from T_Courses where code='%s'", selected_course[i]);
                        MYSQL_RES *result_ = executeQuery(sql, host, user, pass, db);
                        MYSQL_ROW row_;
                        if(row_=mysql_fetch_row(result_))
                        {
                            if(strcmp(row_[0],"主修")==0){num_major+=1;}
                        }
                        else
                        {
                            show_error("课程数据有误！");
                            return 0;
                        }
                    }
                    fprintf(cgiOut, "<td>%d</td>", num_major);
                    char sql[1024]={0};
                    sprintf(sql,"update T_Users set num_major='%d' where code='%s'",num_major,row[0]);
                    executeNonQuery(sql, host, user, pass, db);
                    if(num_major>=sum_major)
                    {
                        fprintf(cgiOut, "<td><font color='green'>是</font></td>");
                        char sql[1024]={0};
                        sprintf(sql,"update T_Users set is_enough='是' where code='%s'",row[0]);
                        executeNonQuery(sql, host, user, pass, db);
                    }
                    else
                    {
                        fprintf(cgiOut, "<td><font color='red'>否</font></td>");
                        char sql[1024]={0};
                        sprintf(sql,"update T_Users set is_enough='否' where code='%s'",row[0]);
                        executeNonQuery(sql, host, user, pass, db);
                    }
                    fprintf(cgiOut,"<td>%s</td>",row[6]);
                    fprintf(cgiOut,"<td><a href='admin_manage.cgi?mod=student&action=delete&code=%s'>删除</td><td><a href='admin_manage.cgi?mod=student&action=edit&code=%s'>修改</td></tr>",row[0],row[0]);
                }
            }
            fprintf(cgiOut,"</tbody>");
            fprintf(cgiOut,"</table>");
            fprintf(cgiOut,"</body></html>");
            time(&tmpcal_ptr);tmp_ptr = gmtime(&tmpcal_ptr);tmp_ptr = localtime(&tmpcal_ptr);
            sprintf(filename,"user_data_%d%d%d-%d:%d:%d.txt",(1900+tmp_ptr->tm_year), (1+tmp_ptr->tm_mon), tmp_ptr->tm_mday, tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec);
            export_data(filename, "T_Users", host, user, pass, db);
            mysql_free_result(result);
            return 0;
        }
        else if (strcmp(action, "addnew")==0)
        {
            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<html><head><title>用户添加</title></head><body>\
                            <form action='admin_manage.cgi' method='post'>\
                                <input type='hidden' name='mod' value='student'/>\
                                <input type='hidden' name='action' value='addnew_submit'/>\
                                学号：<input type='text' name='code'/>\
                                用户名：<input type='text' name='username'/>\
                                <input type='submit' name='btnAddnew' value='添加'/></br>");
            fprintf(cgiOut,"<font color='orange'>请选择你要进行的操作：</font>");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=student&action=find'>查找课程</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut, "<a href='admin_manage.cgi?mod=student&action=sort'>课程排序&nbsp;&nbsp;&nbsp;</a>");
            fprintf(cgiOut, "<a href='login.cgi'>退出登录</a>");
            fprintf(cgiOut,"<html><head><title>课程列表</title></head><body>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>用户列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td><td><font color='blue'>是否选择</font></td><td><font color='blue'>选择任课教师</font></td></tr></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            MYSQL_RES *result = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
            MYSQL_ROW row;
            while(row=mysql_fetch_row(result))
            {
                fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td>",\
                    row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
                fprintf(cgiOut,"<td><input type='radio' name='course%s' value='%s' id='choose_yes'/><font color='green'>✓</font></label>",row[0],row[0]);  
                fprintf(cgiOut,"<input type='radio' name='course%s' value='' id='choose_no' checked/><font color='red'>×</font></label></td>",row[0]);
                fprintf(cgiOut,"<td><select name='teachers%s'>",row[0]);
                if(strcmp(row[5],"")!=0){fprintf(cgiOut,"<option value='%s'>%s</option>",row[5],row[5]);}
                if(strcmp(row[6],"")!=0){fprintf(cgiOut,"<option value='%s'>%s</option>",row[6],row[6]);}
                if(strcmp(row[7],"")!=0){fprintf(cgiOut,"<option value='%s'>%s</option>",row[7],row[7]);}
                if(strcmp(row[8],"")!=0){fprintf(cgiOut,"<option value='%s'>%s</option>",row[8],row[8]);}
                fprintf(cgiOut,"</select></td></tr>"); 
            }
            fprintf(cgiOut,"</tbody>");
            fprintf(cgiOut,"</table>");
            fprintf(cgiOut,"</form>");
            fprintf(cgiOut,"</body></html>");
            return 0;
        }
        else if (strcmp(action, "addnew_submit")==0)
        {
            char code[256]={0}, username[256]={0}, course[2048]={0}, selected_course[2048]={0}, teacher[2048]={0}, teachers[2048]={0};
            
            if(cgiFormString("code",code,sizeof(code))!=cgiFormSuccess)
            {
                show_error("学号不能为空！");
                return 0;
            }
            MYSQL_RES *result = executeQuery("select code from T_Users", host, user, pass, db);
            MYSQL_ROW row;
            while(row=mysql_fetch_row(result))
            {
                if(strcmp(code, row[0])==0)
                {
                    show_error("学号重复！");
                    return 0;
                }
            }
            cgiFormString("username",username,sizeof(username));
            
            
            MYSQL_RES *result_course = executeQuery("select code from T_Courses", host, user, pass, db);
            MYSQL_ROW row_course;
            while(row_course=mysql_fetch_row(result_course))
            {
                char radio[20]={0};
                char tmp[10]={0};
                sprintf(radio,"course%s",row_course[0]);
                if(cgiFormString(radio,tmp,sizeof(tmp))==cgiFormSuccess);
                {
                    if(strcmp(tmp,"")!=0)
                    {
                        strcat(course,tmp);strcat(course,",");
                        //教师信息
                        char select[20]={0};
                        char tmp_[256]={0};
                        sprintf(select,"teachers%s",row_course[0]);
                        if(cgiFormString(select,tmp_,sizeof(tmp_))==cgiFormSuccess);
                        {
                            strcat(teacher,tmp_);strcat(teacher,",");
                        }
                    }
                }
            }
            for(i=0;i<strlen(course)-1;i++){selected_course[i]=course[i];} //删除末尾','
            for(i=0;i<strlen(teacher)-1;i++){teachers[i]=teacher[i];} //删除末尾','
            //计算学分
            int score=0;
            char *tmp_=strtok(course,",");
            while(tmp_ != NULL)
            {
                char sql[2048]={0};
                
                sprintf(sql, "select score from T_Courses where code=%s",tmp_);
                MYSQL_RES *res_course = executeQuery(sql, host, user, pass, db);
                MYSQL_ROW row_score;
                while(row_score=mysql_fetch_row(res_course))
                {
                    score += atoi(row_score[0]);
                }
                tmp_ = strtok(NULL, ",");
                mysql_free_result(res_course);
            }
            
            char sql[4096]={0};
            sprintf(sql,"Insert into T_Users(code,username,password,score,selected_course,teachers) \
                    values('%s','%s','123','%d', '%s','%s')",\
                    code,username,score,selected_course,teachers);
            executeNonQuery(sql, host, user, pass, db);
            cgiHeaderLocation("admin_manage.cgi?mod=student&action=list"); //重定向返回学生列表
            mysql_free_result(result);
            return 0;
        }
        else if (strcmp(action, "edit")==0)
        {
            char code[256]={0};

            if(cgiFormString("code",code,sizeof(code))!=cgiFormSuccess)
            {
                show_error("请提供要修改的用户学号！");
                return 0;
            }
            MYSQL_RES *result;
            MYSQL_ROW row;
            char sql[4096] = {0};
            sprintf(sql, "select code,username,password,score,selected_course,num_major,is_enough,teachers from T_Users where code=%s", code);
            result = executeQuery(sql, host, user, pass, db);
            // 因为code为主键，所以至多有一条记录
            if(row=mysql_fetch_row(result))
            {
                cgiHeaderContentType("text/html;charset=utf-8");
                fprintf(cgiOut,"<html><head><title>用户修改</title></head><body>");
                fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s号用户原始数据</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
                fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>密码</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>主修是否已修满</td><td>任课教师</td></tr></thead>");
                fprintf(cgiOut,"<tbody style='text-align:center'>");
                fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7]);
                char course[256]={0};
                strcpy(course,row[4]);
                char selected_course[50][20];
                int pos=0,count=0;
                for(i=0;i<=strlen(course);i++)
                {
                    if(course[i]!=','){selected_course[count][pos]=course[i];pos+=1;}
                    else{pos=0;count+=1;}
                }
                char teacher[256]={0};
                strcpy(teacher,row[7]);
                char teachers[50][20];
                int pos_=0,count_=0;
                for(i=0;i<=strlen(teacher);i++)
                {
                    if(teacher[i]!=','){teachers[count_][pos_]=teacher[i];pos_+=1;}
                    else{pos_=0;count_+=1;}
                }
                fprintf(cgiOut,"</tbody>");
                fprintf(cgiOut,"</table>");
                fprintf(cgiOut,"<font color='orange'>请进行你的修改：</font></br>");
                fprintf(cgiOut,"<form action='admin_manage.cgi' method='post'>\
                                <input type='hidden' name='mod' value='student'/>\
                                <input type='hidden' name='code' value='%s'/>\
                                <input type='hidden' name='action' value='edit_submit'/>",row[0]);
                fprintf(cgiOut,"用户名：<input type='text' name='username' value='%s'/>\
                                密码：<input type='text' name='password' value='%s'/>\
                                <input type='submit' name='btnAddnew' value='修改'/></br>",row[1],row[2]);
                fprintf(cgiOut,"<font color='blue'>请选择你要进行的操作：</font>");
                fprintf(cgiOut, "<a href='admin_manage.cgi?mod=student&action=find'>查找课程</a>&nbsp;&nbsp;&nbsp;");
                fprintf(cgiOut, "<a href='admin_manage.cgi?mod=student&action=sort'>课程排序</a>");
                fprintf(cgiOut,"<html><head><title>课程列表</title></head><body>");
                fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s号用户选课列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
                fprintf(cgiOut,"<tr><td>课程代码</td><td>课程名称</td><td>种类</td><td>学分</td><td>上课时间</td><td>任课教师1</td><td>任课教师2</td><td>任课教师3</td><td>任课教师4</td><td>上课教室</td><td>课程容量</td><td>选课人数</td><td><font color='blue'>是否选择</font></td><td><font color='blue'>选择任课教师</font></td></tr></thead>");
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
            else
            {
                show_error("找不到这个学号对应的用户信息");
                return 0;
            }
            mysql_free_result(result);
        }
        else if (strcmp(action, "edit_submit")==0)
        {
            char code[256]={0}, username[256]={0}, password[256]={0}, course[2048]={0}, selected_course[2048]={0}, teacher[2048]={0}, teachers[2048]={0};
            
            cgiFormString("code",code,sizeof(code));
            cgiFormString("username",username,sizeof(username));
            cgiFormString("password",password,sizeof(password));
            
            MYSQL_RES *result_course = executeQuery("select code,time,capacity,num from T_Courses", host, user, pass, db);
            MYSQL_ROW row_course;
            char times[50][256];
            int count=1;
            //课程信息
            while(row_course=mysql_fetch_row(result_course))
            {
                char radio[20]={0};
                char tmp[10]={0};
                sprintf(radio,"course%s",row_course[0]);
                if(cgiFormString(radio,tmp,sizeof(tmp))==cgiFormSuccess);
                {
                    if(strcmp(tmp,"")!=0)
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
                            sprintf(error,"课程编号为%s的课程与已选课程时间冲突",row_course[0]);
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
            char sql[4096]={0};
            sprintf(sql,"update T_Users set username='%s',password='%s',selected_course='%s',teachers='%s' where code='%s'",\
                        username,password,selected_course,teachers,code);
            executeNonQuery(sql, host, user, pass, db);
            cgiHeaderLocation("admin_manage.cgi?mod=student&action=list");
            mysql_free_result(result_course);
            return 0;
        }
        else if (strcmp(action, "delete")==0)
        {
            char code[256]={0};

            if(cgiFormString("code",code,sizeof(code))!=cgiFormSuccess)
            {
                show_error("请提供要删除的用户学号！");
                return 0;
            }
            MYSQL_RES *result;
            MYSQL_ROW row;
            char sql[4096] = {0};
            sprintf(sql, "select code,username,password,score,selected_course,num_major,is_enough,teachers from T_Users where code=%s", code);
            result = executeQuery(sql, host, user, pass, db);
            // 因为code为主键，所以至多有一条记录
            if(row=mysql_fetch_row(result))
            {
                cgiHeaderContentType("text/html;charset=utf-8");
                fprintf(cgiOut,"<html><head><title>用户修改</title></head><body>");
                fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>%s号用户原始数据</strong></div><tr style='background-color:#FFCC99;text-align:center;'>",row[0]);
                fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>密码</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>主修是否已修满</td><td>任课教师</td></tr></thead>");
                fprintf(cgiOut,"<tbody style='text-align:center'>");
                fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",row[0],row[1],row[2],row[3],row[4],row[3],row[6],row[7]);
                fprintf(cgiOut,"</tbody>");
                fprintf(cgiOut,"</table>");
                fprintf(cgiOut, "<form action='admin_manage.cgi' method='post'>");
                fprintf(cgiOut, "<input type='hidden' name='mod' value='student'/>");
                fprintf(cgiOut, "<input type='hidden' name='action' value='delete_submit'/>");
                fprintf(cgiOut, "<input type='hidden' name='code' value='%s'/>",row[0]);
                fprintf(cgiOut, "<font color='red'>你确定要删除吗？（删除操作是不可逆的）</font>");
                fprintf(cgiOut,"<input type='radio' name='delete_or_not' value='yes' id='delete_yes'/>是</label>");
                fprintf(cgiOut,"<input type='radio' name='delete_or_not' value='no' id='delete_no'/>否</label>");
                fprintf(cgiOut,"<input type='submit' name='btnDelete' value='确定'/>");
                fprintf(cgiOut, "</form>");
                fprintf(cgiOut,"</body></html>");
                mysql_free_result(result);
                return 0;
            }
            else
            {
                show_error("找不到这个学号对应的用户信息");
                return 0;
            }
        }
        else if (strcmp(action, "delete_submit")==0)
        {
            char sql[4096]={0}, code[256]={0}, judge[256]={0};
            
            cgiFormString("code",code,sizeof(code));
            if(cgiFormString("delete_or_not",judge,sizeof(judge))!=cgiFormSuccess)
            {
                show_error("请确认是否删除！");
                return 0;
            }
            if(strcmp(judge,"yes")==0)
            {
                sprintf(sql,"delete from T_Users where code='%s'", code);
                executeNonQuery(sql, host, user, pass, db);
            }
            cgiHeaderLocation("admin_manage.cgi?mod=student&action=list");
            return 0;
        }
        else if (strcmp(action, "find")==0)
        {
            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<html><head><title>用户查找</title></head><body>");
            fprintf(cgiOut, "<form action='admin_manage.cgi' method='post'>");
            fprintf(cgiOut, "<input type='hidden' name='mod' value='student'/>");
            fprintf(cgiOut, "<input type='hidden' name='action' value='find_submit'/>");
            fprintf(cgiOut, "<font color='blue'>请选择查找的字段：</font>");
            fprintf(cgiOut,"<select name='find'>");
            fprintf(cgiOut,"<option value='code'>学号</option>");
            fprintf(cgiOut,"<option value='username'>用户名</option>");
            fprintf(cgiOut,"</select>");
            fprintf(cgiOut,"&nbsp;&nbsp;&nbsp;<input type='text' name='value'/>");
            fprintf(cgiOut,"<input type='submit' name='btnFind' value='查找'/>");
            fprintf(cgiOut, "</form>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>用户列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>主修是否已修满</td><td>任课教师</td></tr></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            //显示数据
            MYSQL_RES *result = executeQuery("select code,username,score,selected_course,num_major,is_enough,teachers from T_Users", host, user, pass, db);
            MYSQL_ROW row;
            while(row=mysql_fetch_row(result))
            {
                if(strcmp(row[1],"admin")!=0)
                {
                    fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",
                        row[0],row[1],row[2],row[3],row[4],row[5],row[6]);
                }
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
            sprintf(sql,"SELECT * FROM T_Users WHERE LOCATE('%s',%s)>0",value,select);
            MYSQL_RES *result = executeQuery(sql, host, user, pass, db);
            MYSQL_ROW row;

            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<html><head><title>查找结果</title></head><body>");
            fprintf(cgiOut,"<a href='admin_manage.cgi?mod=student&action=list'>返回用户列表</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut,"<a href='admin_manage.cgi?mod=student&action=find'>返回用户查找</a>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>");
            if(strcmp(select,"code")==0){fprintf(cgiOut,"学号");}
            else if(strcmp(select,"username")==0){fprintf(cgiOut,"用户名");}
            fprintf(cgiOut,"查找结果</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>主修是否已修满</td></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            while(row=mysql_fetch_row(result))
            {
                if(strcmp(row[1],"admin")!=0)
                fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",\
                    row[0],row[1],row[3],row[4],row[5],row[6]);
            }
            mysql_free_result(result);
            return 0;
        }
        else if (strcmp(action, "sort")==0)
        {
            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<html><head><title>用户排序</title></head><body>");
            fprintf(cgiOut, "<form action='admin_manage.cgi' method='post'>");
            fprintf(cgiOut, "<input type='hidden' name='mod' value='student'/>");
            fprintf(cgiOut, "<input type='hidden' name='action' value='sort_submit'/>");
            fprintf(cgiOut, "<font color='blue'>请选择排序的字段：</font>");
            fprintf(cgiOut,"<select name='sort'>");
            fprintf(cgiOut,"<option value='code'>学号</option>");
            fprintf(cgiOut,"<option value='username'>用户名</option>");
            fprintf(cgiOut,"<option value='score'>已选课程学分</option>");
            fprintf(cgiOut,"<option value='selected_course'>已选课程</option>");
            fprintf(cgiOut,"<option value='num_course'>已选主修课门数</option>");
            fprintf(cgiOut,"<option value='is_enough'>主修是否已修满</option>");
            fprintf(cgiOut,"<option value='teachers'>任课教师</option>");
            fprintf(cgiOut,"</select>");
            fprintf(cgiOut,"<input type='radio' name='order' value='ASC' id='order_asc' checked='true'/>升序</label>");
            fprintf(cgiOut,"<input type='radio' name='order' value='DESC' id='order_desc'/>降序</label>");
            fprintf(cgiOut,"&nbsp;&nbsp;&nbsp;<input type='submit' name='btnSort' value='排序'/>");
            fprintf(cgiOut, "</form>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>课程列表</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>主修是否已修满</td><td>任课教师</td></tr></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            //显示数据
            MYSQL_RES *result = executeQuery("select code,username,score,selected_course,num_major,is_enough,teachers from T_Users", host, user, pass, db);
            MYSQL_ROW row;
            while(row=mysql_fetch_row(result))
            {
                if(strcmp(row[1],"admin")!=0)
                {
                    fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",
                        row[0],row[1],row[2],row[3],row[4],row[5],row[6]);
                }
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
            sprintf(sql,"SELECT * from T_Users ORDER BY %s %s",select,order);
            MYSQL_RES *result = executeQuery(sql, host, user, pass, db);
            MYSQL_ROW row;

            cgiHeaderContentType("text/html;charset=utf-8");
            fprintf(cgiOut,"<html><head><title>排序结果</title></head><body>");
            fprintf(cgiOut,"<a href='admin_manage.cgi?mod=student&action=list'>返回用户列表</a>&nbsp;&nbsp;&nbsp;");
            fprintf(cgiOut,"<a href='admin_manage.cgi?mod=student&action=sort'>返回用户排序</a>");
            fprintf(cgiOut,"<table border='1' class='dataframe' style='width:80%'><thead><div style='text-align:center;width:78.18%;padding: 8px; line-height: 1.42857; vertical-align: top; border-top-width: 1px; border-top-color: rgb(221, 221, 221); background-color: #3399CC;color:#fff'><strong>");
            if(strcmp(select,"code")==0){fprintf(cgiOut,"学号");}
            else if(strcmp(select,"username")==0){fprintf(cgiOut,"用户名");}
            else if(strcmp(select,"score")==0){fprintf(cgiOut,"已选课程学分");}
            else if(strcmp(select,"selected_course")==0){fprintf(cgiOut,"已选课程");}
            else if(strcmp(select,"num_major")==0){fprintf(cgiOut,"已选主修课门数");}
            else if(strcmp(select,"is_enough")==0){fprintf(cgiOut,"主修是否已修满");}
            else if(strcmp(select,"teachers")==0){fprintf(cgiOut,"任课教师");}
            if(strcmp(order,"ASC")==0){fprintf(cgiOut,"升序");}
            else if(strcmp(order,"DESC")==0){fprintf(cgiOut,"降序");}
            fprintf(cgiOut,"排序结果</strong></div><tr style='background-color:#FFCC99;text-align:center;'>");
            fprintf(cgiOut,"<tr><td>学号</td><td>用户名</td><td>已选课程学分</td><td>已选课程</td><td>已选主修课门数</td><td>主修是否已修满</td><td>任课教师</td></tr></thead>");
            fprintf(cgiOut,"<tbody style='text-align:center'>");
            while(row=mysql_fetch_row(result))
            {
                if(strcmp(row[1],"admin")!=0)
                {
                    fprintf(cgiOut, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",
                        row[0],row[2],row[3],row[4],row[5],row[6],row[7]);
                }
            }
            mysql_free_result(result);
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