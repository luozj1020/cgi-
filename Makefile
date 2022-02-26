all: login.cgi admin_manage.cgi student_manage.cgi
.PHONY: clean

login.cgi: login.cpp
	gcc login.cpp cgic.c -o login.cgi -lmysqlclient
	mkdir /usr/lib/cgi-bin/4
	cp -r *data* /tmp

admin_manage.cgi: admin_manage.cpp
	gcc admin_manage.cpp cgic.c -o admin_manage.cgi -lmysqlclient

student_manage.cgi: student_manage.cpp
	gcc student_manage.cpp cgic.c -o student_manage.cgi -lmysqlclient
	cp login* create_table* admin_manage* student_manage* cgic* /usr/lib/cgi-bin/4

clean:
	rm -rf login.cgi
	rm -rf admin_manage.cgi
	rm -rf student_manage.cgi
	rm -rf /usr/lib/cgi-bin/4
	


