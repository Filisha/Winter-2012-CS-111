////////////////////////////////////////////////////////////////////////////////
// UCLA CS 111 Lab 1 command reading
////////////////////////////////////////////////////////////////////////////////

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Increase size allocation by 50 bytes
void command_stream_reallocate(command_stream_t cmd_stream)
{
  cmd_stream->max_token_length += 50;
  
  cmd_stream->next_token_string = 
  checked_realloc(  cmd_stream->next_token_string, 
                    cmd_stream->max_token_length * sizeof(char));
  
  cmd_stream->current_token_string = 
  checked_realloc(  cmd_stream->current_token_string, 
                    cmd_stream->max_token_length * sizeof(char));
  
/*
  if( !cmd_stream->next_token_string || 
     !cmd_stream->current_token_string   )
  {
    error(1, 0, "%d: Unable to allocate memory for long word", 
          cmd_stream->line_number);
  }
*/
}

int get_next_char(command_stream_t cmd_stream) 
{
  int current_char;
  
  // If there is no character stored up, get one from the command stream
  if(cmd_stream->next_char == -2)
    current_char = cmd_stream->getbyte(cmd_stream->arg);
  // Otherwise, use the stored character, and mark it as used
  else 
  {
    current_char = cmd_stream->next_char;
    cmd_stream->next_char = -2;
  }
  
  return current_char;
}

// Store a character in a previous character buffer
void unget_char(int unwanted, command_stream_t cmd_stream)
{
  cmd_stream->next_char = unwanted;
}

// Check if the character is a valid part of a WORD
int is_valid_word_char(int current_char)
{
  return (current_char >= '0' && current_char <= '9')
          || (current_char >= 'A' && current_char <= 'Z')
          || (current_char >= 'a' && current_char <= 'z')
          || current_char == '!'
          || current_char == '%' || current_char == '+' 
          || current_char == ',' || current_char == '-' 
          || current_char == '.' || current_char == '/'
          || current_char == ':' || current_char == '@' 
          || current_char == '^' || current_char == '_';
}

// Tokenization, the lexer
// Produces the next valid token, or END if there are no tokens remaining
enum token_type read_next_token(command_stream_t cmd_stream)
{
  char* next_token_string = cmd_stream->next_token_string;
  
  // Move the next token up to current, then read a new one
  strcpy(cmd_stream->current_token_string, next_token_string);
  cmd_stream->current_token = cmd_stream->next_token;
  next_token_string[0] = 0;
  
/*fprintf(stderr, "\nCurrent token(%d): %s", cmd_stream->current_token, 
          cmd_stream->current_token_string);
*/

  // That this loop finds the value that will be returned the next time
  // read_next_token is called
  int current_char;
  int index = 0;
  while( (current_char = get_next_char(cmd_stream) ) )
  {
    // Negative return from get_next_char means no more input
    if(current_char == EOF || current_char < 0)
    {
      cmd_stream->next_token = END;
      break;
    }
       
    // Ignore any leading whitespace
    else if(current_char == ' ' || current_char == '\t')
      continue;

    // Remove any comment text up to, but not including, the next newline
    else if(current_char == '#')
    {
      while((current_char = get_next_char(cmd_stream)) != '\n')
        continue; //Ignore until a newline
      //Don't consume newline
      unget_char(current_char, cmd_stream);
    }
    
    // Form a WORD token from valid WORD characters
    else if(is_valid_word_char(current_char))
    {
      next_token_string[index] = current_char;
      index++;
      
      // Consume all word characters to form the word until a seperator
      while((current_char = get_next_char(cmd_stream)))
      {
        
        // If the character is no longer one of the valid WORD characters,
        // return that character, and stop
        if(!is_valid_word_char(current_char))
        {
          unget_char(current_char, cmd_stream);
          break;
        }
        
        // Grow allocated memory when necessary. Size of next_token_string 
        // and current_token_string are equal because all data in 
        // next_token_string passes into current_token_string        
        if(index >= cmd_stream->max_token_length)
        {
          command_stream_reallocate(cmd_stream);
          next_token_string = cmd_stream->next_token_string;
        }
        
        // Add the array forming the next token
        next_token_string[index] = current_char;
        index++;
        
      }

      // Allocate memory if needed, then add the zero byte
      if(index >= cmd_stream->max_token_length)
      {
        command_stream_reallocate(cmd_stream);
        next_token_string = cmd_stream->next_token_string;
      }
      next_token_string[index] = 0;     
      cmd_stream->next_token = WORD;
      break;
    }
    
    // Check for &&
    else if(current_char == '&')
    {
      next_token_string[index++] = current_char;  
      if((current_char = get_next_char(cmd_stream)) != '&')
      {
        error(1, 0, "%d: Found lone &, invalid character", 
              cmd_stream->line_number);
        break;
      }
      
      next_token_string[index++] = current_char;     

      //Add the end zero byte
      next_token_string[index] = 0;     
      cmd_stream->next_token = D_AND;
      break;
    }
    
    // Differentiate between a PIPE (|) and a D_OR (||)
    else if(current_char == '|')
    {
      next_token_string[index++] = current_char;  
      if((current_char = get_next_char(cmd_stream)) == '|')
      {
        next_token_string[index++] = current_char;     
        next_token_string[index] = 0;
        cmd_stream->next_token = D_OR;
        break;
      }
      unget_char(current_char, cmd_stream);     
      next_token_string[index] = 0;     
      cmd_stream->next_token = PIPE;
      break;
    }
    
    else if(current_char == '(')
    {
      next_token_string[index++] = current_char;
      next_token_string[index] = 0;     
      cmd_stream->next_token = LEFT_PARAN;
      break;
    }
    
    else if(current_char == ')')
    {
      next_token_string[index++] = current_char;
      next_token_string[index] = 0;     
      cmd_stream->next_token = RIGHT_PARAN;
      break;
    }
    
    else if(current_char == '<')
    {
      next_token_string[index++] = current_char;
      next_token_string[index] = 0;     
      cmd_stream->next_token = LESS;
      break;
    }
    
    else if(current_char == '>')
    {
      next_token_string[index++] = current_char;
      next_token_string[index] = 0;     
      cmd_stream->next_token = GREATER;
      break;
    }
    
    else if(current_char == '\n')
    {
      cmd_stream->line_number++;
      
      // Ignore any subsequent newlines, but keep line count
      while((current_char = get_next_char(cmd_stream)) == '\n')
      {
        cmd_stream->line_number++;
      }
        
      if(current_char == EOF)
      {
        cmd_stream->next_token = END;
        break;
      }
      unget_char(current_char, cmd_stream);
      
      next_token_string[index++] = ' ';
      next_token_string[index] = 0;     
      cmd_stream->next_token = NEWLINE;
      break;
    }
    
    else if(current_char == ';')
    {
      next_token_string[index++] = current_char;
      next_token_string[index] = 0;     
      cmd_stream->next_token = SEMICOLON;
      break;
    }
    
    else
    {
      error(1, 0, "%d: Unrecognized or out of place character", 
            cmd_stream->line_number);
      break;
    }
  }
  
