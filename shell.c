#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

#define BUFSIZE 100
#define CWD_SIZE 100
#define PATH_MAX 100



void changeDirectory(char **tokens) {

    if(chdir(tokens[1])!=0){
	    printf("Shell: Incorrect command\n");
    }

}

char** parseInput(char *input)
{
    int pos = 0;
    int buf_size = BUFSIZE;

    char **tokens = malloc(buf_size*sizeof(char*));
    char *tok;
    while((tok = strsep(&input," ")) != NULL) 
    {
        if(strlen(tok) == 0) 
        {
            continue;
        }
        tokens[pos++] = tok;
    }

    tokens[pos] = NULL;
    return tokens;

}
    

void executeCommand(char **tokens)
{
	// This function will fork a new process to execute a command
    if(strcmp(tokens[0],"cd") == 0) 
    {
        changeDirectory(tokens);
    }
    else 
    {
        int rc = fork();

        if(rc < 0) {
            //printf("Forking failed\n");
            exit(1);
        }
        else if(rc == 0) 
        {
            // child process

            // restoring the signals
            signal(SIGINT, SIG_DFL);        // for ctrl + c    
		    signal(SIGTSTP, SIG_DFL);       // for ctrl + z

            if(execvp(tokens[0], tokens) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }   
        }
        else 
        {
            int rc_wait = wait(NULL);
        } 
    }

    
    
}


void executeSequentialCommands(char **tokens)
{	
	// This function will run multiple commands in sequence
    int start = 0;
    int i=0;
    while(tokens[i])
    {    
        while(tokens[i] && strcmp(tokens[i],"##")!=0) 
        {
            i++;
        }

        tokens[i] = NULL;
        executeCommand(&tokens[start]);
        i++;
        start = i;
    }
}


void executeParallelCommands(char **tokens)
{
	// This function will run multiple commands in parallel
    int r;
    int i = 0;
    int start = 0;
    int process_count=0;
    
    while(tokens[i])
    {
        while(tokens[i] && strcmp(tokens[i],"&&")!=0) 
        {
            i++;
        }

        process_count++;
        tokens[i] = NULL;
        if(strcmp(tokens[start],"cd") == 0) 
        {
            changeDirectory(&tokens[start]);
        }
        else 
        {
            r = fork();
            if(r < 0) 
            {
                exit(1);
            }
            else if(r == 0) 
            {

                signal(SIGINT, SIG_DFL);
		        signal(SIGTSTP, SIG_DFL);

                if(execvp(tokens[start],&tokens[start]) == -1) 
                {
                    printf("Shell: Incorrect command\n");
                    process_count--;
                    exit(0);
                }
            }
            else 
                break; 
        }
        i++;
        start = i;
    }

    // parent process 
    while(process_count) {
        int ret = wait(NULL);
        process_count--;
    }
   
}


void executeCommandRedirection(char **tokens)
{
	// This function will run a single command with output redirected to an output file specificed by user
    int total_tok = 0;   
    
    for(int i=0;tokens[i]!=NULL;i++) 
    {
        total_tok++;
    }

    int r = fork();

    if(r < 0) 
    {
        // forking failed
        exit(1);
    }
    else if(r == 0) 
    {
        // child process

        // restoring the signals
        signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);

        // creating/emptying file.
        fclose(fopen(tokens[total_tok-1], "w"));

        // Redirecting STDOUT
        close(STDOUT_FILENO);
        int f = open(tokens[total_tok-1],O_CREAT | O_WRONLY | O_APPEND);         // tokens[total_tokens-1] represents filename (to redirect output)

        tokens[total_tok-1] = tokens[total_tok-2] = NULL;

        if(execvp(tokens[0],tokens) == -1) 
        {
            printf("Shell: Incorrect command\n");
            exit(1);
        }

        // closing file descriptor
        fflush(stdout); 
        close(f);
    }
    else 
    {
        int rc_wait = wait(NULL);
    }

}
char cwd[PATH_MAX];     // stored the location of current directory


void sigHandler(int sig) 
{
    printf("\n");
	printf("%s$", cwd);
	fflush(stdout);
    return;

}

int main()
{

    // Initial declarations
    signal(SIGTSTP,&sigHandler);              // for ctrl + z
    signal(SIGINT,&sigHandler);              // for ctrl + c
	

	
	while(1)	// This loop will keep your shell running until user exits.
	{
		// Print the prompt in format - currentWorkingDirectory$
        if (getcwd(cwd, sizeof(cwd)) != NULL) 
	   	{
	       printf("%s$", cwd);
	   	}

		
		// accept input with 'getline()'
        char *input = NULL;
        size_t size = 0;
        
        int byte_read  = getline(&input,&size,stdin);
        int len = strlen(input);
        input[len-1] = '\0';



		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		char **args = parseInput(input); 		
		
		if(strcmp(args[0],"exit") == 0)	
		{
            // When user uses exit command.
			printf("Exiting shell...\n");
			break;
		}

        int c = 0;
        for(int i=0; args[i]!= NULL ; i++) 
        {

            if(strcmp(args[i],"&&") == 0) 
            {
                // parallel execution
                c = 1;
                break;
            }
            else if(strcmp(args[i],"##") == 0) 
            {
                // sequential execution
                c = 2;
                
                break;
            }
            else if(strcmp(args[i],">") == 0) 
            {
                // redirection execution
               	c=3;
                break;
            }
        }
        if(c == 1) 
        {
            // This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
            executeParallelCommands(args);
        }
        else if(c == 2) 
        {
            // 	executeSequentialCommands();	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
            executeSequentialCommands(args);
        }
        else if(c == 3) 
        {
            // This function is invoked when user wants redirect output of a single command to and output file specificed by user
            executeCommandRedirection(args);
        }
        else 
        {
            // simple command
            executeCommand(args);
        }
				
	}
	return 0;
}
