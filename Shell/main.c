#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>
#include "linkedlist.h"
#include <termios.h>

pid_t r_procces;
char command[1024];
char promptName[1024];
char *argv[1024];
char ch, gc;
int last_command = 0; // or -1
List commands_Memmory;
char *new_command;
int stdout_fd = 0;
int mainProcess =0;

char last_Command[1024]; // Previous command buffer
char currentCommand[1024]; // Curret command buffer
List variables;
int status = 0; // status
int process(char **args);

///////////////////////////////////////////helper fuction//////////////////////////////////////////

// fuction that help to count the length of the command
int count_Args(char **args)
{
    char **pt = args;
    int ans = 0;
    while (*pt != NULL)
    {
        pt++;
        ans++;
    }
    return ans;
}

// fuction that help to identify the first occurrence of the string `"|"` (pipe character). -> for part 9
char **find_Pipe(char **args)
{
    char **pt = args;
    while (*pt != NULL)
    {
        if (strcmp(*pt, "|") == 0)
        {
            return pt;
        }

        pt++;
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
// Our 'Hash map'
typedef struct Var
{
    char *key;
    char *value;
} Var;


void sighandler(int sig)
{
    if (getpid() == mainProcess) {
        printf("\n");
        printf("You typed Control-C!");
        printf("\n");
        write(0, promptName, strlen(promptName)+1);
        return;
    }
    else{
        fprintf(stderr, "\ncaught signal: %d\n", sig);
        kill(mainProcess, SIGKILL); //If the SHELL runs another process, the process will be thrown by the system
        return;
    }
}

// Tokens to split a command ("ls -l -r" => {ls, -l, -r})
void splitCommand(char *command)
{
    char *token = strtok(command, " ");
    int i = 0;
    while (token != NULL)
    {
        argv[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    argv[i] = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////

int execute(char **args)
{
    int rv = -1, c_pip = 0, i = count_Args(args);
    char **pipPointer = find_Pipe(args); // returns pointer to the location of the character in the string, NULL otherwise.
    int pipe_fd[2];
    int amper = -1;
    char *outfile;
    int redirect_fd = -1;

    int fd, redirect = -1;
    pid_t pid;

    // for task 9 , if there pip
    if (pipPointer != NULL)
    {

        *pipPointer = NULL;
        c_pip++;

        int result = pipe(pipe_fd);

        if (result == -1)
        {
            printf("Failed to create pipe\n");
            exit(EXIT_FAILURE);
        }

        int cpid = fork();

        if (cpid == -1)
        {
            perror("Error occurred: Failed to create new process\n");
            exit(EXIT_FAILURE);
        }

        // fork is successful and creates a new process we get 0
        else if (cpid == 0)
        {

            int status = close(STDOUT_FILENO);
            if (status == -1)
            {
                printf("Failed to close standard output file descriptor\n");
                exit(EXIT_FAILURE);
            }

            int status_pip = close(pipe_fd[1]); // Reader will see EOF
            if (status_pip == -1)
            {
                printf("Failed to close file descriptor\n");
                exit(EXIT_FAILURE);
            }
            int new_fd = dup(pipe_fd[0]); // Duplicate FD, returning a new file descriptor on the same file
            if (new_fd == -1)
            {
                printf("Failed to duplicate file descriptor\n");
                exit(EXIT_FAILURE);
            }
            execute(pipPointer + 1);
            exit(0);
        }

        stdout_fd = dup(STDOUT_FILENO); // duplicate fd, return new file descriptor on the same file.
        if (stdout_fd == -1)
        {
            // Error occurred
            printf("Failed to duplicate file descriptor\n");
            exit(EXIT_FAILURE);
        }
        // Redirect output to write end of pipe
        // Duplicate FD to FD2, closing FD2 and making it open on the same file.
        dup2(pipe_fd[1], STDOUT_FILENO);
    }

    // if the command empty
    if (args[0] == NULL)
    {
        return 0;
    }

    // if the command line end with &
    if (!strcmp(args[i - 1], "&"))
    {
        amper = 1;
        args[i - 1] = NULL;
    }
    else
    {
        amper = 0;
    }

    // for task 6  run the last command
    if (strcmp(args[0], "!!") == 0)
    {
        
        if (commands_Memmory.size > 0)
        {
            strcpy(currentCommand, last_Command);
            splitCommand(currentCommand);
            execute(argv);
        }
        else
        {
            printf("The command history list is empty");
        }
        return 0;
    }

    // for task 10 Adding variables to the shell
    if (args[0][0] == '$' && i >= 3)
    {
        Var *var = (Var *)malloc(sizeof(Var));
        var->key = malloc((strlen(args[0]) + 1));
        var->value = malloc((strlen(args[2]) + 1));
        strcpy(var->key, args[0]);
        strcpy(var->value, args[2]);
        add(&variables, var);
        return 0;
    }

    if (!strncmp(args[0], "if", 2))
    {
        int flag = 1;
        while (argv[flag] != NULL)
        {
            argv[flag - 1] = argv[flag];
            flag++;
        }
        argv[flag - 1] = NULL;
        int currstatus = execute(args);

        char insidestatement[1024];
        if (!currstatus)
        {
            fgets(insidestatement, 1024, stdin);
            insidestatement[strlen(insidestatement) - 1] = '\0';
            if (!strcmp(insidestatement, "then"))
            {
                fgets(insidestatement, 1024, stdin);
                insidestatement[strlen(insidestatement) - 1] = '\0';
                int elseFlag = 1;
                while (strcmp(insidestatement, "fi"))
                {
                    if (!strcmp(insidestatement, "else"))
                    {
                        elseFlag = 0;
                    }
                    if (elseFlag)
                    {
                        splitCommand(insidestatement);
                        process(argv);
                    }
                    fgets(insidestatement, 1024, stdin);
                    insidestatement[strlen(insidestatement) - 1] = '\0';
                }
            }
            else
            {
                printf("Bad if statemnt");
                return 0;
            }
        }
        else
        {
            fgets(insidestatement, 1024, stdin);
            insidestatement[strlen(insidestatement) - 1] = '\0';
            while (strcmp(insidestatement, "else"))
            {
                fgets(insidestatement, 1024, stdin);
                insidestatement[strlen(insidestatement) - 1] = '\0';
            }
            while (strcmp(insidestatement, "fi"))
            {
                splitCommand(insidestatement);
                process(argv);
                fgets(insidestatement, 1024, stdin);
                insidestatement[strlen(insidestatement) - 1] = '\0';
            }
        }
        return 0;
    }

    /// for task 11 read variable decleration
    if (strcmp(args[0], "read") == 0)
    {
        Var *var = (Var *)malloc(sizeof(Var));
        var->key = malloc(sizeof(char) * (strlen(args[1])));
        var->value = malloc(sizeof(char) * 1024);
        var->key[0] = '$';
        memset(var->value, 0, 1024);
        strcpy(var->key + 1, args[1]);
        fgets(var->value, 1024, stdin);
        var->value[strlen(var->value) - 1] = '\0';
        add(&variables, var);
        return 0;
    }

    // task 5:
    // changes the sorectory
    if (!strcmp(args[0], "cd"))
    {
        if (chdir(args[1]) != 0)
        {
            printf(" %s: no such directory\n", argv[1]);
        }
        return 0;
    }

    // task 2 command to change the cursor
    if (strcmp(args[0], "prompt") == 0)
    {
        char newPromptName[1024] = "";
        for (int k = 2; k < i; ++k)
        {
            strcat(newPromptName, args[k]);
            strcat(newPromptName, " ");
        }
        strcpy(promptName, newPromptName);
        return 0;
    }

    if (strcmp(args[0], "echo") == 0)
    {
        char **echo_var = args + 1;
        // task 4: 
        if (strcmp(*echo_var, "$?") == 0)
        {
            printf("%d\n", status);
            return 0;
        }
        while (*echo_var)
        {
            if (*echo_var != NULL && *echo_var[0] == '$')
            {
                
                // task 3:

                Node *node = variables.head;
                char *new_variable = NULL;

                while (node)
                {
                    if (strncmp(((Var *)node->data)->key, *echo_var, strlen(*echo_var)) == 0)
                    {
                        new_variable = ((Var *)node->data)->value;
                    }
                    node = node->next;
                }
                if (new_variable != NULL)
                {
                    printf("%s ", new_variable);
                }
            }
            else
            {
                printf("%s ", echo_var[0]);
            }
            echo_var++;
        }
        printf("\n");
        return 0;
    }

    // task 1:
    // understand what kind of a redirection we have here
    // if the file does not exist, it will be created.

    if (i >= 2 && (!strcmp(argv[i - 2], ">") || !strcmp(argv[i - 2], ">>")))
    {
        outfile = argv[i - 1];
        redirect_fd = 1;
    }
    else if (i >= 2 && !strcmp(argv[i - 2], "2>"))
    {
        outfile = argv[i - 1];
        redirect_fd = 2;
    }
    else if (i >= 2 && !strcmp(argv[i - 2], "<"))
    {
        outfile = argv[i - 1];
        redirect_fd = 0;
    }

    // Fork a new process to execute the command
    r_procces = fork();
    if (r_procces == -1) // not success to forking
    {
        printf("Error forking a new process\n");
        exit(1);
    }
    else if (r_procces == 0) // success forking
    {
        /* redirection of IO ? */
        if (redirect_fd >= 0)
        {
            if (!strcmp(args[i - 2], ">>"))
            {
                if ((fd = open(outfile, O_APPEND | O_CREAT | O_WRONLY, 0660)) < 0)
                {
                    perror("Error: Can't create file");
                    exit(1);
                }
                lseek(fd, 0, SEEK_END);
            }
            else if (!strcmp(args[i - 2], ">") || !strcmp(args[i - 2], "2>"))
            {
                fd = creat(outfile, 0660);
            }
            else
            {
                // stdin
                fd = open(outfile, O_RDONLY);
            }

            close(redirect_fd);
            dup(fd);
            close(fd);
            args[i - 2] = NULL;
        }

        execvp(args[0], args);
    }
    /* parent continues here */
    if (amper == 0)
    {
        wait(&status);
        rv = status;
        r_procces = -1;
    }

    if (c_pip)
    {
        int status = close(STDOUT_FILENO);
        if (status == -1)
        {
            printf("Failed to close standard output file descriptor\n");
            exit(EXIT_FAILURE);
        }
        int status_pip = close(pipe_fd[1]);
        if (status_pip == -1)
        {
            printf("Failed to close file descriptor\n");
            exit(EXIT_FAILURE);
        }
        int new_df = dup(stdout_fd);
        if (new_df == -1)
        {
            printf("Failed to duplicate file descriptor\n");
            exit(EXIT_FAILURE);
        }
        wait(NULL);
    }

    return rv;
}

int process(char **args)
{
    int rv = -1;
    // do control command
    if (args[0] == NULL)
    {
        rv = 0;
    }
    else
    {
        rv = execute(args);
    }
    return rv;
}

int main()
{
    mainProcess = getpid();
    signal(SIGINT, sighandler);
    strcpy(promptName, "hello: ");
    int commandPosition = -1;
    
    char *previous_Command = malloc(sizeof(char) * 1024);

    while (1)
    {
        printf("%s", promptName);
        ch = getchar();
        if (ch == '\003')
        {
            printf("\33[2K");
            getchar(); // skip the [
            gc = getchar();
            switch (gc)
            {
            case 'A':
                if (commands_Memmory.size == 0)
                {
                    break;
                }
                // code for arrow up
                else if (last_command > 0)
                {
                    last_command--;
                }
                previous_Command=(char *)get(&commands_Memmory, last_command);

                printf("%s", (char *)get(&commands_Memmory, last_command));
                break;

            case 'B': // code for arrow down
                if (commands_Memmory.size==0 ||last_command >= commands_Memmory.size-1 )
                {
                    if(last_command == commands_Memmory.size-1){
                        last_command++;
                    }
                    break;
                }
                else 
                {
                    last_command++;
                }

                previous_Command=(char *)get(&commands_Memmory, last_command);
                printf("%s\n", (char *)get(&commands_Memmory, last_command));
                break;
            }

            command[0]=ch;
            continue;
        }
        if (ch == '\n')
        {

            splitCommand((char *)get(&commands_Memmory, commandPosition));
            execute(argv);
        }

        command[0] = ch;
        fgets(command + 1, 1023, stdin);
        command[strlen(command) - 1] = '\0';

        if (!strcmp(command, "quit"))
        {
            /*
            Task number: 7
            command to exit the shell:
            hello: quit
            */
            // tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
            exit(0);
        }
        if (strcmp(command, "!!"))
        {
            strcpy(previous_Command, command);
        }

        // adding the new command to the command list

        strcpy(previous_Command, command);
        add(&commands_Memmory, previous_Command);

        // We will update the last command index to be the updated size
        last_command = commands_Memmory.size;

        splitCommand(command);
        status = process(argv);
    }
}