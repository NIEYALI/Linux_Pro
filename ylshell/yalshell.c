/*************************************************************************
	> File Name: yalshell.c
	> Author: Yali Nie
	> Mail: yali.nie@miun.se 
	> Created Time: Mon Aug 30 11:24:27 2021
 ************************************************************************/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<ctype.h>
#include<sys/fcntl.h>
#include<sys/stat.h>

#define LEN 1024
#define CNT 10

char *trim(char *str){
    int head=0;
    int tail=strlen(str)-1;

    while(isspace(str[head]))
        head++;
    while(isspace(str[tail]))
        str[tail--]=0;
    return str+head;
}
void runcmd(char *buff){
    
        //exit(1);
        int redfd=-1;
        char *sep=NULL;
        if(strstr(buff,"<")){
            redfd=0;
            sep="<";
        }
        if(strstr(buff,">")){
            redfd=1;
            sep=">";
        }

        char *cmd=NULL;
        if(redfd!=-1){
            char *token=strtok(buff,sep);
            if(!token)
                exit(1);
            //printf("token:[%s]\n",token);
            cmd=token;
            token=strtok(NULL,sep);
            //printf("token::[%s]\n,token");
            token=trim(token); 

            int fd;
            if(redfd==0){
                fd=open(token,O_RDWR);
            } else{
                fd=open(token,O_RDWR | O_CREAT | O_TRUNC,0644);
            }
            
            if(fd<0){
                perror(token);
                exit(1);
                }
                //token=trim(token);
               // printf("open %s successfully ~\n",token);
                close(redfd);
                dup2(fd,redfd);
    
        } else{
            cmd=buff;
        }

        int i=0;
        char *argarr[CNT];
        char *tk=strtok(cmd," ");
        do{
            argarr[i++]=tk;
            tk=strtok(NULL," ");
        }while(tk);
        argarr[i]=tk;

        execvp(argarr[0],argarr);
        perror(argarr[0]);
        exit(1);
}
int main(void){
    char buff[LEN];
    signal(SIGTTOU,SIG_IGN);
    while(1){
        printf("input$");
        //scanf("%s",buff);

        fgets(buff,LEN,stdin);
        buff[strlen(buff)-1]=0;
        printf("cmd:[%s]\n",buff);

        if(!strcmp(buff,"exit")){
            printf("exit^_^\n");
            break;
        }

        //Create child processing
        //pid_t pid=fork();
       // if(pid<0){
         //   perror("fork");
         //   exit(1);
        //}
        //父进程等待子进程，继续下一轮
        //if(pid){
          //  wait(NULL);
            //continue;
        //
        //}
        //处理后台运行指令
        int bgidx=strlen(buff)-1;
        int isbgrun=0;
        while(isspace(buff[bgidx]))
            bgidx--;
        if(buff[bgidx]=='&'){
            isbgrun=1;
            buff[bgidx]=0;
        }
        printf("isbgrun=[%d]\n",isbgrun);
        int i=0;
        char *cmdarr[CNT];
        char *tk=strtok(buff,"|");

        while(tk){
            cmdarr[i++]=tk;
            tk=strtok(NULL,"|");
        }
        cmdarr[i]=tk;
        pid_t npgid=0;
        pid_t oldtcpid=0;
       //没有管道时
        if(i==1){
            pid_t pid=fork();
            if(pid<0){
                perror("fork");
                exit(1);
            }
            if(pid){
                setpgid(pid,0);
                if(!isbgrun){
                    oldtcpid=tcgetpgrp(1);
                    tcsetpgrp(1,pid);
                }
                if(!isbgrun)
                    wait(NULL);
                if(!isbgrun)
                    tcsetpgrp(1,oldtcpid);
                continue;
            }
            runcmd(cmdarr[0]);
        }
        int pparr[CNT-1][2];
        //创建所有管道

        int j;
        for(j=0;j<i-1;j++){
            pipe(pparr[j]);
        }
        for(j=0;j<i;j++){
            pid_t pid=fork();
            if(pid<0){
                perror("fork");
                exit(1);
            }

            //子进程操作
            if(pid==0){
                //重定向相应标准输入输出
                if(j==0){
                    dup2(pparr[j][1],1);

                }else if(j==i-1){
                    dup2(pparr[j-1][0],0);
                    
                }else{
                    dup2(pparr[j][1],1);
                    dup2(pparr[j-1][0],0);

                }
                //关闭所有无用的文件描述符:3 4 5 6  7 8...
                int k;
                for(k=0;k<i-1;k++){
                    close(pparr[k][0]);
                    close(pparr[k][1]);

                }
                runcmd(cmdarr[j]);
            }else{

                //父进程操作
                //第一个孩子
            if(j==0)
                 npgid=pid;
            setpgid(pid,npgid);
            /*********************/
            /*********************/
            //设置前后进程组
                if(!isbgrun && j==0){ 
                   oldtcpid=tcgetpgrp(1); 
                   tcsetpgrp(1,npgid);
                }
            } 

        
        }
        
        int k;
        for(k=0;k<i-1;k++){
            close(pparr[k][0]);
            close(pparr[k][1]);
        }
        if(!isbgrun){
            for(j=0;j<i;j++)
                 wait(NULL);
        }
    
        //signal(SIGTTOU,SIG_IGN);
        if(!isbgrun)
             tcsetpgrp(1,oldtcpid);
        /*
    //创建管道
        int pp[2];
        if(pipe(pp)<0){
            perror("pipe");
            exit(1);
        }
        //创建第一个管道
        pid_t pid=fork();
        if(pid<0){
            perror("fork");
            exit(1);
        }
        if(pid==0){
            //close(pp[0]);
            dup2(pp[1],1);
            close(pp[0]);
            close(pp[1]);
            runcmd(cmdarr[0]);
        }
        //Child processing
        //execl(cmd);
        
        //创建第二个管道
        pid=fork();
        if(pid<0){
            perror("fork");
            exit(1);
        }
        if(pid==0){
            //close(pp[1]);
            dup2(pp[0],0);
            close(pp[0]);
            close(pp[1]);
            runcmd(cmdarr[1]);
        }
        close(pp[0]);
        close(pp[1]);

        wait(NULL);
        wait(NULL);*/
        }

    return 0;
}
