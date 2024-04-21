#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define PATH_MAX 1024

#define HISTORY_DEPTH 1000
static int CMDnum = 0;
char history[HISTORY_DEPTH][COMMAND_LENGTH];

char input_buffer[COMMAND_LENGTH];
char *tokens[NUM_TOKENS];

void handle_signal(int signal);


void display() {
    if (system("clear") == -1) {
        perror("system");
        exit(EXIT_FAILURE);
    }
    printf("\n\n\n");
    printf("\033[1;36m"); // Set text color to cyan
    printf("\t\t\t\t\t\t WELCOME TO SD SHELL");
    printf("\033[0m"); // Reset text color
    fflush(stdout);
    sleep(3);
    if (system("clear") == -1) {
        perror("system");
        exit(EXIT_FAILURE);
    }
}


void addcmd(char buff[])
{
        strcpy(history[CMDnum],buff);
        CMDnum++;
}

void getCMD(int n)
{
        char buffI[10];
        sprintf(buffI,"%d",n);
        write(STDOUT_FILENO,buffI,strlen(buffI));
        write(STDOUT_FILENO,"\t",strlen("\t"));
        write(STDOUT_FILENO,history[n],strlen(history[n]));
        write(STDOUT_FILENO,"\n",strlen("\n"));
}

void printcmd()
{
        int i = CMDnum - 1;
        if(i <= 9){
                while(i >= 0){
                        getCMD(i);
                        i--;
                }
        }
        else{
                int s = i;
                int e = s-9;
                while(s >= e){
                        getCMD(s);
                        s--;
                }
        }
}

int tokenize_command(char *buff, char *tokens[])
{
        int token_count = 0;
        _Bool in_token = false;
        int num_chars = strnlen(buff, COMMAND_LENGTH);
        for (int i = 0; i < num_chars; i++) {
                switch (buff[i]) {
                case ' ':
                case '\t':
                case '\n':
                        buff[i] = '\0';
                        in_token = false;
                        break;
                default:
                        if (!in_token) {
                                tokens[token_count] = &buff[i];
                                token_count++;
                                in_token = true;
                        }
                }
        }
        tokens[token_count] = NULL;
        return token_count;
}

void read_command(char *buff, char *tokens[], _Bool *in_background)
{
        *in_background = false;

        int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

        if ( (length < 0) && (errno !=EINTR) ){
        perror("Unable to read command. Terminating.\n");
        exit(-1);
        }

        buff[length] = '\0';

        if (buff[strlen(buff) - 1] == '\n') {
                buff[strlen(buff) - 1] = '\0';
        }

        if(buff[0] == '!' && buff[1] == '!'){
                buff = history[CMDnum - 1];
                addcmd(buff);
                write(STDOUT_FILENO,buff,strlen(buff));
                write(STDOUT_FILENO,"\n",strlen("\n"));
        }
        else if(buff[0] == '!'){
                char cnum[10];
                int j = 0;
                for(int i = 1; buff[i] != '\0'; i++){
                        if(buff[i] >= 48 && buff[i] <= 57){
                                cnum[j] = buff[i];
                                j++;
                        }
                }
                cnum[j] = '\0';
                if(cnum[0] == '\0'){
                        write(STDOUT_FILENO,"\nERROR: invalid argument, try entering a number after '!'\n",strlen("\nERROR: invalid argument, try entering a number after '!'\n"));
                }
                else{
                        int n = atoi(cnum);
                        if(n < CMDnum - 11){
                                write(STDOUT_FILENO,"\nERROR: inalid argument, try entering a number in range\n",strlen("\nERROR: inalid argument, try entering a number in range\n"));
                        }
                        else{
                                buff = history[n];
                                addcmd(buff);
                                write(STDOUT_FILENO,buff,strlen(buff));
                                write(STDOUT_FILENO,"\n",strlen("\n"));
                        }
                }
        }
        else{
                addcmd(buff);
        }

        int token_count = tokenize_command(buff, tokens);
        if (token_count == 0) {
                return;
        }

        if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
                *in_background = true;
                tokens[token_count - 1] = 0;
        }
}
int myCD(char **args);
int myPWD(char **args);
int myHELP(char **args);
int myHistory(char **args);

char *builtinCMDstr[] = {"cd", "pwd", "help","history", "!!", "!n", "exit"};
int (builtinCMDfunc[]) (char *) = { &myCD, &myPWD , &myHELP, &myHistory};

