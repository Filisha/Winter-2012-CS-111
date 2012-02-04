// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

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
      close(buf[0]);
      if( dup2(buf[1], 1) == -1 )
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
    close(buf[1]);
      if( dup2(buf[0], 0)== -1 )
        error (1, errno,  "dup2 error");
      execute_generic(c->u.command[1]);
      _exit(c->u.command[1]->status);
  }
  else
    error(1, 0, "Could not fork");
}


////////////////////////////////////////////////////////////////////////////////
// Time Travel Functions
////////////////////////////////////////////////////////////////////////////////

// Given a word node and a char*, this will add the word node to the linked list
void add_word_dependencies(word_node_t word_list, char* word)
{
	if(word_list == NULL)
	{
		word_list = checked_malloc(sizeof(word_node_t));
		word_list->word = word;
		word_list->next = NULL;
	}
	else
	{
		add_word_dependencies(word_list->next, word);
	}
}

// Given a top level command node and a command, this function will add all of 
// the dependencies of the command to the node, and then will also recursively 
// add all of the dependencies of the command's subcommands to the node
void add_command_dependencies(tlc_node_t node, command_t cmd)
{
	if(cmd->input != NULL)
		add_word_dependencies(node->inputs, cmd->input);
		
	if(cmd->output != NULL)
		add_word_dependencies(node->outputs, cmd->output);
	
	int k;
	switch(cmd->type)
	{
		// AND, OR, SEQUENCE, and PIPE all have two subcommands
		case AND_COMMAND:
		case OR_COMMAND:
		case SEQUENCE_COMMAND:
		case PIPE_COMMAND:
			add_command_dependencies(node, cmd->u.command[0]);
			add_command_dependencies(node, cmd->u.command[1]);
			break;
			
		// SUBSHELL has a single subcommand
		case SUBSHELL_COMMAND:
			add_command_dependencies(node, cmd->u.subshell_command);
			break;
			
		// SIMPLE has no additional subcommands, but more input dependencies
		case SIMPLE_COMMAND:
			k = 1;
			while(cmd->u.word[k] != NULL)
			{
				add_word_dependencies(node->inputs, cmd->u.word[k]);
				k++;
			}
			break;
	}
}

void add_to_tlc_list(depend_node_t depend_list, tlc_node_t addition)
{
  depend_node_t last_node = depend_list;
  while(depend_list != NULL)
  {
    last_node = depend_list;
    depend_list = depend_list->next;
  }
  last_node->next = addition;
}

command_t
execute_time_travel (command_stream_t s)
{
  tlc_node_t graph_head = NULL;

  command_t last_command = NULL;
  command_t command;
  while ((command = read_command_stream (s)))
  {
    tlc_node_t new_node = checked_malloc(sizeof(struct tlc_node));
    new_node->c = command;
    new_node->inputs = NULL;
    new_node->outputs = NULL;
    new_node->dependencies = 0;
    new_node->dependents = NULL;
    new_node->pid = -1;
    
    generate_dependencies(new_node, command);  //Stick a dependency list in the node
    
    // For all on the list, walk through all even if there one is found
    tlc_node_t last_node = graph_node;
    tlc_node_t curr_node = graph_node;
    while(curr_node != NULL)
    {
      //If dependency is found
      
      word_node_t curr_input;
      word_node_t curr_output;
      
      curr_output = new_node-> outputs;
      //Find WAR
      while(curr_output != NULL)
      {
        curr_input = curr_node->inputs;
        while(curr_input != NULL)
        {
          if(strcmp(curr_input->word, curr_output->word))
          {
              // Note that on the waiting list, that is, this command is a dependent for another
              add_to_tlc_list( curr_node->dependents, new_node);
              break;
          }
          
          curr_input = curr_input->next;
        }
        
        curr_output = curr_output->next;
      }
      
      curr_input = new_node->inputs;
      curr_output = curr_node-> outputs;
      // Find RAW
       while()
      {
        if()
        {
            // Note that on the waiting list, that is, this command is a dependent for another
        }
      }
      
      
      last_node = curr_node;
      curr_node = curr_node->next;
    }
    
    // Add the node to the list of executing and waiting commands
    
    
    last_command = command;
  }
  
  // While there's someone on the waiting list
  while(graph_head != NULL)
  {
    // For everyone on the list
      // If they're not waiting on anyone
        //fork and execute, indicate its pid
        
    // Wait for somone to finish
    // Use pid to determine who finished and remove them
    // for all on the list of dependents
      // free that depenenct (reduce dependencies)
  }
  
  return last_command;
}

void
execute_command (command_t c)
{
    execute_generic(c);
}
