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

int
command_status (command_t c)
{
  return c->status;
}

void execute_generic(command_t c)
{
  switch(c->command_type)
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

void execute_and(command_t c)
{
  execute_generic(c->u.command[0]);
  if(c->u.command[0]->status == 0)
  {
    // If the first command succeeds, it depends on the 2nd command
    execute_generic(c->u.command[1]);
    c->status = u.command[1]->status;
  }
  else
    c->status = u.command[0]->status;
  
}

void execute_or(command_t c)
{
  execute_generic(c->u.command[0]);
  if(c->u.command[0]->status != 0)
  {
    // If the first command fails, success depends on the 2nd command
    execute_generic(c->u.command[1]);
    c->status = u.command[1]->status;
  }
  else
    c->status = u.command[0]->status;
  
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

void execute_io_command(command_t c);

void
execute_pipe (command_t c)
{
  int status;
  int buf[2];
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
  
  
}

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (1, 0, "command execution not yet implemented");
}
