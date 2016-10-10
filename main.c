#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINE 2048

// global variables for determining which type of operation we will be performing
int iRedirectStdOut = 0, iAppendStdOut = 0, iRedirectStdin = 0, iAddittionalPipe = 0, iPipeTokenPosition = 0, iTokenCount = 0;

char szInputFileName[MAX_LINE] = {0};
char szOutputFileName[MAX_LINE] = {0};
char szFinalCommand[MAX_LINE] = {0};


// Function does the following:
// 1) determines what kind of redirect we want to do
// 2) assembles the UNIX instruction the user wants to execute
// 3) gets the target output filename
void AnalyzeArguments(char *szArgs[], int iCurrArgCount)
{
	// base case terminator
	if (szArgs[iCurrArgCount] == NULL)
	{
		// no more arguments to process
		return;
	}

	// We will redirect the STDOUT from a command to a file
	if (strcmp(szArgs[iCurrArgCount], ">") == 0)
	{
		printf("\n[+] We will redirect the STDOUT from a command to a file and overwrite the entire file if it exists already (no append)\n");
		iRedirectStdOut = 1;
		strcat(szOutputFileName, szArgs[iCurrArgCount + 1]);
		// argv[iCurrArgCount + 1] will be the output file name
	}

	// check if we want to APPEND standard output from a command to a file if the file exists
	else if (strcmp(szArgs[iCurrArgCount], ">>") == 0)
	{
		printf("\n[+] We will redirect the STDOUT from a command to a file and APPEND the data\n");
		iAppendStdOut = 1;
		strcat(szOutputFileName, szArgs[iCurrArgCount + 1]);
		// argv[iCurrArgCount + 1] will be the output file name
	}

	// check for redirect of stdin from file
	else if (strcmp(szArgs[iCurrArgCount], "<") == 0)
	{
		printf("\n[+] We will redirect stdin to file instead of keyboard\n");
		iRedirectStdin = 1;
		strcat(szInputFileName, szArgs[iCurrArgCount + 1]);
	}

	// check for addittional pipe request from user
	else if (strcmp(szArgs[iCurrArgCount], "|") == 0)
	{
		printf("\n[+] The user has requested additional pipe operations\n");
		iAddittionalPipe = 1;

		// save the position of the new pipe request so we can build the next command the user wants to execute later
		iPipeTokenPosition = iCurrArgCount;

		// we can build it with a for loop here or later
	}

	// otherwise just build the argument
	else
	{
		// everything before ">", ">>", "<", ect is the unix command we want to execute
		if (!iRedirectStdOut && !iAppendStdOut && !iRedirectStdin)
		{
			// tokenize the unix command here also so we know when to pass params
			//sprintf(szFinalCommand, "%s ", szArgs[iCurrArgCount]);
			strcat(szFinalCommand, szArgs[iCurrArgCount]);
			strcat(szFinalCommand, " ");
		}
	}

	//printf("szFinalCommand = [%s]\n", szFinalCommand);

	return AnalyzeArguments(szArgs, ++iCurrArgCount);
}


