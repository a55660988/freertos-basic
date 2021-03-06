#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"

typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void _command(int, char **);
void pwd_command(int, char **);
void cd_command(int, char **);
void new_command(int, char **);

char pwd[1024] = "/romfs/";

#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function"),
	MKCL(pwd, "Show the current path"),
	MKCL(cd, "Change path"),
	MKCL(new, "Create new task"),
	MKCL(, ""),
};

int i_fib(int num){
    int i=0;
    int x=0, y=1, z=1;
    for(i=0; i<num; i++){
       z = x + y;
       y = x;
       x = z;
    }
    return z;
}
int r_fib(int num){
    if(num<=0){
        return 0;
    }else if(num == 1){
        return 1;
    }else{
        return r_fib(num-1) + r_fib(num-2);
    }
}
int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

	return count;
}

void ls_command(int n, char *argv[]){
    fio_printf(1,"\r\n"); 
    int dir;
    if(n == 0){
        dir = fs_opendir("");
    }else if(n == 1){
        dir = fs_opendir(argv[1]);
        //if(dir == )
    }else{
        fio_printf(1, "Too many argument!\r\n");
        return;
    }
(void)dir;   // Use dir
}

int filedump(const char *filename){
	char buf[128];

	int fd=fs_open(filename, 0, O_RDONLY);

	if( fd == -2 || fd == -1)
		return fd;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0){
		fio_write(1, buf, count);
    }
	
    fio_printf(1, "\r");

	fio_close(fd);
	return 1;
}

void ps_command(int n, char *argv[]){
	signed char buf[1024];
	vTaskList(buf);
        fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
        fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	
}

void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");
		return;
	}

    int dump_status = filedump(argv[1]);
	if(dump_status == -1){
		fio_printf(2, "\r\n%s : no such file or directory.\r\n", argv[1]);
    }else if(dump_status == -2){
		fio_printf(2, "\r\nFile system not registered.\r\n", argv[1]);
    }
}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

    int dump_status = filedump(buf);
	if(dump_status < 0)
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i = 0;i < sizeof(cl)/sizeof(cl[0]) - 1; ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}

void test_command(int n, char *argv[]) {
    int handle;
    int error;

    fio_printf(1, "\r\n");
    
    handle = host_action(SYS_SYSTEM, "mkdir -p output");
    handle = host_action(SYS_SYSTEM, "touch output/syslog");

    handle = host_action(SYS_OPEN, "output/syslog", 8);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n\r");
        return;
    }

    char *buffer = "Test host_write function which can write data to output/syslog\n";
    error = host_action(SYS_WRITE, handle, (void *)buffer, strlen(buffer));
    if(error != 0) {
        fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
        host_action(SYS_CLOSE, handle);
        return;
    }

    if(n > 1){
        if(strcmp(argv[1], "fib") == 0){
            int num=0, i;
            // cannot use atoi, write some code like atoi
            for(i = 0; argv[3][i]!='\0'; i++){
                num = num*10 + argv[3][i] - '0';
            }
            if(strcmp(argv[2], "Iterative") == 0){
                fio_printf(1, "fibonacci in iterative method: fib(%d) = %d\r\n", num, i_fib(num));
            }else if(strcmp(argv[2], "Recursive") == 0){
                fio_printf(1, "fibonacci in recursive method: fib(%d) = %d\r\n", num, r_fib(num));
            }else{
                fio_printf(1, "Error!! Please type Iterative or Recursive!\r\n");
            }
        }
    }

    host_action(SYS_CLOSE, handle);
}

void _command(int n, char *argv[]){
    (void)n; (void)argv;
    fio_printf(1, "\r\n");
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}

void pwd_command(int n, char *argv[]){
	if(n>1){
        	fio_printf(1, "Too many argument!\r\n");
		fio_printf(1, "\r\nUsage: pwd\r\n");
	}else{
		fio_printf(1, "\r\n%s\r\n", pwd);
	}
}
void cd_command(int n, char *argv[]){//do it just change pwd arg
    fio_printf(1,"\r\n"); 
    if(n == 1){
        fio_printf(2, "\r\nUsage: cd path\r\n");
    }else if(n == 2){
        if(strcmp(argv[1], "~") == 0 || strcmp(argv[1], "/")==0){
            strcpy(pwd, "/romfs/");
            fio_printf(1, "\r\n%s\r\n", pwd);
        }else if(strcmp(argv[1], "..") == 0){
            if(strcmp(pwd, "/romfs/") == 0){
                fio_printf(1, "\r\n%s\r\n", pwd);
            }else{
                char* pch;
                char pwd_temp[1024];
                strcpy(pwd_temp, pwd);
                int last=0, secondlast=0;
                last = strlen(pwd_temp)-2; // -2 for '\0'
                pch=strchr(pwd_temp, '/');
                while(pch!=NULL){
                    if((pch-pwd_temp+1)!=last){
                        secondlast = pch-pwd_temp+1;
                    }else{
                        pch=strchr(pwd_temp+1, '/');
                    }
                }
                //after find secondlast slash
                strncpy(pwd, pwd_temp, secondlast); 
            }
        }else{//other folder, need to check dir
            char path[1024];
            //check first / or not /
            if(strchr(argv[1], '/')!=0){// no / at first, not from /romfs/
                strcpy(path, pwd);
                strcat(path, argv[1]);
                if(fs_checkdir(path)==1){
                    strcpy(pwd, path);
                    fio_printf(1, "\r\n%s\r\n", pwd);
                }else{
                    fio_printf(2, "\r\nerror\r\n");
                }
            }else{ //from romfs
                strcpy(path, "/romfs/");
                strcat(path, argv[1]);
                if(fs_checkdir(path)==1){
                    strcpy(pwd, path);
                    fio_printf(1, "\r\n%s\r\n", pwd);
                }else{
                    fio_printf(2, "\r\nerror\r\n");
                }
            }
        }
    }else{
        fio_printf(1, "Too many argument!\r\n");
        return;
    }
}
// refer from command_prompt
void new_task(void *pvParameters){
    //fio_printf(1, "\r\nWe are going to create new task!!\r\n");
    while(1);
}


void new_command(int n, char *argv[]){
    if(xTaskCreate(new_task, (signed portCHAR *) "NEWTASK", 512, NULL, tskIDLE_PRIORITY + 1, NULL)!=-1){
    //+1? +2?
    fio_printf(1, "\r\nWe are going to create new task!!\r\n");
    }else{
        fio_printf(1, "\r\nNew task create unsuccessfully!!\r\n");
        
    }

}
