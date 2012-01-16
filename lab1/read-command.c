// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int get_next_char(command_stream_t cmd_stream) 
{
  int current_char;
  // If there is no character stored up
  if(cmd_stream->next_char == -2)
    current_char = cmd_stream->getbyte(cmd_stream->arg);
  else {
    //Use the stored character if there is one
    current_char = cmd_stream->next_char;
    cmd_stream->next_char = -2;
    }
  return current_char;
}

void unget_char(int unwanted, command_stream_t cmd_stream)
{
  cmd_stream->next_char = unwanted;
}

//Check if the character is a valid part of a WORD
int is_valid_word_char(int current_char)
{
  return (current_char >= '0' && current_char <= '9')
          || (current_char >= 'A' && current_char <= 'Z')
          || (current_char >= 'a' && current_char <= 'z')
          || current_char == '!'
          || current_char == '%' || current_char == '+' || current_char == ','
          || current_char == '-' || current_char == '.' || current_char == '/'
          || current_char == ':' || current_char == '@' || current_char == '^'
          || current_char == '_';
}

// Tokenization, the lexer
enum token_type read_next_token(command_stream_t cmd_stream)
{
  char* next_token_string = cmd_stream->next_token_string;
  
  //Move the next token up to current, then read a new one
  strcpy(cmd_stream->current_token_string, next_token_string);
  cmd_stream->current_token = cmd_stream->next_token;
  next_token_string[0] = 0;
  
  //fprintf(stderr, "\nCurrent token(%d): %s", cmd_stream->current_token, cmd_stream->current_token_string);

  int current_char;
  int index = 0;
  while((current_char = get_next_char(cmd_stream)))
  {
    // Negative return from get_next_char means no more input
    if(current_char == EOF || current_char < 0)
    {
      cmd_stream->next_token = END;
      break;
    }
       
    // Remove leading whitespace
    else if(current_char == ' ' || current_char == '\t')
      continue;
    // If it's a comment
    else if(current_char == '#')
    {
      while((current_char = get_next_char(cmd_stream)) != '\n')
        continue; //Ignore until a newline
      //Don't consume newline
      unget_char(current_char, cmd_stream);
    }
    // If it's a WORD token
    else if(is_valid_word_char(current_char))
    {
      next_token_string[index] = current_char;
      index++;
      // Consume all word characters to form the word until a seperator
      while((current_char = get_next_char(cmd_stream)))
      {
        
        if(!is_valid_word_char(current_char))
        {
          unget_char(current_char, cmd_stream);
          break;
        }
        
        next_token_string[index] = current_char;
        index++;
        
        //TODO: Refactor to use professor's allocs
        if(index > cmd_stream->max_token_length)
        {
          cmd_stream->max_token_length += 50;
          // Since both next_token_string and current_token_string will hold this eventually
          // realloc both of them now
          cmd_stream->next_token_string = realloc(cmd_stream->next_token_string, cmd_stream->max_token_length * sizeof(char));
          next_token_string = cmd_stream->next_token_string;
          cmd_stream->current_token_string = realloc(cmd_stream->current_token_string, cmd_stream->max_token_length * sizeof(char));
          if(!cmd_stream->next_token_string || !cmd_stream->current_token_string)
          {
            error(1, 0, "%d: Unable to allocate memory for long word", cmd_stream->line_number);
          }
        }
      }
      next_token_string[index] = 0;     //Add the end zero byte
      cmd_stream->next_token = WORD;
      break;
    }
    else if(current_char == '&')
    {
      next_token_string[index++] = current_char;  
      if((current_char = get_next_char(cmd_stream)) != '&')
      {
        error(1, 0, "%d: Found lone &, invalid character", cmd_stream->line_number);
        break;
      }
      next_token_string[index++] = current_char;     
      next_token_string[index] = 0;     //Add the end zero byte
      cmd_stream->next_token = D_AND;
      break;
    }
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
      //Read a string of newlines until there is none, this is one newline token
      while((current_char = get_next_char(cmd_stream)) == '\n')
        cmd_stream->line_number++;
        
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
      error(1, 0, "%d: Unrecognized or out of place character", cmd_stream->line_number);
      break;
    }
  }
  return cmd_stream->current_token;
}

enum token_type check_next_token(command_stream_t s)
{
  return s->next_token;
}

