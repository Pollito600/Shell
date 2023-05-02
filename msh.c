// The MIT License (MIT)
// 
// Copyright (c) 2016 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size
#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports four arguments
#define MAX_HISTORY_SIZE 15

void trim(char *str) 
{
  int l = strlen(str);

  if (str[l - 1] == '\n') 
  {
    str[l - 1] = 0;
  }
}

void add_history(char history[][MAX_COMMAND_SIZE], int *history_count, char *working_string) 
{
  //if history is full, data is shifted over popping off the oldest entry
  if (*history_count == MAX_HISTORY_SIZE) 
  {
    for (int i = 1; i < MAX_HISTORY_SIZE; i++) 
    {
      strcpy(history[i - 1], history[i]);
    }

    strcpy(history[MAX_HISTORY_SIZE - 1], working_string);
  }

  //if history_count is less than 15, working_string (command) 
  //is copied over to history, counter is then incremented 
  else
  {
    strcpy(history[*history_count], working_string);
    (*history_count)++;
  }
}

int main(int argc, char **argv)
{
  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );
  char history[MAX_HISTORY_SIZE][MAX_COMMAND_SIZE];
  char temp[MAX_COMMAND_SIZE]; 

  //initializes array of pid_t to zeros
  pid_t pid_history[MAX_HISTORY_SIZE];
  memset(pid_history, 0, sizeof(pid_history));

  int history_count = 0;
  int space = 0;
  int neg = 0;
 
  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the command line.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );
    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup( command_string );                

    //adds command entry to history
    add_history(history, &history_count, working_string);

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Ignores whitespace entries
    if (token[0] != NULL)
    {
      //checks is command entered has a "!"
      if (strncmp(token[0], "!", 1) == 0)
      {
        //skips "!" and turns the character number into an integer stored in index
        int index = atoi(token[0] + 1);

        //if index is less than 0 or greater than or equal to history_count,
        // print invalid statement
        if (index < 0 || index >= history_count)
        {
          printf("Invalid history index: %d\n", index);
        }

        else
        {
          //uses index value to copy history command into a temp array
          strcpy(temp, history[index]);

          //replaces the "!n" entry with the associated command in history
          strcpy(history[history_count - 1], history[index]);

          //removes "\n"
          trim(temp);

          //looks for any spaces in the temp array
          for (int i = 0; i < strlen(temp); i++)
          {
            if (temp[i] == ' ')
            {
              space = 1;
              
              break;
            }
          }

          //if there is a space, then there must be 2 or more arguments
          if (space == 1)
          {
            //initializes a pointer to the first token in temp using the delimiter " "
            char *ptr = strtok(temp, " ");
            int i = 0;

            //loops through each token and add it to the token array
            while (ptr != NULL && i < 2)
            {
              token[i] = (char *)malloc(strlen(ptr) + 1);

              //copies the current token into the token array
              strcpy(token[i], ptr);

              // move the pointer to the next token
              ptr = strtok(NULL, " ");

              i++;
            }
          }

          //must be 1 argument 
          else
          {
            strcpy(token[0], temp);
          }
        }
      }

      //Checks if input was "exit" or "quit" to close the program
      if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)
      {
        exit(0);
      }

      //Checks if command "history" was inputted then prints current history 
      else if (strcmp(token[0], "history") == 0)
      {
        //pid is assigned a negative -1
        neg = -1;

        pid_history[history_count - 1] = neg;

        //checks if there is no second argument
        if (token[1] == NULL)
        {
          for (int i = 0; i < history_count; i++) 
          {
            printf("%d: %s", i, history[i]);
          }
        }

        //checks is the second argument is "-p" and prints the associated PID
        else if (strcmp(token[1], "-p") == 0)
        {
          for (int i = 0; i < history_count; i++)
          {
            printf("[%d]: [%2d] %s", i, pid_history[i], history[i]);
          }
        }

        continue;
      }

      //Checks if command "cd" was inputted then changes directory to its 
      //second argument if successful 
      else if (strcmp(token[0], "cd") == 0)
      {
        //pid is assigned a negative -1
        neg = -1;

        if (chdir(token[1]) == -1)
        {
          printf("\nDirectory not found.\n");
        }

        pid_history[history_count - 1] = neg;

        continue;
      }

      //forks a new process 
      pid_t pid = fork();

      if (pid < 0)
      {
        printf("\nFork failed\n");

        exit(1);

      }

      else if (pid == 0)
      {
        //command is passed to execvp and executes
        //if execvp fails then print statement and exit are executed
        if (execvp(token[0], token) == -1)
        {
          printf("%s: Command not found.\n", token[0]);
        }

        exit(EXIT_FAILURE);
      }

      else
      {
        int status;

        //stores pid in an array of pids
        pid_history[history_count - 1] = pid;

        waitpid(pid, &status, 0);
      }
    }

    // Cleanup allocated memory
    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      if( token[i] != NULL )
      {
        free( token[i] );
      }
    }

    free( head_ptr );

  }

  free( command_string );

  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}
