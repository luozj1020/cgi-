#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

void write_data(char *filename, char *host, char *user, char *pass, char *db)
{
    FILE *file;
    file = fopen(filename,"w");
    fclose(file);
    file = fopen(filename,"w+");
    MYSQL_RES *result = executeQuery("select code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num from T_Courses", host, user, pass, db);
    MYSQL_ROW row;
    while(row=mysql_fetch_row(result))
    {  
        fprintf(file,"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n",row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11]);
    }
    fclose(file);
}

int main()
{
    char *host = "localhost"; //ip地址
    char *user = "root";      //数据库用户名
    char *pass = "";          //自己数据库密码
    char *db   = "study";     //表名
    
    // 生成课程
    char sql[4096]={0};
    int count=0;
    int i, j, k;
    int num_teacher, flag, score, capacity;
    char code[10]={0}, name[10]={0}, classification[10]={0}, t[20]={0}, classroom[3]={0}, tmp[10]={0};
    char teacher[4][10] = {"", "", "", ""}, day[7][10] = {"一", "二", "三", "四", "五", "六", "日"};

    executeNonQuery("truncate table T_Courses", host, user, pass, db); //清空数据库
    for (i=0;i<20;i++)
    {
        for(j=0;j<4;j++)
        {
            strcpy(teacher[j],"");
        }
        // 课程编码
        sprintf(code,"%4d",i+1);
        for(j=0;j<4;++j)
        {if(code[j]==' ')code[j]='0';}
        // 种类、学分
        flag = rand() % 2;
        if (flag && count<5){strcpy(classification,"主修"); score=4; count+=1;}
        else{strcpy(classification,"辅修"); score=2;}
        // 上课教室
        sprintf(tmp,"%2d",rand()%16+1);
        for(k=0;k<2;++k)
        {if(tmp[k]==' ')tmp[k]='0';}
        sprintf(classroom,"F%s",tmp);
        // 教室容量
        capacity = (rand() % 20 + 1) * 10;
        // 任课教师
        num_teacher = rand()%4+1;
        for(j=0;j<num_teacher;j++)
        {
          sprintf(tmp,"%2d",rand()%10+1);
          for(k=0;k<2;++k)
          {if(tmp[k]==' ')tmp[k]='0';}
          sprintf(teacher[j],"teacher%s",tmp);
        }

        sprintf(sql,"Insert into T_Courses(code,name,classification,score,time,teacher1,teacher2,teacher3,teacher4,classroom,capacity,num) \
                    values('%s','course%s', '%s', '%d', '星期%s/第%d节', '%s', '%s', '%s', '%s', '%s', %d, 0)",\
                    code,code,classification,score,day[rand()%7], rand()%9+1,teacher[0],teacher[1],teacher[2],teacher[3],classroom,capacity);
        printf("%s\n", sql);
        executeNonQuery(sql, host, user, pass, db);
        //break;
    }
    return 0;
}
//  gcc create_table.cpp -o create_table -lmysqlclient