int myCD(char **args){
        if(args[1] == NULL){
                write(STDOUT_FILENO,"\nEXPECTED ARGUMENT TO 'cd'\n",strlen("\nEXPECTED ARGUMENT TO 'cd'\n"));
        } else{
                if(chdir(args[1]) != 0){
                        write(STDOUT_FILENO,"No such directory as '",strlen("No such directory as '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"'\n",strlen("'\n"));
                }
        }
        return 1;
}

int myPWD(char **args){
        char cwd[PATH_MAX];
        if(args[1] != NULL){
                write(STDOUT_FILENO,"\nERROR: Did not expect argument '",strlen("\nERROR: Did not expect argument '"));
                write(STDOUT_FILENO,args[1],strlen(args[1]));
                write(STDOUT_FILENO,"' for commmand 'pwd'\n",strlen("' for commmand 'pwd'\n"));
        } else{
                if(getcwd(cwd, sizeof(cwd)) != NULL){
                        write(STDOUT_FILENO,"Current working directory: ",strlen("Current working directory: "));
                        write(STDOUT_FILENO,cwd,strlen(cwd));
                        write(STDOUT_FILENO,"\n",strlen("\n"));
                }
                else{
                        write(STDOUT_FILENO,"\nERROR: could not load current working directory\n",strlen("\nERROR: could not load current working directory\n"));
                }
        }
        return 1;
}

int isBuiltin(char *str){
        int i = 0;
        while(i < 7){
                if(strcmp(str,builtinCMDstr[i]) == 0){
                        return i;
                }
                i++;
        }
        return -1;
}

int myHELP(char **args){
        if(args[2] != NULL){
                write(STDOUT_FILENO,"\nERROR: Too many arguments for 'help'\n",strlen("\nERROR: Too many arguments for 'help'\n"));
                return 1;
        }
        if(args[1] == NULL){
                write(STDOUT_FILENO,"\t\t  HELP  \n",strlen("\t\t  HELP  \n"));
                write(STDOUT_FILENO,"Type 'help' followed by command name and press ENTER\n",strlen("Type 'help' followed by command name and press ENTER\n"));
                write(STDOUT_FILENO,"The following are BUILT-IN commmands \n",strlen("The following are BUILT-IN commmands \n"));
                int i = 0;
                while(i < 7){
                        write(STDOUT_FILENO," - ",strlen(" - "));
                        write(STDOUT_FILENO,builtinCMDstr[i],strlen(builtinCMDstr[i]));
                        if(strcmp(builtinCMDstr[i],"cd") == 0){
                                write(STDOUT_FILENO," : built-in commmand for changing the current working directory\n\n",strlen(" : built-in commmand for changing the current working directory\n\n"));
                        } else if(strcmp(builtinCMDstr[i],"pwd") == 0){
                                write(STDOUT_FILENO," : built-in commmand for printing the current working directory\n\n",strlen(" : built-in commmand for printing the current working directory\n\n"));
                        } else if(strcmp(builtinCMDstr[i],"exit") == 0){
                                write(STDOUT_FILENO," : built-in commmand for exiting the program\n\n",strlen(" : built-in commmand for exiting the program\n\n"));
                        } else if(strcmp(builtinCMDstr[i],"help") == 0){
                                write(STDOUT_FILENO," : built-in commmand for displaying information on all commands[no arguments] or a specific command[with argument]\n\n",strlen(" : built-in commmand for displaying information on all commands[no arguments] or a specific command[with argument]\n\n"));
                        } else if(strcmp(builtinCMDstr[i],"exit") == 0){
                                write(STDOUT_FILENO," : built-in commmand for exiting the shell\n\n",strlen(" : built-in commmand for exiting the shell\n\n"));
                        } else if(strcmp(builtinCMDstr[i],"history") == 0){
                                write(STDOUT_FILENO," : built-in commmand for displaying last 10 commands\n\n",strlen(" : built-in commmand for displaying last 10 commands\n\n"));
                        } else if(strcmp(builtinCMDstr[i],"!!") == 0){
                                write(STDOUT_FILENO," : built-in commmand for executing the last command\n\n",strlen(" : built-in commmand for executing the last command\n\n"));
                        } else if(strcmp(builtinCMDstr[i],"!n") == 0){
                                write(STDOUT_FILENO," : built-in commmand for executing the nth-last command\n\n",strlen(" : built-in commmand for executing the nth-last command\n\n"));
                        }
                        i++;
                }
                return 1;
        } else{
                int bI = isBuiltin(args[1]);
                if(bI == 0){
                        write(STDOUT_FILENO,"   - '",strlen("   - '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' is a built-in command for changing the current working directory\n\n",strlen("' is a built-in command for changing the current working directory\n\n"));
                        //return 1;
                } else if(bI == 1){
                        write(STDOUT_FILENO,"   - '",strlen("   - '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' is a built-in command for printing the current working directory\n\n",strlen("' is a built-in command for printing the current working directory\n\n"));
                        //return 1;
                } else if(bI == 2){
                        write(STDOUT_FILENO,"   - '",strlen("   - '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' is a built-in command for showing info. on commands\n\n",strlen("' is a built-in command for showing info. on commands\n\n"));
                } else if(bI == 3){
                        write(STDOUT_FILENO,"   - '",strlen("   - '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' is a built-in command for exiting this shell\n\n",strlen("' is a built-in command for exiting this shell\n\n"));
                } else if(bI == 4){
                        write(STDOUT_FILENO,"   - '",strlen("   - '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' is a built-in command for displaying the last 10 executed commands\n\n",strlen("' is a built-in command for displaying the last 10 executed commands\n\n"));
                } else if(bI == -1){
                        write(STDOUT_FILENO,"   - '",strlen("   - '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' is an external commmand or application. Run 'man ",strlen("' is an external commmand or application. Run 'man "));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' for more information\n\n",strlen("' for more information\n\n"));
                } else if(bI == 5){
                        write(STDOUT_FILENO,"   - '",strlen("   - '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' is a built-in command for runnning the last command entered\n\n",strlen("' is a built-in command for runnning the last command entered\n\n"));
                } else if(bI == 6){
                        write(STDOUT_FILENO,"   - '",strlen("   - '"));
                        write(STDOUT_FILENO,args[1],strlen(args[1]));
                        write(STDOUT_FILENO,"' is a built-in command for runnning the nth-last command entered\n\n",strlen("' is a built-in command for runnning the nth-last command entered\n\n"));
                }
                return 1;
        }
}

int myHistory(char **args){
        printcmd();
        return 1;
}

int shellExec(char **args){
        int i = 0;
        if(args[0] == NULL){
                return 1;
        }
        while(i < 5){
                if(strcmp(args[0],builtinCMDstr[i]) == 0){
                        return (*builtinCMDfunc[i])(args);
                }else{
                        i++;
                }
        }
        return 0;
}

void handle_signal(int sig){
        signal(SIGINT, handle_signal);
        strcpy(input_buffer,"help");
}

int main(int argc, char* argv[])
{
        display();
        struct sigaction sa;
        sa.sa_handler = &handle_signal;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);

        while (true) {

                char cwd[PATH_MAX];
                write(STDOUT_FILENO," ~",strlen(" ~"));
                if(getcwd(cwd, sizeof(cwd)) != NULL){
                        write(STDOUT_FILENO,cwd,strlen(cwd));
                }
                write(STDOUT_FILENO, "$ ", strlen("$ "));
                _Bool in_background = false;
                read_command(input_buffer, tokens, &in_background);

                for (int i = 0; tokens[i] != NULL; i++) {
                        write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
                        write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
                        write(STDOUT_FILENO, "\n", strlen("\n"));
                }
                write(STDOUT_FILENO,"\n",strlen("\n"));
                if(strcmp(tokens[0],"exit") == 0)
                {
                        write(STDOUT_FILENO,"Exiting shell ...\n.",strlen("Exiting shell ....\n"));
                        return 0;
                } else{
                        if(shellExec(tokens) == 0)
                        {
                                pid_t pid = fork();
                                if(pid == -1){
                                        write(STDOUT_FILENO," ERROR FORKING\n", strlen(" ERROR FORKING\n"));
                                continue;
                                } else if(pid == 0){
                                execvp(tokens[0],tokens);
                                        write(STDOUT_FILENO,"COULD NOT RUN COMMAND '",strlen("COULD NOT RUN COMMAND '"));
                                        write(STDOUT_FILENO,tokens[0],strlen(tokens[0]));
                                        write(STDOUT_FILENO,"'\n",strlen("'\n"));
                                        exit(-1);
                                } else{
                                        if (in_background) {
                                        write(STDOUT_FILENO, "Run in background.\n", strlen("Run in background.\n"));
                                        wait(NULL);
                                        write(STDOUT_FILENO,"Command Executed: after runnning in background\n",strlen("Command Executed: after runnning in background\n"));
                                        }
                                        else{
                                                wait(NULL);
                                                write(STDOUT_FILENO,"Command Executed\n",strlen("Command Executed\n"));
                                        }
                                }
                        }
                }
        }
}