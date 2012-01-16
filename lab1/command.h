// UCLA CS 111 Lab 1 command interface

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

//Retrieve the next valid character
int get_next_char(command_stream_t cmd_stream);

//Put back an unused input
void unget_char(int unwanted, command_stream_t cmd_stream);

//Check if the character is a valid part of a WORD
int is_valid_word_char(int character);

// Tokenization, the lexer
enum token_type read_next_token(command_stream_t cmd_stream);

// Parsing complete command
command_t complete_command (command_stream_t s);

// Parses and_or clause
command_t and_or (command_stream_t s);

//Parses pipeline
command_t pipeline (command_stream_t s);

//Parses command
command_t command_parse (command_stream_t s);

//Parses subshell
command_t subshell (command_stream_t s);

//Parses simple command
command_t simple_command (command_stream_t s);

/* Create a command stream from LABEL, GETBYTE, and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the integer flag is
   nonzero.  */
void execute_command (command_t, int);

/* Return the exit status of a command, which must have previously been executed.
   Wait for the command, if it is not already finished.  */
int command_status (command_t);
