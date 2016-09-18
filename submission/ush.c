#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include "parse.h"

typedef enum {not_in_use, used_left, used_right} IN_USE;

typedef enum {cmd_cd, cmd_echo, cmd_logout, cmd_nice, 
	cmd_pwd, cmd_setenv, cmd_unsetenv, cmd_where, 
	cmd_other, cmd_invalid, cmd_end} CMD_TYPE;

struct my_pipe{
	int buffPipe[2];
	IN_USE in_use;
};

struct my_pipe Pipe1,Pipe2;

int priority_to_set=0;

int input_file, output_file, default_std_out,default_std_in,default_std_err;
int is_done = 0;



void sig_handler(int signo)
{
  if(signo == SIGQUIT || SIGINT){
  	// printf("\r");
  	// fflush(stdout);
  	return;
  }

  if(signo == SIGTERM){
  	is_done = 1 ;
  }
}

CMD_TYPE command_type(Cmd c){
	
	if(strcmp(c->args[0],"cd") == 0) return cmd_cd;

	else if(strcmp(c->args[0],"echo") == 0) return cmd_echo;
	
	else if(strcmp(c->args[0],"logout") == 0) return cmd_logout;

	else if(strcmp(c->args[0],"nice") == 0)	return cmd_nice;

	else if(strcmp(c->args[0],"pwd") == 0) return cmd_pwd;

	else if(strcmp(c->args[0],"setenv") == 0) return cmd_setenv;

	else if(strcmp(c->args[0],"unsetenv") == 0)	return cmd_unsetenv;

	else if(strcmp(c->args[0],"where") == 0)	return cmd_where;

	else if(strcmp(c->args[0],"end") == 0)	return cmd_end;

	else  return cmd_other;
		
}

void create_pipe(void){
	if(Pipe1.in_use == not_in_use){
		pipe(Pipe1.buffPipe);
		Pipe1.in_use = used_right;
	}

	else {

		pipe(Pipe2.buffPipe);
		Pipe2.in_use = used_right;
	}
	
}

void change_pipe(){
	if(Pipe1.in_use != not_in_use){

		if(Pipe1.in_use == used_right) Pipe1.in_use = used_left;
		else Pipe1.in_use = not_in_use;
	}

	if(Pipe2.in_use != not_in_use){
		if(Pipe2.in_use == used_right) Pipe2.in_use = used_left;
		else Pipe2.in_use = not_in_use;
	}

}

void close_pipe(Cmd c){
	if(c->in == Tpipe || c->in == TpipeErr){
  			
  			if(Pipe1.in_use == not_in_use) close(Pipe1.buffPipe[0]);

  			else close(Pipe2.buffPipe[0]);
  		}

	if(c->next!=NULL)
	{
  		if(c->out == Tpipe ){
    		
    		if(Pipe1.in_use == used_left) close(Pipe1.buffPipe[1]);
    		else close(Pipe2.buffPipe[1]);
    		dup2(default_std_out,1);
  		}
  		if(c->out == TpipeErr){
  			
    		if(Pipe1.in_use == used_left) close(Pipe1.buffPipe[1]);
    		else close(Pipe2.buffPipe[1]);
    		dup2(default_std_out,1);
  			dup2(default_std_err,2);
  		}	
	}
}


