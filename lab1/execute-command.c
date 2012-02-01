// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <sys/types.h>  /* pid_t */
#include <sys/wait.h> /* waitpid */
#include <unistd.h>  /* _exit, fork */
#include <stdlib.h>  /* exit */
#include <stdio.h>
#include <error.h>
#include <errno.h>

#include <fcntl.h>			// File Control Options
#include <sys/stat.h>		// File Permission Options

int
command_status (command_t c)
{
  return c->status;
}

void execute_generic(command_t c)
{
  switch(c->type)
	{
    case AND_COMMAND:
      execute_and(c);
      break;
    case OR_COMMAND:
      execute_or(c);
      break;
    case SEQUENCE_COMMAND:
      execute_sequence(c);
    break;
    case PIPE_COMMAND:
      execute_pipe(c);
      break;
    case SIMPLE_COMMAND:
    case SUBSHELL_COMMAND:
      execute_io_command(c);
      break;
    default:
      error(1, 0, "Invalid command type");
	}
}

void execute_and(command_t c)
{
  execute_generic(c->u.command[0]);
  if(c->u.command[0]->status == 0)
  {
    // If the first command succeeds, it depends on the 2nd command
    execute_generic(c->u.command[1]);
    c->status = c->u.command[1]->status;
  }
  else
    c->status = c->u.command[0]->status;
}

void execute_or(command_t c)
{
  execute_generic(c->u.command[0]);
  if(c->u.command[0]->status != 0)
  {
    // If the first command fails, success depends on the 2nd command
    execute_generic(c->u.command[1]);
    c->status = c->u.command[1]->status;
  }
  else
    c->status = c->u.command[0]->status;
}

void execute_sequence(command_t c)
{
  int status;
  pid_t pid = fork();
  if(pid > 0)
  {
    // Parent process
    waitpid(pid, &status, 0);
    c->status = status;
  }
  else if(pid == 0)
  {
    //Child process
    pid = fork();
    if( pid > 0)
    {
      // The child continues
      waitpid(pid, &status, 0);
      execute_generic(c->u.command[1]);
      _exit(c->u.command[1]->status);
    }
    else if( pid == 0)
    {
      // The child of the child now runs
      execute_generic(c->u.command[0]);
      _exit(c->u.command[0]->status);
    }
    else
      error(1, 0, "Could not fork");
  }
  else
    error(1, 0, "Could not fork");
}


// If a command has particular input/output characteristics,
// this function will open and redirect the appropriate i/o locations
void setup_io(command_t c)
{
	// Check for an input characteristic, which can be read
	if(c->input != NULL)
	{
		int fd_in = open(c->input, O_RDWR);
		if( fd_in < 0)
			error(1, 0, "Unable to read input file: %s", c->input);
		
		if( dup2(fd_in, 0) < 0)
			error(1, 0, "Problem using dup2 for input");
		
		if( close(fd_in) < 0)
			error(1, 0, "Problem closing input file");
	}
	
	// Check for an output characteristic, which can be read and written on,
	// and if it doesn't exist yet, should be created
	if(c->output != NULL)
	{
		// Be sure to set flags 
		int fd_out = open(c->output, O_CREAT | O_WRONLY | O_TRUNC, 
											S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
		if( fd_out < 0)
			error(1, 0, "Problem reading output file: %s", c->output);
		
		if( dup2(fd_out, 1) < 0)
			error(1, 0, "Problem using dup2 for output");
		
		if( close(fd_out) < 0)
			error(1, 0, "Problem closing output file");
	}	
}


void execute_simple(command_t c)
{
  int status; 
	pid_t pid = fork();
	
	if(pid > 0)
	{
		// Parent waits for child, then stores status
		waitpid(pid, &status, 0);
		c->status = status;
	}
	else if(pid == 0)
	{
		// Set input-output characteristics
		setup_io(c);
		
		// In a semicolon simple command, exit as fast as possible
		if(c->u.word[0][0] == ':')
			_exit(0);
		
		// Execute the simple command program
		execvp(c->u.word[0], c->u.word );
		error(1, 0, "Invalid simple command");
	}
	else
		error(1, 0, "Could not fork");
}


void execute_io_command(command_t c)
{

	if(c->type == SIMPLE_COMMAND)
	{
		execute_simple(c);
	}
	else if(c->type == SUBSHELL_COMMAND)
	{
		// Set input-output characteristics
		setup_io(c);
		
		// Execute contents of subshell
		execute_generic(c->u.subshell_command);
	}
	else
		error(1,0, "Error with processing i/o command");
}

void
execute_pipe (command_t c)
{
  int status;
  int buf[2];
  pid_t returned_pid;
  pid_t pid_1;
  pid_t pid_2;
  
  if ( pipe(buf) == -1 )
      error (1, errno, "cannot create pipe");
  pid_1 = fork();
  if( pid_1 > 0 )
  {
    // Parent process
    pid_2 = fork();
    if( pid_2 > 0 )
    {
      //The parent doesn't need any of the pipe
      close(buf[0]);
      close(buf[1]);
      // Wait for any process to finish
      returned_pid = waitpid(-1, &status, 0);
      if( returned_pid == pid_1 )
      {
        c->status = status;
        // kill the other process?
        waitpid(pid_2, &status, 0);
        return;
      }
      else if(returned_pid == pid_2)
      {
        waitpid(pid_1, &status, 0);
        c->status = status;
        return;
      }
    }
    else if( pid_2 == 0 )
    {
      // The 2nd child now runs, first part of the pipe
      close(buf[1]);
      if( dup2(buf[0], 1) == -1 )
        error (1, errno,  "dup2 error");
      execute_generic(c->u.command[0]);
      _exit(c->u.command[0]->status);
    }
    else
      error(1, 0, "Could not fork");
  }
  else if( pid_1 == 0)
  {
    // First child, command 2nd in the pipe
    close(buf[0]);
      if( dup2(buf[1], 0)== -1 )
        error (1, errno,  "dup2 error");
      execute_generic(c->u.command[1]);
      _exit(c->u.command[1]->status);
  }
  else
    error(1, 0, "Could not fork");
  
	
	
	
	
	
  /*
  if(pid > 0)
  {
    // Parent process
    waitpid(pid, &status, 0);
    c->status = status;
  }
  else if(pid == 0)
  {
    //Child process
    if (pipe(buf) == -1)
      error (1, errno, "cannot create pipe");
    pid = fork();
    if( pid > 0)
    {
      // The child continues
      close(buf[0]);
      if( dup2(buf[1], stdin)== -1 )
        error (1, errno,  "dup2 error");
      execute_generic(c->u.command[1]); //TODO: Make sure interface is correct
      _exit(c->u.command[1]->status);
    }
    else if( pid == 0)
    {
      // The child of the child now runs, first part of the pipe
      close(buf[1]);
      if( dup2(buf[0], stdout) == -1 )
        error (1, errno,  "dup2 error");
      execute_generic(c->u.command[0]);
      _exit(c->u.command[0]->status);
    }
    else
      error(1, 0, "Could not fork");
  }
  else
    error(1, 0, "Could not fork");
	 */
}
	 
void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
     
  if(time_travel == 1)
    error (1, 0, "timetravel command execution not yet implemented");
  else
    execute_generic(c);
}
