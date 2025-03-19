#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

int check_empty(char* pinput, int pinput_len){
    int is_empty = 1;
    if(pinput_len == 0){
        return 1;
    }
    for(int i = 0; i < pinput_len; i++){
        if(pinput[i] == ' ' || pinput[i] == '\t' || pinput[i] == '\n'){
            is_empty = 1;
        }
        else{
            return 0;
        }
    }
    return is_empty;

}
void remove_newline(char *s) {
    int len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

void print_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

void strip_white(char *str) {
    int i = 0;
    int j = 0;
    int len_str = strlen(str);
    for(i = 0; i < len_str; i++){
        if(str[i] != '\t' && str[i] != ' '){
            str[j] = str[i];
            j++;
        }
    }
    str[j] = '\0';
}

void execute_cmd(char *args[], int num_args){
    if(strcmp(args[0], "exit") == 0){
        if(num_args > 1){
            print_error();
        }else{
            exit(0);
        }

    }else if(strcmp(args[0], "cd") == 0){
        if(num_args == 1){
            chdir(getenv("HOME"));
        } else if(num_args == 2){
            if(chdir(args[1]) != 0){
                print_error();
            }
        } else{
            print_error();
        }
    }else if(strcmp(args[0], "pwd") == 0){
        if(num_args == 1){
            if(!(getcwd(NULL, 0))){
                print_error();
            }else{
                myPrint(getcwd(NULL, 0));
                myPrint("\n"); 
            }
        }else{
            print_error();
        }
    }else{
        pid_t pid = fork();
        if(pid < 0){
            print_error();
        } else if(pid > 0){
            int status = 0;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
                print_error();
            }
        } else{
            execvp(args[0], args);
            exit(1);
        }
    }
}

void execute_redir(char *args[], int fd){
    pid_t pid = fork();
    if(pid < 0){
        print_error();
        exit(0);
    } else if(pid > 0){
        int status = 0;
        waitpid(pid, &status, 0);
        close(fd);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
            print_error();
        }
    } else{
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execvp(args[0], args);
        exit(0);
    }
}

void execute_advanced(char* args[], int fd, char* dest){
    int temp_fd = open("temporary_file_to_copy1234567.txt", O_WRONLY | O_CREAT, 0777);
    if(temp_fd < 0){
        print_error();
        exit(0);
    }
    execute_redir(args, temp_fd);
    close(temp_fd);
    temp_fd = open("temporary_file_to_copy1234567.txt", O_WRONLY | O_APPEND);
    if(temp_fd < 0){
        print_error();
        exit(0);
    }
    FILE *dest_file = fopen(dest, "r");
    if(dest_file != NULL){
        char temp_buf[5000];
        while(fgets(temp_buf, 5000, dest_file)){
            write(temp_fd, temp_buf, strlen(temp_buf));
        }
        fclose(dest_file);  
    }
    close(temp_fd);

    FILE *temp_file = fopen("temporary_file_to_copy1234567.txt", "r");
    if(temp_file){
        char temp_buf2[5000];
        while(fgets(temp_buf2, 5000, temp_file)){
            write(fd, temp_buf2, strlen(temp_buf2));
        }
        fclose(temp_file);
    }
    close(fd);
    remove("temporary_file_to_copy1234567.txt");
}

int parse_line(char* pinput, char* cmds[]){  
    int pinput_len = strlen(pinput);
    if(check_empty(pinput, pinput_len)){
        return 0;
    }
    int count_cmd = 0;
    char* token = strtok(pinput, ";");
    while(token != NULL){
        cmds[count_cmd] = token;
        token = strtok(NULL, ";");
        count_cmd++;
    }
    return count_cmd;
}

int num_tokens(char* str){
    char* temp = strdup(str);
    int count = 0;
    char* tok = strtok(temp, " \t");
    while(tok != NULL){
        tok = strtok(NULL, " \t");
        count++;
    }
    return count;
}

