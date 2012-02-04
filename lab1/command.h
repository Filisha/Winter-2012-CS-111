// UCLA CS 111 Lab 1 command interface

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;
typedef struct word_node* word_node_t;
typedef struct depend_node* depend_node_t;
typedef struct tlc_node* tlc_node_t;


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

/* Based on command type, executes the appropriate function. */
void execute_generic(command_t);

/* If the first part is false, returns that immediately, otherwise
   returns the status of the 2nd command */
void execute_and(command_t);

/* If the first part is true, returns that immediately, otherwise
   returns the status of the 2nd command */
void execute_or(command_t);

/* Executes the first command in sequence, waits for that to complete,
   then executes the 2nd command.  Exit status is that of the 2nd command */
void execute_sequence(command_t);

/* Executes simple and subshell commands, with the possibilty of input
   and output (< and >) accounted for here */
void execute_io_command(command_t);

/* Executes both and allows the first commands output to be piped to
   the second command's input.  Returns when the 2nd command finishes and
   gives the 2nd command's exit status */
void execute_pipe(command_t);

/* Fills in the tlc node with all input output dependencies that occur
   inside of the command */
void generate_dependencies(tlc_node_t, command_t);

/* Execute commands with time travel.  */
command_t execute_time_travel (command_stream_t);

/* Execute a command.  No time travel involved */
void execute_command (command_t);

/* Return the exit status of a command, which must have previously been executed.
   Wait for the command, if it is not already finished.  */
int command_status (command_t);
