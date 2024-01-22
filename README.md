# Sell
Assignment 1 - Advanced Programming :
In this assignment we will create **SELL**

## How To Run The Shell
We will compile the program by `make all` <br />
We will run the program: `./myshell`

## My Program Can Do
We can see that the program executes commands: <br />
`hello: ls -l` <br />
And also executes commands in the background:<br />
`hello: ls –l &`<br />
and also routes output to a file:<br />
`hello: ls –l > file`<br />

Note that the parts of the command are separated by the space character.<br />

write routing to stderr:<br />
`hello: ls –l nofile 2> mylog`<br />
Adding to an existing file by >> :<br />
`hello: ls -l >> mylog`<br />
As in a normal shell program, if the file does not exist, it will be created<br />

Command to change the cursor:<br />
`hello: prompt = myprompt`<br />
(The command contains three words separated by two spaces)<br />

echo command that prints the arguments:<br />
`hello: echo abc xyz`<br />
will print:<br />
abc xyz<br />
The command will support output routing(>> , >) <br />

the command<br />
`hello: echo $?`<br />
Print the status of the last command executed.<br />

A command that changes the shell's current working directory:<br />
`hello: cd mydir`<br />

A command that repeats the last command:<br />
`hello: !!`<br />
two exclamation marks in the first word of the command<br />

Command to exit the shell:<br />
`hello: quit`<br />

If the user typed C-Control, the program will not terminate but will print the message:<br />
`You typed Control-C!`<br />
If the SHELL runs another process, the process will be thrown by the system<br />
(default behavior)<br />

Option to chain several commands in a pipe.<br />
For each command in pipe a dynamic allocation of ar is needed<br />

Adding variables to sell<br />

command `read`<br />

Support flow control ie ELSE/IF.<br />