void set_input_output(Cmd c){

	if(c->in == Tin){
		//fflush(stdout);
		//printf("In c->in Tin\n");
		if((input_file = open(c->infile, O_RDONLY)) == -1){
			perror(c->infile);
			fflush(stderr);
		}
		dup2(input_file,0);
	}

	if(c->out == Tout){
		//printf("In c->out Tout\n");
		if((output_file = open(c->outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH )) == -1){
			perror(c->infile);
			fflush(stderr);
		}
		dup2(output_file,1);
	}

	if(c->out == Tapp){
		//printf("In c->out Tapp\n");
		if((output_file = open(c->outfile, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == -1){
			perror(c->outfile);
			fflush(stderr);
		}
		dup2(output_file,1);
	}

	if(c->out == ToutErr){
		//printf("In c->out ToutErr\n");
		if((output_file = open(c->outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH )) == -1){
			perror(c->outfile);
			fflush(stderr);
		}
		dup2(output_file,1);
		dup2(output_file,2);
	}

	if(c->out == TappErr){
		//printf("In c->out TappErr\n");
		if((output_file = open(c->outfile, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == -1){
			perror(c->outfile);
			fflush(stderr);
		}
		dup2(output_file,1);
		dup2(output_file,2);
	}


	if(c->in == Tpipe || c->in == TpipeErr){
		
		if(Pipe1.in_use == used_left){
			close(Pipe1.buffPipe[1]);
			dup2(Pipe1.buffPipe[0],0);
			close(Pipe1.buffPipe[0]);

		}

		else{
			close(Pipe2.buffPipe[1]);
			dup2(Pipe2.buffPipe[0],0);
			close(Pipe2.buffPipe[0]);
			
		}
		
	}

	if(c->out == TpipeErr){
		if(Pipe1.in_use == used_right){
			close(Pipe1.buffPipe[0]);
			dup2(Pipe1.buffPipe[1],1);
			dup2(Pipe1.buffPipe[1],2);
			
		}
		else {
			close(Pipe2.buffPipe[0]);
			dup2(Pipe2.buffPipe[1],1);
			dup2(Pipe2.buffPipe[1],2);
		}
	}

	if(c->out == Tpipe){
				
		if(Pipe1.in_use == used_right){
			close(Pipe1.buffPipe[0]);
			dup2(Pipe1.buffPipe[1],1);
		}
		else {
			close(Pipe2.buffPipe[0]);
			dup2(Pipe2.buffPipe[1],1);
		}
	}

	if(c->out == Tnil){
		//printf("In c->out Tnil\n");
		dup2(default_std_out,1);
	}
}

void close_files(Cmd c){
	if(c->in == Tin){
		//dup2(default_std_in,0);
		close(input_file);
	}

	if(c->out == Tout || c->out == ToutErr || c->out == Tapp || c->out == TappErr){
		dup2(default_std_out,1);
		close(output_file);
	}
}

char * get_environ(char *name){
	//printf("%s ",name);
	return getenv(name);

}

int execute_end(Cmd c){
	printf("\n");
	exit(0);
}

int execute(Cmd c){
	
	int pid;
	int status;
	int cd_return_value;
	if(c->out == Tpipe || c->out == TpipeErr) create_pipe();

	pid = fork();
	if(pid == 0){

		setpriority(PRIO_PROCESS, getpid(), priority_to_set);
		set_input_output(c);
		if(execvp(*(c->args),c->args) == -1){
			perror(*(c->args));
			fflush(stderr);
			exit(-1);
		}
		exit(0);
	}

	else{
		//waitpid(pid,&status);
		wait(&status);
		//printf("Status %d\n",status);
		fflush(stdout);
		fflush(stderr);
		change_pipe();
		close_pipe(c);
		//close_files(c);
		if(status != 0){
			if(c->out == Tpipe || c->out == TpipeErr){
				if(Pipe1.in_use == used_left){
					Pipe1.in_use = not_in_use;
					close(Pipe1.buffPipe[0]);
				}
				else if(Pipe2.in_use == used_left){
					Pipe2.in_use = not_in_use;
					close(Pipe2.buffPipe[0]);
				}
			}
			//printf("I am here\n");
			return -1;
		}
		return 0;
	}
}

int execute_cd(Cmd c){

	char *name = "HOME";
	char *path;
	int cd_return_value,pid;
	if(c->nargs == 1){

		c->args[1] = get_environ(name);
	}

	
	if(c->out != Tpipe && c->out != TpipeErr){
		
		set_input_output(c);	
		
		if(c->nargs > 2){
			fprintf(stderr, "%s: too many arguments\n",c->args[0] );
			fflush(stderr);
		}

		else{
			cd_return_value = chdir(c->args[1]);
			if(cd_return_value != 0){
				perror(c->args[1]);
				fflush(stderr);	
			} 
		}
		
		if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);

		close_files(c);
		return 0;
	}

	else {
		create_pipe();
		pid = fork();
	    if( pid == 0 ) {
	    	setpriority(PRIO_PROCESS, getpid(), getpriority(PRIO_PROCESS,getppid()));
	    	set_input_output(c);

	    	if(c->nargs > 2){
				fprintf(stderr, "%s: too many arguments\n",c->args[0] );
				fflush(stderr);
			}

			else{
				cd_return_value = chdir(c->args[1]);
				if(cd_return_value != 0){
					perror(c->args[1]);
					fflush(stderr);
				} 
			}
			close_files(c);
			close_pipe(c);
			exit(0);
			
	    } 
	    else {
	        wait();
	        
	        change_pipe();
	        close_pipe(c);
	        close_files(c);
	        return 0;
	    }
	}
	return 0;	
}

int execute_echo(Cmd c){
	int i=0,pid;
	char *name;
	char *path;

	if(c->out != Tpipe && c->out != TpipeErr){
		set_input_output(c);
		for(i=1; i<c->nargs;i++){
			if(*(c->args[i]) == '$'){
				path = get_environ(c->args[i]+1);
				if(path != NULL)
					fprintf(stdout, "%s ",path);
				else{
					fprintf(stderr, "%s: Undefined variable\n",c->args[i]+1);
				}
			}
			else{
				printf( "%s ",c->args[i]);
			}
		}

		printf("\n");
		fflush(stdout);
		close_files(c);
		if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);
		return 0;
	}
	else{
		create_pipe();
		pid = fork();

	    if( pid == 0 ) {
	    	setpriority(PRIO_PROCESS, getpid(), getpriority(PRIO_PROCESS,getppid()));
	    	set_input_output(c);
	    	for(i=1; i<c->nargs;i++){
				if(*(c->args[i]) == '$'){
					path = get_environ(c->args[i]+1);
					if(path != NULL)
						printf("%s ",path);
					else{
						fprintf(stderr, "%s: Undefined variable\n",c->args[i]+1 );
					}
				}
				else{
					printf("%s ",c->args[i]);
				}
			}
			printf("\n");
			fflush(stdout);
			close_files(c);
			close_pipe(c);
			exit(0);

	    }
	    else{
	    	
			wait();
        
	        change_pipe();
	        close_pipe(c);
	        return 1;
	    }

	}
}

int execute_logout(Cmd c){

	int pid;

	if(c->out != Tpipe && c->out != TpipeErr){
		set_input_output(c);

		if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);

		close_files(c);
		exit(0);
	}
	else{
		
		create_pipe();
		pid = fork();
		if( pid == 0 ) {
	    	//printf("Inside fork\n");
	    	setpriority(PRIO_PROCESS, getpid(), getpriority(PRIO_PROCESS,getppid()));
	    	set_input_output(c);

	    	close_files(c);
			close_pipe(c);
			exit(0);
	    }
	    else{
			wait();
        
	        change_pipe();
	        close_pipe(c);
	        return 1;
	    }
	}
}

int is_number(char args[]){
	int i=0;
  	if(args[0] == '+' || args[0] == '-') i++;
  	for(;args[i] != '\0' ; i++){
  		if(!isdigit(args[i])){
  			//printf("Not number\n");
  			return 0;
  		}
  	}
  	//printf("Number \n");
  	return 1;
}

int execute_nice(Cmd c){
	
	int which = PRIO_PROCESS;
	CMD_TYPE cmd_type;
	id_t pid, pid1, priority, old_priority, ret,i,is_num;
	Cmd temp;

	if(c->nargs >= 2 ){
		is_num = is_number(c->args[1]);
		if((is_num && c->nargs > 2) || !(is_num) ){
			pid = getpid();
			temp = ckmalloc(sizeof(*temp));
			temp->exec = c->exec;
			temp->in = c->in;
			temp->out = c->out;
			temp->infile = c->infile;
			temp->outfile = temp->outfile;
			if(is_num)
				temp->nargs = c->nargs - 2;
			else
				temp->nargs = c->nargs - 1;
			temp->maxargs = c->maxargs;
			temp->next = c->next;
			temp->args = ckmalloc(c->maxargs*sizeof(char*));

			if( is_num )
				for(i=0;i < (c->nargs-2);i++)			
					*(temp->args+i) = *(c->args + 2 +i);

			else
				for(i=0;i < (c->nargs-1);i++)			
					*(temp->args+i) = *(c->args + 1 +i);

			cmd_type = command_type(temp);

			if(is_num){
				if( (cmd_type != cmd_other) ) {
					old_priority = getpriority(which, pid);
					ret = setpriority(which, pid, atoi(c->args[1]));
				}
				else {
					priority_to_set = atoi(c->args[1]);
				}
			}

			else{
				if( (cmd_type != cmd_other) ) {
					old_priority = getpriority(which, pid);
					ret = setpriority(which, pid, 4);
				}
				else {
					priority_to_set = 4;
				}
			}
			begin_execution(cmd_type ,temp);
			// printf("I am here\n");
			fflush(stdout);
			if( (cmd_type != cmd_other) ) setpriority(which,pid,old_priority);
			else priority_to_set = 0;
			return;	
		}
		
	}

	if(c->out != Tpipe && c->out != TpipeErr){
		set_input_output(c);
		if(c->nargs == 1){
			priority = 4;
			pid = getpid();
			ret = setpriority(which, pid, priority);
			if(ret != 0) {
				perror("setpriority failed");
				fflush(stderr);
			}
			
		}

		else if(c->nargs ==2){
		 	priority = atoi(c->args[1]);
			ret = setpriority(which,pid,priority);
			if(ret != 0) {
				perror("setpriority failed");
				fflush(stderr);
			}
			
		}

		if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);
		close_files(c);
		
	}
	else{
		create_pipe();
		pid1 = fork();

	    if( pid1 == 0 ) {
	    	set_input_output(c);
			if(c->nargs == 1){
				priority = 4;
				pid = getpid();
				ret = setpriority(which, pid, priority);
				if(ret != 0) {
					perror("setpriority failed");
					fflush(stderr);
				}
				
			}

			else if(c->nargs ==2){
				pid = getpid();
			 	priority = atoi(c->args[1]);
				ret = setpriority(which,pid,priority);
				if(ret != 0) {
					perror("setpriority failed");
					fflush(stderr);
				}
				
			}


	    	close_files(c);
			close_pipe(c);
			exit(0);

	    }
	    else{
	    	wait();
        
	        change_pipe();
	        close_pipe(c);
	        return 0;
	    }
		
	}
}