  return cmd_stream->current_token;
}

// Peek at next token without fetching it
enum token_type check_next_token(command_stream_t s)
{
  return s->next_token;
}

// Parse sequence commands
command_t complete_command (command_stream_t s)
{
  command_t first_c = and_or(s);
  
  while(check_next_token(s) == NEWLINE || check_next_token(s) == SEMICOLON)
  {
    read_next_token(s);
    if(check_next_token(s) == END)
      break;
    
    command_t sequence_b = and_or(s);
    
    command_t seq_com = checked_malloc(sizeof(struct command));
    
    //sequence command with and_or_c plus the command sequence_b
    seq_com->type = SEQUENCE_COMMAND;
    seq_com->status = -1;
    seq_com->input = 0;
    seq_com->output = 0;
    seq_com->u.command[0] = first_c;
    seq_com->u.command[1] = sequence_b;
    first_c = seq_com;
  }
  
  return first_c;
}

// Parse left-associative and_or clauses
command_t and_or (command_stream_t s)
{
  command_t first_c = pipeline(s);
  
  while(check_next_token(s) == D_AND || check_next_token(s) == D_OR)
  {
    enum token_type curr_token = read_next_token(s);
    command_t and_or_b = pipeline(s);
    command_t next_com = checked_malloc(sizeof(struct command));
      
    if(curr_token == D_AND)
      next_com->type = AND_COMMAND;
    else if(curr_token == D_OR)
      next_com->type = OR_COMMAND;
    next_com->status = -1;
    next_com->input = 0;
    next_com->output = 0;
    next_com->u.command[0] = first_c;
    next_com->u.command[1] = and_or_b;
    first_c = next_com;
  }
  
  return first_c;
}

// Parse pipeline commands
command_t pipeline (command_stream_t s)
{
  command_t first_c = command_parse(s);
  
  while(check_next_token(s) == PIPE)
  {
    read_next_token(s);
    command_t pipe_b = command_parse(s);
    command_t pipe_com = checked_malloc(sizeof(struct command));
    
    //the pipe command composed of a command | another pipe command
    pipe_com->type = PIPE_COMMAND;
    pipe_com->status = -1;
    pipe_com->input = 0;
    pipe_com->output = 0;
    pipe_com->u.command[0] = first_c;
    pipe_com->u.command[1] = pipe_b;
    first_c = pipe_com;
  }
  return first_c;
}