command_t complete_command (command_stream_t s)
{
  command_t first_c = and_or(s);
  
  while(check_next_token(s) == NEWLINE || check_next_token(s) == SEMICOLON)
  {
    read_next_token(s);
    if(check_next_token(s) == END)
      break;
    
    command_t sequence_b = and_or(s);
    
    command_t seq_com = malloc(sizeof(struct command));
    
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

// Parses and_or clause
command_t and_or (command_stream_t s)
{
  command_t first_c = pipeline(s);
  while(check_next_token(s) == D_AND || check_next_token(s) == D_OR)
  {
    enum token_type curr_token = read_next_token(s);
    command_t and_or_b = pipeline(s);
    command_t next_com = malloc(sizeof(struct command));
      
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

//Parses pipeline
command_t pipeline (command_stream_t s)
{
  command_t first_c = command_parse(s);
  
  while(check_next_token(s) == PIPE)
  {
    read_next_token(s);
    command_t pipe_b = command_parse(s);
    command_t pipe_com = malloc(sizeof(struct command));
    
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

//Parses command
command_t command_parse (command_stream_t s)
{
  //Optional newlines as white space may appear before a simple command or the ( of a subshell
  if(check_next_token(s) == NEWLINE)
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
        error(1, 0, "%d: Could not read script, expected input argument", s->line_number);
      }
      
      subshell_c->input = malloc(strlen(s->current_token_string));
      strcpy(subshell_c->input, s->current_token_string);
    }
    if(check_next_token(s) == GREATER)
    {
      // Read in the >
      read_next_token(s);
      // Attempt to read in the output argument
      if(read_next_token(s) != WORD)
      {
        error(1, 0, "%d: Could not read script, expected output argument", s->line_number);
      }
      
      subshell_c->output = malloc(strlen(s->current_token_string));
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
        error(1, 0, "%d: Could not read script, expected input argument", s->line_number);
      }
      
      simple_c->input = malloc(strlen(s->current_token_string));
      strcpy(simple_c->input, s->current_token_string);
    }
    if(check_next_token(s) == GREATER)
    {
      // Read in the >
      read_next_token(s);
      // Attempt to read in the output argument
      if(read_next_token(s) != WORD)
      {
        error(1, 0, "%d: Could not read script, expected output argument", s->line_number);
      }
      
      simple_c->output = malloc(strlen(s->current_token_string));
      strcpy(simple_c->output, s->current_token_string);
    }
    return simple_c;
  }
}

//Parses subshell
command_t subshell (command_stream_t s)
{
  if(read_next_token(s) != LEFT_PARAN)
  {
    error(1, 0, "%d: Could not read script, expected left parathesis", s->line_number);
  }
  command_t inner_com = complete_command(s);
  
  //Optional newlines as white space may appear before ) of the subshell
  if(check_next_token(s) == NEWLINE)
  {
    read_next_token(s);
  }
  
  if(read_next_token(s) != RIGHT_PARAN)
  {
    error(1,0, "Line %d, Could not read script, expected right paranthesis", s->line_number);
  }
  
  command_t shell_command = malloc(sizeof(struct command));
    
  shell_command->type = SUBSHELL_COMMAND;
  shell_command->status = -1;
  shell_command->input = 0;
  shell_command->output = 0;
  shell_command->u.subshell_command = inner_com;
  
  return shell_command;
}

//Parses simple command
command_t simple_command (command_stream_t s)
{
  int index = 0;
  int array_max = 50;
  enum token_type next;
  if(read_next_token(s) != WORD)
  {
    error(1, 0, "%d: Could not read script, expected command, found: %s", s->line_number, s->current_token_string);
  }
  
  //Allocate inital space for array of string pointers
  char** words_array = malloc(sizeof(char*) * 50); //Inital size of 50 words
  
  //Allocate the string's space
  words_array[index] = malloc(strlen(s->current_token_string));
  strcpy(words_array[index], s->current_token_string);
  index++;
  
  while(check_next_token(s) == WORD)
  {
    read_next_token(s);
    words_array[index] = malloc(strlen(s->current_token_string));
    strcpy(words_array[index], s->current_token_string);
    index++;
    //If the number of words becomes to long, reallocate space  TODO: Use professor's reallocs
    if(index > array_max)
    {
      array_max += 50;
      words_array = realloc(words_array, array_max);
      if(words_array == NULL)
        error(1,0, "Line %d, Unable to allocate memory for number of words in command", s->line_number);
    }
  }
  words_array[index] = 0;     //End with a 0 byte
    
  command_t simple_c = malloc(sizeof(struct command));
    
  simple_c->type = SIMPLE_COMMAND;
  simple_c->status = -1;
  simple_c->input = 0;
  simple_c->output = 0;
  simple_c->u.word = words_array;
  
  return simple_c;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
         void *get_next_byte_argument)
{
  command_stream_t cmd_stream = malloc( sizeof(struct command_stream) );
  cmd_stream->getbyte = get_next_byte;
  cmd_stream->arg = get_next_byte_argument;
  // Invalid next char is -2
  cmd_stream->next_char = -2;
  cmd_stream->line_number = 1;
  
  // Allocate inital size for token string arrays
  cmd_stream->next_token_string = malloc ( 50 * sizeof(char) );
  cmd_stream->next_token_string[0] = 0;
  cmd_stream->current_token_string = malloc ( 50 * sizeof(char) );
  cmd_stream->current_token_string[0] = 0;
  cmd_stream->max_token_length = 50;  //50 characters initially available
  
  return cmd_stream;
}

command_t
read_command_stream (command_stream_t s)
{
  if(check_next_token(s) == END)
    return NULL;
  read_next_token(s);
  command_t result = and_or(s);
  return result;
}