int execute_pwd(Cmd c){
	char *buffer,*ptr;
	size_t size;
	int pid;

	size = pathconf(".", _PC_PATH_MAX);
	
    if(c->out != Tpipe && c->out != TpipeErr){
		set_input_output(c);
		if(c->nargs > 1) {
			fprintf(stderr, "pwd: ignoring arguments \r" );
			fflush(stderr);
		}

		if ((buffer = (char *)malloc((size_t)size)) != NULL)
    	ptr = getcwd(buffer, (size_t)size);
		if( ptr != NULL ) {
        	printf( "%s\n", ptr );
        	fflush(stdout);
    	}
    	else perror(buffer);
 
    	if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);

		close_files(c);
		return 0;
    }

    else{
    	create_pipe();
		pid = fork();
	    if( pid == 0 ) {
	    	setpriority(PRIO_PROCESS, getpid(), getpriority(PRIO_PROCESS,getppid()));
	    	set_input_output(c);

	    	if(c->nargs > 1) {
	    		fprintf(stderr, "pwd: ignoring arguments\n" );
	    		fflush(stderr);
	    	}

	    	if ((buffer = (char *)malloc((size_t)size)) != NULL)
    		ptr = getcwd(buffer, (size_t)size);
			if( ptr != NULL ) {
        		printf( "%s\n", ptr );
        		fflush(stdout);
    		}
    		else perror(buffer);

	    	close_files(c);
			close_pipe(c);
			exit(0);
	    } 
	    else {
	        wait();
	        
	        change_pipe();
	        close_pipe(c);
	        return 1;
	    }
    }
}