// General command parser
command_t command_parse (command_stream_t s)
{
  // Optional newlines as white space may appear before a simple command 
  // or the ( of a subshell
  // Loop multiple times to eliminate all newlines, which may occur when
  // using multiple lines of comments
  while(check_next_token(s) == NEWLINE)
  {
    read_next_token(s);
  }

  //If a ( is coming up, it's a subshell command
  if(check_next_token(s) == LEFT_PARAN)
  {
    command_t subshell_c = subshell(s);
    
    if(check_next_token(s) == LESS)
    {
      // Read in the <
      read_next_token(s);
      // Attempt to read in the input argument
      if(read_next_token(s) != WORD)
      {
        error(1, 0, "%d: Could not read script, expected input argument", 
              s->line_number);
      }
      
      subshell_c->input = checked_malloc(strlen(s->current_token_string) + 1);
      strcpy(subshell_c->input, s->current_token_string);
    }
    
    if(check_next_token(s) == GREATER)
    {
      // Read in the >
      read_next_token(s);
      // Attempt to read in the output argument
      if(read_next_token(s) != WORD)
      {
        error(1, 0, "%d: Could not read script, expected output argument", 
              s->line_number);
      }
      
      subshell_c->output = checked_malloc(strlen(s->current_token_string) + 1);
      strcpy(subshell_c->output, s->current_token_string);
    }
    
    return subshell_c;
  }
  
  else
  {
    command_t simple_c = simple_command(s);
    if(check_next_token(s) == LESS)
    {
      // Read in the <
      read_next_token(s);
      // Attempt to read in the input argument
      if(read_next_token(s) != WORD)
      {
        error(1, 0, "%d: Could not read script, expected input argument", 
              s->line_number);
      }
      
      simple_c->input = checked_malloc(strlen(s->current_token_string) + 1);
      strcpy(simple_c->input, s->current_token_string);
    }
    
    if(check_next_token(s) == GREATER)
    {
      // Read in the >
      read_next_token(s);
      // Attempt to read in the output argument
      if(read_next_token(s) != WORD)
      {
        error(1, 0, "%d: Could not read script, expected output argument", 
              s->line_number);
      }
      
      simple_c->output = checked_malloc(strlen(s->current_token_string) + 1);
      strcpy(simple_c->output, s->current_token_string);
    }
    return simple_c;
  }
}

// Parse subshell
command_t subshell (command_stream_t s)
{
  if(read_next_token(s) != LEFT_PARAN)
  {
    error(1, 0, "%d: Could not read script, expected left parathesis", 
          s->line_number);
  }
  command_t inner_com = complete_command(s);
  
  //Optional newlines as white space may appear before ) of the subshell
  if(check_next_token(s) == NEWLINE)
  {
    read_next_token(s);
  }
  
  if(read_next_token(s) != RIGHT_PARAN)
  {
    error(1,0, "Line %d, Could not read script, expected right paranthesis", 
          s->line_number);
  }
  
  command_t shell_command = checked_malloc(sizeof(struct command));
    
  shell_command->type = SUBSHELL_COMMAND;
  shell_command->status = -1;
  shell_command->input = 0;
  shell_command->output = 0;
  shell_command->u.subshell_command = inner_com;
  
  return shell_command;
}

// Parses simple command
command_t simple_command (command_stream_t s)
{
  int index = 0;
  int array_max = 50;
  enum token_type next;
  if(read_next_token(s) != WORD)
  {
    error(1, 0, "%d: Could not read script, expected command, found: %s", 
          s->line_number, s->current_token_string);
  }
  
  // Allocate inital space for array of string pointers
  // Inital size of 50 words
  char** words_array = checked_malloc(sizeof(char*) * 50); 
  
  //Allocate the string's space
  words_array[index] = checked_malloc(strlen(s->current_token_string) + 1);
  strcpy(words_array[index], s->current_token_string);
  index++;
  
  // Keep retrieving simple commands
  while(check_next_token(s) == WORD)
  {
    read_next_token(s);
    words_array[index] = checked_malloc(strlen(s->current_token_string) + 1);
    strcpy(words_array[index], s->current_token_string);
    index++;

    //If the number of words becomes to long, reallocate space  
    if(index >= array_max)
    {
      array_max += 50;
      words_array = checked_realloc(words_array, array_max);

/*    if(words_array == NULL)
        error(1, 0, 
              "%d: Unable to allocate memory for number of words in command", 
              s->line_number);
*/
    }
  }

  //End with a 0 byte
  words_array[index] = 0;     
    
  command_t simple_c = checked_malloc( sizeof(struct command) );
    
  simple_c->type = SIMPLE_COMMAND;
  simple_c->status = -1;
  simple_c->input = 0;
  simple_c->output = 0;
  simple_c->u.word = words_array;
  
  return simple_c;
}

command_stream_t make_command_stream (int (*get_next_byte) (void *),
                                      void *get_next_byte_argument    )
{
  command_stream_t cmd_stream = checked_malloc( sizeof(struct command_stream) );
  cmd_stream->getbyte = get_next_byte;
  cmd_stream->arg = get_next_byte_argument;

  // Invalid next char is -2
  cmd_stream->next_char = -2;
  cmd_stream->line_number = 1;
  
  // Allocate inital size for token string arrays
  cmd_stream->next_token_string = checked_malloc ( 50 * sizeof(char) );
  cmd_stream->next_token_string[0] = 0;
  cmd_stream->current_token_string = checked_malloc ( 50 * sizeof(char) );
  cmd_stream->current_token_string[0] = 0;
  cmd_stream->max_token_length = 50;  //50 characters initially available
  
  return cmd_stream;
}

command_t read_command_stream (command_stream_t s)
{
  if(check_next_token(s) == END)
    return NULL;
  read_next_token(s);
  command_t result = and_or(s);
  return result;
}
