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

pid_t r_procces;
char command[1024];
char promptName[1024];
char *new_command2;
char *argv[1024];
char ch, gc;
int last_command = 0;
List commands_Memmory;
char *new_command;
int stdout_fd = 0;
int mainProcess = 0;
int i;

char *end_if = "fi\n";

char last_Command[1024];   // Previous command buffer
char currentCommand[1024]; // Curret command buffer
List variables;
int status = 0; // status
int process(char **args);

typedef struct Var
{
    char *key;
    char *value;
} Var;

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

void sighandler(int sig)
{
    if (getpid() == mainProcess)
    {
        printf("\n");
        printf("You typed Control-C!");
        printf("\n");
        write(0, promptName, strlen(promptName) + 1);
        return;
    }
    else
    {
        fprintf(stderr, "\ncaught signal: %d\n", sig);
        kill(mainProcess, SIGKILL); // If the SHELL runs another process, the process will be thrown by the system
        return;
    }
}

// split The command ("ls -l -r" => {ls, -l, -r})
void splitCommand(char *command)
{
    char *p_command = strtok(command, " ");
    int i = 0;
    while (p_command != NULL)
    {
        argv[i] = p_command;
        p_command = strtok(NULL, " ");
        i++;
    }
    argv[i] = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

int execute(char **args)
{
    int rv = -1, c_pip = 0, i = count_Args(args), amper = -1;
    char **pipPointer = find_Pipe(args); // returns pointer to the location of the character in the string, NULL otherwise.

    int pipe_fd[2];
    char *outfile;
    int fd, redirect_fd = -1;

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

        if (cpid == -1) // the forking not success
        {
            perror("Error occurred: Failed to create new process\n");
            exit(EXIT_FAILURE);
        }

        // fork is successful and creates a new process we get 0
        else if (cpid == 0)
        {

            int status = close(0);
            if (status == -1)
            {
                printf("Failed to close standard input file descriptor\n");
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

        stdout_fd = dup(1); // duplicate fd, return new file descriptor on the same file.
        if (stdout_fd == -1)
        {
            printf("Failed to duplicate file descriptor\n");
            exit(EXIT_FAILURE);
        }
        // Redirect output to write end of pipe
        // Duplicate FD to FD2, closing FD2 and making it open on the same file.
        dup2(pipe_fd[1], 1);
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

    // for task 6 run the last command
    if (strcmp(args[0], "!!") == 0)
    {
        strcpy(currentCommand, last_Command);
        splitCommand(currentCommand);
        execute(argv);
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
        for (int j = 2; j < i; ++j)
        {
            strcat(newPromptName, args[j]);
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
            if (echo_var != NULL && echo_var[0][0] == '$')
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
    else
    {
        amper = 0;
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
                if ((fd = creat(outfile, 0660)) < 0)
                {
                    perror("Error: Can't create file");
                    exit(1);
                }
            }
            else
            {
                fd = open(outfile, O_RDONLY);
            }

            close(redirect_fd);
            dup(fd);
            close(fd);
            args[i - 2] = NULL;
        }

        if (execvp(args[0], args) == -1)
        {
            printf("%s\n", command);
            exit(1);
        }
    }
    else
    {
        wait(&status);
        rv = status;
        r_procces = -1;
    }

    if (c_pip != 0)
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
    int commandPosition = 0;

    char *previous_Command;

    while (1)
    {
        printf("\r");
        printf("%s", promptName);
        ch = getchar();

       //If the user presses the delete key
        if(ch== 127 || ch == '\b')
        {
            if(i<strlen(promptName))
            {
                printf("\b\b\b   \b");
            }
                continue;
        }

        if (ch == '\033') //up or down was pressed
        {
            printf("\33[2K"); // delete line
            getchar();         // skip the [
            gc = getchar();
            switch (gc)
            {
            case 'A':
                if (commands_Memmory.size == 0)
                {
                    break;
                }
                else if (commandPosition > 0)
                {
                    commandPosition--;
                }
                printf("\b");
                printf("\b");
                printf("\b");
                printf("\b");
                previous_Command = (char *)get(&commands_Memmory, commandPosition);
                printf("%s", (char *)get(&commands_Memmory, commandPosition));
                break;

            case 'B': // code for arrow down
                if (commands_Memmory.size == 0 || commandPosition >= commands_Memmory.size - 1)
                {
                    if (commandPosition == commands_Memmory.size - 1)
                    {
                        commandPosition++;
                    }
                    break;
                }
                else
                {
                    commandPosition++;
                }
                printf("\b");
                printf("\b");
                printf("\b");
                printf("\b");
                previous_Command = (char *)get(&commands_Memmory, commandPosition);
                printf("%s", (char *)get(&commands_Memmory, commandPosition));
                break;
            }

            getchar();
            continue;
        }
        else if (ch == '\n') 
        {
   
            if (commands_Memmory.size > 0)
            {
                splitCommand((char *)get(&commands_Memmory, commandPosition));
                execute(argv);
            }
            continue;
        }
        command[0] = ch;
        fgets(command + 1, 1023, stdin);

        int if_flag = 0;
        if (!strncmp(command, "if", 2))
        {
            if_flag = 1;
            while (1)
            {
                fgets(currentCommand, 1024, stdin);
                strcat(command, currentCommand);
                command[strlen(command) - 1] = '\n';
                if (!strcmp(currentCommand, end_if))
                    break;
            }
            char *commandcurr = "bash";
            char *argument_list[] = {"bash", "-c", command, NULL};
            if (fork() == 0)
            {
                // Newly spawned child Process. This will be taken over by "bash"
                int status_code = execvp(commandcurr, argument_list);
                printf("bash has taken control of this child process. This won't execute unless it terminates abnormally!\n");
                if (status_code == -1)
                {
                    printf("Terminated Incorrectly\n");
                    return 1;
                }
            }
            wait(&status);
            strcat(command, currentCommand);
        }

        command[strlen(command) - 1] = '\0';

        // task 7
        if (!strcmp(command, "quit"))
        {
            exit(0);
        }

        if (strcmp(command, "!!"))
        {
            strcpy(last_Command, command);
        }

        previous_Command = malloc(sizeof(char) * strlen(command));
        strcpy(previous_Command, command);
        add(&commands_Memmory, previous_Command);

        // We will update the last command index to be the updated size
        commandPosition = commands_Memmory.size;

        // if the last command is "if" we dont want to run this command again(Because we already ran it after it was called)
        if (!if_flag)
        {
            splitCommand(command);
            status = process(argv);
        }
        else
        {
            continue;
        }

    }
}