int execute_setenv(Cmd c){
	extern char **environ;
	char *value,*value2;
	int return_value=0,i=1,pid,j;
	char value1[100];
	char *s = *environ;
	char *t;
	int found;

	if(c->out != Tpipe && c->out != TpipeErr){
		set_input_output(c);
		if(c->args[1] == NULL){
			
	  		while(s) {
	    		fprintf(stdout, "%s\n", s);
	    		fflush(stdout);
	 			s = *(environ+i);
	 			i++;
	  		}
		}
	
		else {

			if(c->nargs == 3)
				value2 = malloc((strlen(c->args[1])+strlen(c->args[2])+2)*sizeof(char));

			else if(c->nargs == 2)
				value2 = malloc((strlen(c->args[1])+2)*sizeof(char));

			strcpy(value2,c->args[1]);
			strcat(value2,"=");
			if(c->nargs == 3) strcat(value2,c->args[2]);
			return_value =  putenv(value2);
			if(return_value != 0){
				perror(value1);
				fflush(stderr);
			}
		}

		if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);

		close_files(c);
		return return_value;
	}
	else{

		fflush(stdout);
		create_pipe();
		pid = fork();

	    if( pid == 0 ) {
	    	//printf("Inside fork\n");
	    	setpriority(PRIO_PROCESS, getpid(), getpriority(PRIO_PROCESS,getppid()));
	    	set_input_output(c);
	    	if(c->args[1] == NULL){
		
		  		while(s) {
		    		fprintf(stdout, "%s\n", s);
		    		fflush(stdout);
		 			s = *(environ+i);
		 			i++;
		  		}
			}
		
			else {

				if(c->nargs == 3)
					value2 = malloc((strlen(c->args[1])+strlen(c->args[2])+2)*sizeof(char));

				else if(c->nargs == 2)
					value2 = malloc((strlen(c->args[1])+2)*sizeof(char));

				strcpy(value2,c->args[1]);
				strcat(value2,"=");
				if(c->nargs == 3) strcat(value2,c->args[2]);
				return_value =  putenv(value2);
				if(return_value != 0){
					perror(value1);  
					fflush(stderr);		
				}
			}

			close_files(c);
			close_pipe(c);
			exit(0);
	    }
	    else{
			wait();
        
	        change_pipe();
	        close_pipe(c);
	        return 1;
	    }
	}
}

