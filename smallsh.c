#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<time.h>
#include<fcntl.h>
#include<signal.h>

//POSIX STUF
#include<sys/types.h>
#include<sys/stat.h>	
#include<unistd.h>


//global vars
int child_exit_method = -5;
bool foreground_mode = 0;
int exit_status = -5;

//array of child procs
pid_t BG_array[1000];
pid_t FG_proc = -5;
int BG_array_size = 0;


//******************************************************************************
//DEBUG STRING FUNCTION
//******************************************************************************
void debugString(char* in)
{
	printf("----------------- DEBUG STRING -------------------\n");
	printf("STRING LENGTH: %d\n", strlen(in));
	printf("STRING: %s\n\n", in); 

}

//******************************************************************************
//DEBUG STRING ARRAY FUNCTION
//******************************************************************************
void debugStringArray(char** in, int* size)
{
	int i;
	printf("-------------- DEBUG STRING ARRAY ----------------\n");
	printf("Array size: %d\n", *size);
	for(i = 0; i < *size; i++)
	{
		if(in[i] != NULL)
			printf("arg %d: %s  -------- has size %d\n", i, in[i], strlen(in[i]));
	}
}



//******************************************************************************
//this function removes any trailing spaces the user may have entered
//******************************************************************************
int cleanString(char* input, int len)
{
	int i;
	for(i = len-2; i > 0; i--)
	{
		//bail out if we detect a non space
		if(input[i] != 32)
			break;

		//char is space, we need to remove it
		else
		{
			input[i] = '\0';
			len--;
		}

	}

	input[strlen(input)-1] = '\0';
	return len;
}

//******************************************************************************
//this function reads input for special run-in-background character
//if background command, return 1
//******************************************************************************
int detectBackgroundCommand(char* input, int commentFlag)
{
	//this is a comment string
	if(commentFlag == 1)
		return 0;

	if(strcmp(input, "&") == 0)
		return 1;

	return 0;
}

//******************************************************************************
//this function reads input to check for comment. returns 1 for comment
//******************************************************************************
int detectCommentLine(char* input)
{
	if(input[0] == '#')
		return 1;
		
	return 0;
}

void printFlags(int* flags)
{	
	printf("-----FLAG STATUS-----\n");
	printf("comment    flag: %d\n", flags[0]);
	printf("background flag: %d\n", flags[1]);
	printf("cd         flag: %d\n", flags[2]);
	printf("status     flag: %d\n", flags[3]);
}

//******************************************************************************
//this function takes a string and expands its PID if it has the PID symbol of $$
//returns a new string if has PID
//******************************************************************************
char* expandPID(char* input, int len)
{
	char* check;


	check = strstr(input, "$$");

	if(check)
	{
		//citation: https://stackoverflow.com/questions/53230155/converting-pid-t-to-string
		int pid = getpid();
		char pid_string[6];
		sprintf(pid_string, "%d", pid);
		
		char* newString = malloc(2054 * sizeof(char));
		memset(newString, '\0', 2054); 

		int i;
		int x = 0;
		for(i = 0; i < len-1; i++)
		{
			newString[x] = input[i];

			//make sure we don't go out of range
			if(i < len - 1)
			{
				//we need to expand this
				if(input[i] == '$' && input[i+1] == '$')
				{
					int k;
					for(k = 0; k < 6; k++)
					{
						newString[x] = pid_string[k];
						x++;
					}
					//some weird stuff going on here strcat was getting more difficult
					x--;
					i++;
				}
			}	
			x++;
		}
		return newString;
	}
	else
		return input;
}


int checkEXIT(char * input, int* size)
{
	char* exit = "exit";
	if(strcmp(input, exit) == 0)
	{
		if(*size == 1)
		{
		//**** TODO ******
		//must kill all processes before terminating
		//
		//
		//
			return 1;
		}
	}
	return 0;
}

//******************************************************************************
//this function checks the user input for command "CD". If detected, CD will
//chdir to HOME or valid do absolute /  direct path to new directory
//******************************************************************************
int checkCD(char* input, char* cwd, char** arg_array, int* size)
{
	char* cd = "cd";
	int test;

	//absolute cd check
	if(strcmp(input, cd) == 0)
	{
		//no CD argumetns
		if(*size == 1)
		{
			char* homedir = getenv("HOME");
			test = chdir(homedir);
			return 1;
		}
		else if(*size == 2)
		{

			char new_cwd[1024];
			getcwd(new_cwd, sizeof(new_cwd));

			strcat(new_cwd,"/");
			strcat(new_cwd,arg_array[1]); 
			test = chdir(new_cwd);
				if(test)
				{
					printf("this is not a dir!\n");
					return 2;
				}
			return 1;
		}
		else
			printf("CD only takes one argument");
	}
	return 0;	
}


//******************************************************************************
//this function checks the user input for command "STATUS". If detected, STATUS 
//will print to stdout the last exit status of a child termination, or 0 if none
//******************************************************************************
int checkSTATUS(char** arg_array, int* exit_status, int* size)
{
	char* status = "status";
	
	if(strcmp(arg_array[0], status) == 0)
	{
	
		//check for no arguments
		if(*size == 1 || (*size == 2 && strcmp(arg_array[*size-1], "&") == 0))		
		{
			printf("exit value %d\n", *exit_status);
			fflush(stdout);
			return 1;
		}
		else
			return 2;
	}

	return 0;
}