int main(int argc, char *argv[]) 
{
    char cmd_buff[514];
    char *pinput;
    char* cmds[257];
    int checking = 1;

    while(checking){

        if(argc == 2){
            FILE* file = fopen(argv[1], "r");
            if(!file){
                print_error();
                exit(0);
            }
            while((pinput = fgets(cmd_buff, 514, file)) != NULL){
                int pinput_len = strlen(pinput);
                if(!(strchr(pinput, '\n'))){
                    write(STDOUT_FILENO, pinput, strlen(pinput));
                    int c;
                    while((c = fgetc(file)) != '\n'){
                        char ch = (char)c;
                        write(STDOUT_FILENO, &ch, 1);
                    }
                    write(STDOUT_FILENO, "\n", 1);
                    print_error();
                    continue;
                        
                }
                if(!(check_empty(pinput, pinput_len))){
                    write(STDOUT_FILENO, pinput, strlen(pinput));
                } 

                remove_newline(pinput);
                int count_cmd = parse_line(pinput, cmds);
                for(int i = 0; i < count_cmd; i++){
                    char* args[257];
                    int count_args = 0;
                    int invalid = 0;
                    if(strchr(cmds[i], '>')){
                        int num_red = 0;
                        int len_cmds = strlen(cmds[i]);
                        if(cmds[i][0] == '>'){
                            invalid = 1;
                            print_error();
                            continue;
                        }
                        for(int j = 0; j < len_cmds; j++){
                            if(cmds[i][j] == '>'){
                                num_red++;
                            }
                        }
                        if(num_red > 1){
                            invalid = 1;
                            print_error();
                            continue;
                        }
                        if(!invalid){
                            char* cmd_args = strtok(cmds[i], ">");
                            char* dest_arg = strtok(NULL, ">");
                            char* tok = strtok(cmd_args, " \t");
                            while(tok != NULL){
                                args[count_args] = tok;
                                tok = strtok(NULL, " \t");
                                count_args++;
                            }
                            if(dest_arg != NULL){
                                strip_white(dest_arg);
                                args[count_args++] = dest_arg;
                                if(check_empty(dest_arg, strlen(dest_arg))){
                                    print_error();
                                    continue;
                                }
                            }else{
                                print_error();
                                continue;
                            }
                            args[count_args] = NULL;
                            if(count_args < 2){
                                invalid = 1;
                                print_error();
                                continue;
                            }
                            if(num_tokens(args[count_args - 1]) > 1){
                                invalid = 1;
                                print_error();
                            }

                            if(args[0] == NULL || strcmp(args[0], "cd") == 0 || strcmp(args[0], "pwd") == 0 || strcmp(args[0], "exit") == 0 ){
                                print_error();
                                invalid = 1;
                            }
                        }

                        if(invalid){
                            continue;
                        }
                        char* dest = args[count_args - 1];
                        int is_adv = 0;
                        int fd = 0;
                        if(dest[0] == '+'){
                            dest++;
                            is_adv = 1;
                        }
                        if(access(dest, F_OK) == 0){
                            if(!is_adv){
                                print_error();
                                continue;
                            }else{
                                fd = open(dest, O_WRONLY | O_CREAT, 0777);
                                if(fd < 0){
                                    print_error();
                                    continue;
                                }
                            }
                        }else{
                            if(is_adv){
                                is_adv = 0;
                            }
                            fd = open(dest, O_WRONLY | O_CREAT, 0777);
                            if(fd < 0){
                                print_error();
                                continue;
                            }
                        }
                        args[count_args - 1] = NULL;

                        if(is_adv){
                            execute_advanced(args, fd, dest);
                        }else{
                            execute_redir(args, fd);
                        }
                    }else{
                        char* arg = strtok(cmds[i], " \t");
                        while(arg != NULL){
                            args[count_args] = arg;
                            arg = strtok(NULL, " \t");
                            count_args++;
                        }
                        args[count_args] = NULL;
                        if(args[0] == NULL){
                            continue;
                        }
                        execute_cmd(args, count_args);
                    }
                }
            }
            fclose(file);
            exit(0);
        } else if(argc == 1){
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, 514, stdin);
            if (!pinput) {
                exit(0);
            }else if(!(strchr(pinput, '\n'))){
                write(STDOUT_FILENO, pinput, strlen(pinput));
                int c;
                while((c = fgetc(stdin)) != '\n'){
                    char ch = (char)c;
                    write(STDOUT_FILENO, &ch, 1);
                }
                write(STDOUT_FILENO, "\n", 1);
                print_error();
                continue;
                    
            }
            remove_newline(pinput);
            
            int count_cmd = parse_line(pinput, cmds);
            for(int i = 0; i < count_cmd; i++){
                char* args[257];
                int count_args = 0;
                int invalid = 0;
                if(strchr(cmds[i], '>')){
                    int num_red = 0;
                    int len_cmds = strlen(cmds[i]);
                    if(cmds[i][0] == '>'){
                        invalid = 1;
                        print_error();
                        continue;
                    }
                    for(int j = 0; j < len_cmds; j++){
                        if(cmds[i][j] == '>'){
                            num_red++;
                        }
                    }
                    if(num_red > 1){
                        invalid = 1;
                        print_error();
                        continue;
                    }
                    if(!invalid){
                        char* cmd_args = strtok(cmds[i], ">");
                        char* dest_arg = strtok(NULL, ">");
                        char* tok = strtok(cmd_args, " \t");
                        while(tok != NULL){
                            args[count_args] = tok;
                            tok = strtok(NULL, " \t");
                            count_args++;
                        }
                        if (dest_arg != NULL) {
                            strip_white(dest_arg);
                            args[count_args++] = dest_arg;
                            if(check_empty(dest_arg, strlen(dest_arg))){
                                print_error();
                                continue;
                            }
                        }else{
                            print_error();
                            invalid = 1;
                            continue;
                        }
                        args[count_args] = NULL;
                        if(count_args < 2){
                            invalid = 1;
                            print_error();
                            continue;
                        }

                        if(num_tokens(args[count_args - 1]) > 1){
                            invalid = 1;
                            print_error();
                        }
                        
                        if(args[0] == NULL || strcmp(args[0], "cd") == 0 || strcmp(args[0], "pwd") == 0 || strcmp(args[0], "exit") == 0 ){
                            print_error();
                            invalid = 1;
                        }
                    }

                    if(invalid){
                        continue;
                    }
                    char* dest = args[count_args - 1];
                    int is_adv = 0;
                    int fd = 0;
                    if(dest[0] == '+'){
                        dest++;
                        is_adv = 1;
                    }
                    if(access(dest, F_OK) == 0){
                        if(!is_adv){
                            print_error();
                            continue;
                        }else{
                            fd = open(dest, O_WRONLY | O_CREAT, 0777);
                            if(fd < 0){
                                print_error();
                                continue;
                            }
                        }
                    }else{
                        if(is_adv){
                            is_adv = 0;
                        }
                        fd = open(dest, O_WRONLY | O_CREAT, 0777);
                        if(fd < 0){
                            print_error();
                            continue;
                        }
                    }
                    args[count_args - 1] = NULL;
                    if(is_adv){
                        execute_advanced(args, fd, dest);
                    }else{
                        execute_redir(args, fd);
                    }
                }else{
                    char* arg = strtok(cmds[i], " \t");
                    while(arg != NULL){
                        args[count_args] = arg;
                        arg = strtok(NULL, " \t");
                        count_args++;
                    }
                    args[count_args] = NULL;
                    if(args[0] == NULL){
                        continue;
                    }
                    execute_cmd(args, count_args);
                }
            }
            
        } else {
            print_error();
            exit(0);
        }

    }
}