int execute_unsetenv(Cmd c){
	int return_value,pid,i;

	if(c->out != Tpipe && c->out != TpipeErr){
		set_input_output(c);
		if(c->args[1] == NULL){
			fprintf(stderr, "unsetenv: too few arguments\n");
			fflush(stderr);
		}

		else 
			for(i=1;i<c->nargs;i++)
				if((return_value = putenv(c->args[i])) != 0){
					perror(c->args[i]);
				}
		close_files(c);	

		if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);

		return EXIT_SUCCESS;

	}

	else{
		create_pipe();
		pid = fork();

	    if( pid == 0 ) {
	    	setpriority(PRIO_PROCESS, getpid(), getpriority(PRIO_PROCESS,getppid()));
	    	set_input_output(c);
	    	if(c->args[1] == NULL) {
	    		fprintf(stderr, "unsetenv: too few arguments\n");
	    		fflush(stderr);
	    	}

			else 
				for(i=1;i<c->nargs;i++)
					if((return_value = putenv(c->args[i])) != 0){
						perror(c->args[i]);
					}
			close_files(c);	

			if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);			
	    	exit(0);
	    }

	    else {
	    	wait();
        
	        change_pipe();
	        close_pipe(c);
	        return 1;
	    }
	}
}

/*Splitting function taken from stack overflow and modified according to need
http://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
*/
int split (char *str, char c, char ***arr, char *cmd)
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p,*q;
    char *t;
    int command_len = strlen(cmd);
    p = str;
    while (*p != '\0')
    {
        if (*p == c)
            count++;
        p++;
    }

    *arr = (char**) malloc(sizeof(char*) * count);
    if (*arr == NULL)
        exit(1);

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
        {
            (*arr)[i] = (char*) malloc( sizeof(char) * (token_len + command_len));
            if ((*arr)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*arr)[i] = (char*) malloc( sizeof(char) * (token_len +command_len) );
    if ((*arr)[i] == NULL)
        exit(1);

    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0')
    {
        if (*p != c && *p != '\0')
        {
            *t = *p;
            t++; 
        }
        else
        {
        	*t = '/';
        	t++;
        	q = cmd;
        	while(*q != '\0'){
        		*t = *q;
        		t++; q++;
        	}
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
    }
    *t = '/';
    t++;
    q = cmd;
    while(*q != '\0'){
    	*t = *q;
    	t++;q++;
    }
    *t = '\0';
    return count;
}

int execute_where(Cmd c){
	
	char **split_paths = NULL;
	int count,i,j;
	char *path = get_environ("PATH");
	CMD_TYPE cmd_type;
	int pid;

	if(c->out != Tpipe && c->out != TpipeErr){
		set_input_output(c);
		if(c->nargs == 1){
			fprintf(stderr, "usage: where <command>\n" );
			fflush(stderr);
		}
		else{
			for(j=1; j< c->nargs; j++){
				cmd_type = command_type(c);
				if(cmd_type != cmd_other){
					printf("%s is a shell builtin\n", c->args[j]);

				}
				count = split(path,':',&split_paths,c->args[j]);
				printf("%s: ",c->args[j]);
				for (i = 0; i < count; i++)
			        if(access(split_paths[i],X_OK) != -1 )
			        	printf("%s ", split_paths[i]);
			    printf("\n");
			    fflush(stdout);
			}
		}
		if(c->in == TpipeErr || c->in == Tpipe)	close_pipe(c);

		close_files(c);
		//exit(0);
	}
	else{
		
		create_pipe();
		pid = fork();
		if( pid == 0 ) {
	    	//printf("Inside fork\n");
	    	setpriority(PRIO_PROCESS, getpid(), getpriority(PRIO_PROCESS,getppid()));
	    	set_input_output(c);
	    	if(c->nargs == 1){
				fprintf(stderr, "usage: where <command>\n" );
				fflush(stderr);
			}
			else{
				for(j=1; j< c->nargs; j++){
					cmd_type = command_type(c);
					if(cmd_type != cmd_other){
						printf("%s is a shell builtin\n", c->args[j]);
					}
					count = split(path,':',&split_paths,c->args[j]);
					printf("%s: ",c->args[j]);
					for (i = 0; i < count; i++)
				        if(access(split_paths[i],X_OK) != -1 )
				        	printf("%s ", split_paths[i]);
				    printf("\n");
				    fflush(stdout);
				}
			}

	    	close_files(c);
			close_pipe(c);
			exit(0);
	    }
	    else{
			wait();
        
	        change_pipe();
	        close_pipe(c);
	        return 1;
	    }
	}
	return 0;
	
}

int begin_execution(CMD_TYPE cmd, Cmd c){
	switch(cmd){
		case cmd_cd : return execute_cd(c);

		case cmd_echo: return execute_echo(c);

		case cmd_logout: return execute_logout(c);

		case cmd_nice: return execute_nice(c);

		case cmd_pwd: return execute_pwd(c);

		case cmd_setenv: return execute_setenv(c);

		case cmd_unsetenv: return execute_unsetenv(c);

		case cmd_where: return execute_where(c);		

		case cmd_other: return execute(c);

		case cmd_end: return execute_end(c);
	}

}

static void prPipe(Pipe p){
	int return_value;
	CMD_TYPE cmd_type;

	Cmd c;
	int i=0;

	if ( p == NULL )
    	return;
    //printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
	Pipe1.in_use = not_in_use;
	Pipe2.in_use = not_in_use;
	
	//no_of_pipes = 0;
	for ( c = p->head; c != NULL; c = c->next ) {
		//printf("  Cmd #%d: ", ++i);
		cmd_type = command_type(c);
		return_value = begin_execution(cmd_type ,c);
		//printf("%d\n",return_value);
		if(return_value == -1)
			break;
	}
	//printf("End pipe\n");
	fflush(stdout);
	prPipe(p->next);
}

int main(int argc, char *argv[]){

	
	signal(SIGQUIT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	default_std_out = dup(1);
	default_std_in = dup(0);
	default_std_err = dup(2);
	Pipe p;
	char host[30] ;

	int input;
	char *home = get_environ("HOME");
	char *file = malloc(sizeof(char *)*(strlen(home)+8));
	strcpy(file,home);
	strcat(file,"/");
	strcat(file,".ushrc");

	if((input  = open(file, O_RDONLY)) != -1){
		//perror(file);
		dup2(input, 0);
		close(input);
		while( !is_done){
			p = parse();
			if(p == NULL){
				continue;
			}
			if(command_type(p->head) != cmd_end) {
				prPipe(p);
		    	freePipe(p);
			}
			else{
				is_done = 1;
			}
		}
		p = NULL;
		dup2(default_std_in,0);
	}	

	is_done = 0;
	gethostname(host,30);
	while ( !is_done ) {
		printf("%s%% ", host);
		fflush(stdout);
	    p = parse();
	    prPipe(p);
	    freePipe(p);
	}
}