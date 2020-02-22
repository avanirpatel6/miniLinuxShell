#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


static void commandBarrier(const int block);
static void clrZombies();
static char ** splitArguments(char * cmd, int * args_count);
static int isBackground(char **argv, int argc, int* bg);
static int isRedirect(char **argv, int argc, int* fd);
static int isFork(char * CMD);
static int isNewLine(char * cmdline, int len);
void sigHand(int rc);
void currentDirectory();

static unsigned int bgCounter = 0;
static int interrupt = 0;

int main(int argc, char *argv[]){
  
  FILE * input = stdin;

  signal(SIGINT, sigHand);

  if(argc == 2){
    input = fopen(argv[1], "r");
    if(input == NULL){
      perror("fopen");
      return 1;
    }

  }else if(argc > 2){
    fprintf(stderr, "Error: Only one parameter can be acepted\n");
    return 1;
  }

	char * cmdline = NULL;
  size_t cmdline_size = 0;

	while(interrupt == 0){

    clrZombies();

    if(input == stdin){
      currentDirectory();
      printf("$Avani > ");
    }

    ssize_t cmdline_len = getline(&cmdline, &cmdline_size, input);
    if(cmdline_len == -1){
      if(ferror(input)){
        perror("getline");
      }
      break;
    }

    cmdline_len = isNewLine(cmdline, cmdline_len);

    if(cmdline_len == 1){
      continue;
    }
    cmdline[--cmdline_len] = '\0';

    if(cmdline[0] == '#')
      continue;

		if(strcmp("quit", cmdline) == 0)
			break;
		else if(strcmp("barrier", cmdline) == 0){
			commandBarrier(1);
	  }else{
			isFork(cmdline);
		}
	}
	free(cmdline);

  if(input != stdin)
    fclose(input);

  commandBarrier(1);

	return 0;
}

static void commandBarrier(const int block){
    int i, status, finished=0;
    int wflag = (block == 1) ? 0 : WNOHANG;
    
    for(i=0; i < bgCounter; i++){
        if(waitpid(-1, &status, wflag) > 0){
            if(WIFEXITED(status) || WIFSIGNALED(status)){
                finished++;
            }
        }
    }
    bgCounter -= finished;
}

static void clrZombies(){
    commandBarrier(0);
}

static char ** splitArguments(char * cmd, int * args_count){
    int args_size = 10;
    char **argv = malloc(sizeof(char*)*args_size);
    
    int argc = 0;
    char * arg = strtok(cmd, " \t");
    while(arg){
        
        argv[argc++] = arg;
        
        if(argc > args_size){
            args_size += 10;
            argv = realloc(argv, args_size*sizeof(char*));
        }
        arg = strtok(NULL, " \t");
    }
    argv[argc] = NULL;
    
    *args_count = argc;
    return argv;
}

static int isBackground(char **argv, int argc, int* bg){
    if(strcmp(argv[argc-1], "&") == 0){
        *bg = 1;
        argv[--argc] = NULL;
        
    }else{
        int last_arg_len = strlen(argv[argc-1]);
        
        if(argv[argc-1][last_arg_len-1] == '&'){
            *bg = 1;
            argv[argc-1][last_arg_len-1] = '\0';
        }
    }
    
    return argc;
}

static int isRedirect(char **argv, int argc, int* fd){
    if( (argc >= 3) &&
       (strcmp(argv[argc-2], ">") == 0)){
        *fd = open(argv[argc-1], O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if(*fd == -1){
            perror("open");
        }
        
        argv[argc-2] = NULL;
        argc -= 2;
    }
    
    return argc;
}

static int isFork(char * CMD){
    
    int argc = 0;
    char ** argv = splitArguments(CMD, &argc);
    
    int bg = 0;
    argc = isBackground(argv, argc, &bg);
    
    int fd = -1;
    argc = isRedirect(argv, argc, &fd);
    
    
    pid_t pid = fork();
    switch(pid){
        case -1:
            perror("fork");
            break;
        case 0:
            if(fd > 0){
                if(dup2(fd, STDOUT_FILENO) != STDOUT_FILENO){
                    perror("dup2");
                    exit(1);
                }
            }
            execvp(argv[0], argv);
            perror("execv");
            exit(0);
            
        default:
            if(fd > 0)
                close(fd);
            
            if(bg){
                ++bgCounter;
            }else{
                int status;
                waitpid(pid, &status, 0);
            }
            break;
    }
    free(argv);
    
    return 0;
}

static int isNewLine(char * cmdline, int len){
    int i = -1;
    int whitespaces = 0;
    
    for(i=0; i < len; i++){
        switch(cmdline[i]){
            case ' ': case '\t':
                whitespaces++;
                break;
            default:
                i = len;
                break;
        }
    }
    
    int newlen = (len-whitespaces);
    if(whitespaces > 0){
        for(i=0; i < newlen; i++){
            cmdline[i] = cmdline[whitespaces+i];
        }
        cmdline[newlen] = '\0';
    }
    
    return newlen;
}

void sigHand(int rc){
    if(rc == SIGINT){
        interrupt = 1;
    }
}

void currentDirectory(){
	char directory[1024];
	getcwd(directory, sizeof(directory));
	printf("\nDir: %s\n\n", directory);
}