//******************************************************************************
//this function will parse the user input into a command and an argument array
//******************************************************************************
char* parseInput(char* input, char** arg_array, int* size)
{

	int i = 1;
	int counter = 0;
	char* command;
	char* arg = strtok(input, " ");

	//create command string
	command = arg;

	//put the command as first arg
	if(arg != NULL)
		arg_array[0] = arg;

	///parse all args
	while(arg != NULL)
	{
		arg = strtok(NULL, " ");
		arg_array[i] = arg;	
		i++;
		counter++;
	}

	*size = counter;

	return command;
}


//******************************************************************************
//this function detects redirection and 
//chdir to HOME or valid do absolute /  direct path to new directory
//******************************************************************************
int detectRedirection(char** arg_array, int* size, int* ret)
{
  int i;
	bool flag = false;
  for(i = 0; i < *size; i++)
  {
		//check for stdin redirect
		if(strcmp(arg_array[i], "<") == 0 && i < *size-1)
    {
			int target1FD = open(arg_array[i+1], O_RDONLY, 0644);
			if(target1FD > 0)
			{
				dup2(target1FD, 0);
				flag = true;
			}
			else
			{
				printf("file does not exist\n");
				fflush(stdout);
				*ret = -1;
				return -1;
			}
    }
		//check for stdout redirect
		if(strcmp(arg_array[i], ">") == 0 && i < *size-1)
    {
			int target2FD = open(arg_array[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
			dup2(target2FD, 1);
			flag = true;
    }
  }

	if(flag)
	{
		//we need to clear the arguments now after redirection
		for(i = 0; i < *size; i++)
		{
			//we need to wipe our arg array from this point on
			if(strcmp(arg_array[i], ">") == 0 || strcmp(arg_array[i], "<") == 0)
			{
				for(i = i; i < *size; i++)
					arg_array[i] = NULL;
			}
		}
		return 1;
	}
	else
		return 0;
}

//******************************************************************************
//signal handler function: this function detects when a child process has exited
//and if a BG process, prints to console the relevent information needed
//******************************************************************************

void catchSIGCHLD(int sig)
{
	pid_t tempPID;
	//citation: https://stackoverflow.com/questions/7171722/how-can-i-handle-sigchld
	tempPID = waitpid(-1, &child_exit_method, WNOHANG);
	int i;
	for(i = 0; i < BG_array_size; i++)
	{
		if(BG_array[i] == tempPID)
		{
			//store our strings to write
			char* message1 = "background pid ";
			char* message2 = " is done: ";
			char* stringPID = malloc(6);
			sprintf(stringPID, "%d", tempPID);

			//write the background pid to console
			write(STDOUT_FILENO, message1, 15);
			write(STDOUT_FILENO, stringPID, 6);
			write(STDOUT_FILENO, message2, 10);

			//determine the exit method
			if(WIFEXITED(child_exit_method))
			{
				exit_status = WEXITSTATUS(child_exit_method);
				char* exit_s_string = "exit value ";
				write(STDOUT_FILENO, exit_s_string, 11);
		
			}
			//signal terminated the child
			else
			{
				exit_status = WTERMSIG(child_exit_method);
				char* term_s_string = "terminated by signal ";
				write(STDOUT_FILENO, term_s_string, 21);
			}

			//write the exit status flag
			char* exit_s_method = malloc(6);
			sprintf(exit_s_method, "%d", exit_status);
			write(STDOUT_FILENO, exit_s_method, 6);

			char* end = "\n";
			write(STDOUT_FILENO, end, 1);

			//free our malloc'd memory
			free(exit_s_method);
			free(stringPID);
		}
	}
}

//******************************************************************************
//signal handler function: this function detects when when ctrl-Z is pressed.  
//it will put the program into 'foreground only mode' and prevent any background
//processes from exec'ing.  Exited the same way as entered.
//******************************************************************************
void catchSIGTSTP(int sig)
{
	char* message;
	if(!foreground_mode)
	{
		message = "   Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 52);
		foreground_mode = 1;
	}
	else
	{
		message = "   Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 32);
		foreground_mode = 0;
	}
}

//******************************************************************************
//signal handler function: this function detects when when ctrl-C is pressed.
//this will kill a foreground process only.  If not FG process, does nothing.
//******************************************************************************
void catchSIGINT(int sig)
{
	//check to see if we even have a FG process running
	if(FG_proc > 0)
	{
		kill(FG_proc, SIGINT);
		char* message = "terminated by signal 2\n";
		write(STDOUT_FILENO, message, 23);
	}
	FG_proc = -5;	
}


//_________________________________________________MAIN_____________________________________________________//

int main(int argc, char* argv[])
{
	//declare our local vars for the shell
	int nums = -5;
	size_t buffer = 0;
	char* raw_input = NULL;	
	char* input = NULL;	
	int action;
	int flags[10] = { 0 };
	int size = 0;
	pid_t spawnPID = -5;

	//set up signal set
	struct sigaction SC_action = {0};
	SC_action.sa_handler = catchSIGCHLD;
	sigfillset(&SC_action.sa_mask);
	SC_action.sa_flags = SA_RESTART;

	//set up signal set for SIGTSTP
	struct sigaction TS_action = {0};
	TS_action.sa_handler = catchSIGTSTP;
	sigfillset(&TS_action.sa_mask);
	TS_action.sa_flags = SA_RESTART;

	//set up signal set for SIGINT
	struct sigaction SI_action = {0};
	SI_action.sa_handler = catchSIGINT;
	sigfillset(&SI_action.sa_mask);
	SI_action.sa_flags = SA_RESTART;

	//signal actions
	sigaction(SIGCHLD, &SC_action, NULL);
	sigaction(SIGTSTP, &TS_action, NULL);
	sigaction(SIGINT, &SI_action, NULL);

	//get the current working directory of shell
	//citation: https://stackoverflow.com/questions/16285623/how-to-get-
	//the-to-get-path-to-the-current-file-pwd-in-linux-from-c
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));

	while(1)
	{
		//console indicator
		printf(": ");
		
		//get teh input
		nums = getline(&raw_input, &buffer, stdin);
	
		//input is acceptable
		if(nums != -1 && nums != 1)
		{
			int len = strlen(raw_input);
			
			//remove trailing spaces from input if any
			len = cleanString(raw_input, len);
			input = expandPID(raw_input, len);

			
			//initialize input parser variables
			//parse input
			char* arg_array[512] = { NULL }; 

			char* command = parseInput(input, arg_array, &size);


			//-----------------------------------------------//
			//from this point on, we have our command and its
			//arguments if it has any
			//-----------------------------------------------//
			
			//check built in 1: EXIT
			if(checkEXIT(command, &size))
				return 0;
			
			//check built in 2: CD 
			int cdFlag = checkCD(command, cwd, arg_array, &size);
			
			getcwd(cwd, sizeof(cwd));;

			//check built in 3: STATUS
			int statusFlag = checkSTATUS(arg_array, &exit_status, &size);

			//detect if input was comment
			int commentFlag = detectCommentLine(input);

			//detect if the input is meant to be background process
			int backgroundFlag;
			if(size > 0)
				backgroundFlag = detectBackgroundCommand(arg_array[size-1], commentFlag);
			else
				backgroundFlag = 0;

			//set flags useful to know what user is wanting
			//used for debug function used for state flags
			flags[0] = commentFlag;
			flags[1] = backgroundFlag;
			flags[2] = cdFlag;
			flags[3] = statusFlag;

			//sum the flags, if 0, user didn't try any built ins
			int sum = flags[0] + flags[1] + flags[2] + flags[3];
			
			if(sum == 0 || (sum == 1 && flags[1] == 1))
			{
				//remove our background symbol from argumens
				if(strcmp(arg_array[size-1], "&") == 0)
				{
					arg_array[size-1] = NULL;
					size--;	
				}

				spawnPID = fork();

				//switch to capture fork
				switch(spawnPID)
				{
					case -1:
					{
						perror("something bad happened");
						fflush(stdout);

						exit(1);
						break;
					}

					//we are in the child process here
					case 0:
					{
						//save stdout
						int saved_in = dup(0);

						//save stdin
						int saved_out = dup(1);
						int ret;

						//check for redirection parsing for file pointer changes
						if(detectRedirection(arg_array, &size, &ret))
						{
							//make sure there is a file to read!
						  if(ret == -1)
							{
								exit(ret);
							}

							//call the function
							ret = execvp(arg_array[0], arg_array);
							if(ret != 0)
							{
								dup2(saved_out, 1);
								dup2(saved_in, 0);
								printf("command not found\n");
								fflush(stdout);
								//kill the failed process
								exit(ret);
							}
						}
						//no redirection
						else
						{
							ret = execvp(arg_array[0], arg_array);
							//ret failed
							if(ret != 0)
							{
								printf("command not found\n");
								fflush(stdout);
								//kill the failed process
								exit(ret);
							}
						}
						break;
					}
				
					//we are in the parent process here
					default:
					{
						//this deosn't seem right to have just a break but it works?
						break;
					}
				}

				//block the parent process until child has completed
				if(sum == 0 ||  foreground_mode == 1)
				{
					FG_proc =  spawnPID;
					waitpid(spawnPID, &child_exit_method, 0);
				}

				//this is a background process
				if(sum == 1 && backgroundFlag == 1 && foreground_mode == 0)
				{
					printf("background pid is %d\n", spawnPID);
					fflush(stdout);
					BG_array[BG_array_size] = spawnPID;
					BG_array_size++;
				}
			}
		}

		//end of loop, check exit status of child procs
		if(WIFEXITED(child_exit_method))
			exit_status = WEXITSTATUS(child_exit_method);
		//signal terminated the child
		else
			exit_status = WTERMSIG(child_exit_method);
	}
	return 0;
}