int main (int argc, char * argv[])
{
	char szInputFileBuffer[MAX_LINE] = {0};
	char szShellCommand[MAX_LINE] = {0};
	char **szTokens;
	char **szTokenizedCommand;

	int iTokenPos = 0;
	int iInitArray;

	// relevant to analyzing the arguments in AnalyzeArguments()
	int iArgPosition = 0, iCurrArgPosition = 0;

	// our file descriptor
    int iFileDescriptor[2];

	FILE *fReadFile;

	// "The pid_t data type is a signed integer type which is capable of representing a process ID."
	// pid for ls
	pid_t pProcessOne;

	// pid for sort
	pid_t pProcessTwo;

	printf("** Enter the commands you'd like to execute\n");

	// get the commands we want to execute from the user
	fgets(szShellCommand, sizeof(szShellCommand), stdin);

	printf("[+] Recieved Command: %s\n", szShellCommand);

	// allocate memory for tokenized commands
	szTokenizedCommand = (char **)malloc(MAX_LINE * sizeof(char *));
	szTokens = (char **)malloc(MAX_LINE * sizeof(char *));

	for (iInitArray = 0; iInitArray < MAX_LINE; iInitArray++)
	{
		szTokenizedCommand[iInitArray] = (char *)malloc(256 * sizeof(char));
		szTokens[iInitArray] = (char *)malloc(256 * sizeof(char));
	}

	// string tokenize the input we just got by space as the delimiter - get the first token
	szTokenizedCommand = strtok(szShellCommand, " ");

	// get the other tokens
	while(szTokenizedCommand != NULL)
	{
		szTokens[iTokenCount] = szTokenizedCommand;

		// global counter for number of tokens
		iTokenCount++;

		szTokenizedCommand = strtok(NULL, " ");
	}

	printf("[+] Tokenized the commands, %d tokens total\n", iTokenCount);

	/* DEBUG: check the commands we tokenized
	for (iTokenPos = 0; iTokenPos < iTokenCount; iTokenPos++)
	{
		printf("szTokens[%d] = %s\n", iTokenPos, szTokens[iTokenPos]);
	}
	*/

	printf("\n[+] Begin analyzing your command\n", iTokenCount);

	// analyze the arguments we just tokenized
	AnalyzeArguments(szTokens, iCurrArgPosition);

	printf("\n[+] Finished analyzing your command\n", iTokenCount);
	printf("\n[+] Parsed Information:\n\t**UNIX Command: %s\n", szFinalCommand);

	if (iRedirectStdin)
	{
		printf("\t**Input File: %s\n", szInputFileName);
	}

	if (iRedirectStdOut || iAppendStdOut)
	{
		printf("\t**Output File: %s\n", szOutputFileName);
	}

    // first try to create a pipe
    if (pipe(iFileDescriptor) == -1)
    {
		// failed to make the pipe
		fprintf(stderr, "Pipe failed");
		return 1;
    }

	else
	{
		printf("[+] Successfully created pipe\n");
	}

	// create the second child process to execute 'sort' first because we will be passing the data from ls into here
	pProcessTwo = fork();

	// negative pid means that an error occurred
	if (pProcessTwo < 0)
	{
		perror("Fork failed\n");
		exit(-1);
	}


	// we want the sort child process to wait for the ls child and ls parent process to complete first, and then execute afterwards
	else if (pProcessTwo == 0)
	{
		char szPipeBuffer[2048] = {0};

		if (!iRedirectStdin)
		{
			read(iFileDescriptor[0], szPipeBuffer, 2048);
		}

		// at this point we want to check the global variables and do the appropriate operations
		// redirect stdout to a file (overwrite old files)
		if (iRedirectStdOut)
		{
			szOutputFileName[strlen(szOutputFileName) - 1] = 0;
			FILE *fOutputFile = fopen(szOutputFileName, "w");
			fprintf(fOutputFile, "%s\n", szPipeBuffer);
			fclose(fOutputFile);

			printf("\n[+] Successfully wrote stdout from PIPE to %s\n", szOutputFileName);
		}

		// redirect stdout to a file (append to old file, dont delete and write a new file)
		if (iAppendStdOut)
		{
			FILE *fOutputFile = fopen(szOutputFileName, "a");
			fprintf(fOutputFile, "%s\n", szPipeBuffer);
			fclose(fOutputFile);

			printf("\n[+] Successfully appended stdout from PIPE to %s\n", szOutputFileName);
			// we just write szPipeBuffer to the file here
		}

		// dup2(int oldfd, int newfd); -> "makes newfd be the copy of oldfd, closing newfd first if necessary, but note the following:"
		// make stdinput be the copy of iFileDescriptor[0]
        dup2(iFileDescriptor[0], 0);

		// close iFileDescriptor[0]
		close(iFileDescriptor[0]);

		// we're not using iFileDescriptor[1]
        close(iFileDescriptor[1]);

		// execute command if we redirected stdin, execute command with szPipeBuffer as the stdin
		if (iRedirectStdin)
		{
			// execute command
			system(szFinalCommand);
		}

		exit(0);
	}

	// After the forks, the original parent process waits for both child processes to finish before it terminates.
	else
	{
		// create the first child process
		pProcessOne = fork();

		// negative pid means that an error occurred
		if (pProcessOne < 0)
		{
			perror("Error: fork failed\n");
			exit(-1);
		}

		// child process two, but it executes first
		else if (pProcessOne == 0)
		{
			
			char szData[4096];

			szInputFileName[strlen(szInputFileName) - 1] = 0;

			// if we want to redirect stdin, just pass the stdin (data from file) to next process, and then execute the unix command
			// we identified earlier in that process
			if (iRedirectStdin)
			{
				FILE *fFile = fopen(szInputFileName, "rb");
				if (fFile != NULL)
				{
					size_t sLength = fread(szData, sizeof(char), 1024, fFile);
					if (sLength == 0)
					{
						fputs("Error reading file", stderr);
					}
					
					else
					{
						szData[++sLength] = '\0'; // set null terminator
					}

					fclose(fFile);
				}

				else
				{
					printf("fopen() failed, tried to open file [%s]\n", szInputFileName);
				}
			}

			// we want to redirect stdout to the next process
			dup2(iFileDescriptor[1], 1);

			// dont need iFileDescriptor[0] open
		    close(iFileDescriptor[0]);

			// dont need iFileDescriptor[1] open
			close(iFileDescriptor[1]);

			// if redirecting stdout
			if (!iRedirectStdin)
			{
				system(szFinalCommand);
				//execv(szFinalCommand, NULL);
				exit(0);
			}

			// if redirecting stdin
			printf("%s\n", szData);
		}

		// the parent process for ls
		else
		{
			// stores the return code of the child process: -1 failure, any other value means success
			int iChildReturnCode;

			// wait for the ls child process to completely execute - as shown in myshell.c
			waitpid(pProcessOne, &iChildReturnCode, 0);

			// check if the child thread terminated with an error
			if (iChildReturnCode == -1)
			{
				printf("The child process exited with an error\n");
			}
		}

		return 0;
	}

	return 0;
}
