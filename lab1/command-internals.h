// UCLA CS 111 Lab 1 command internals

enum command_type
  {
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
  };
  
enum token_type
  {
    WORD,           // ASCII, digit, or: ! % + , - . / : @ ^ _
    D_AND,          // &&
    D_OR,           // ||
    PIPE,           // |
    LEFT_PARAN,     // (
    RIGHT_PARAN,    // )
    LESS,           // <
    GREATER,        // >
    NEWLINE,        // \n
    SEMICOLON,      // ;
    END
  };

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or 0 if none.
  char *input;
  char *output;

  union
  {
    // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
    struct command *command[2];

    // for SIMPLE_COMMAND:
    char **word;

    // for SUBSHELL_COMMAND:
    struct command *subshell_command;
  } u;
};

struct command_stream
{
  // How and where to get the next byte
  int (*getbyte) (void *); 
  void *arg;
  
  // A stored char byte, if it was read but not used
  int next_char;
  
  int line_number;
  
  // For storing the token strings
  char *next_token_string;
  enum token_type next_token;
  char *current_token_string;
  enum token_type current_token;
  // Every realloc, this must be updated
  int max_token_length;
};