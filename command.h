/* Andre Byrne
 * 100045589 */ 

#ifndef COMMAND_H
#define COMMAND_H

#include "context.h"
#include "list.h"
#include "va_utils.h"


/* Class Command 
 * brief: Command encapsulates the validation and execution of an executable
 * file. A Command has a context in which it is being executed, a path to 
 * a valid executable file, and a possibly NULL arguments list. The list 
 * can be set with setArgv and retrieved with getArgv. Finally, a command
 * can be run with execute. 
 * */

/* forward declaration: Command needs to know about context */
struct Context;

typedef struct Command {

  char * executablePath; /* the path to a valid executable file */
  const char * cwd; 

  /* weak reference: each Command is created by a context 
   * and needs some information from that context. The 
   * Command object is guaranteed to be released before 
   * the Context that created it, since all Commands are 
   * initialized and released in Command->callCommand 
   * */
  const struct Context * context;

  /*@null@*/ List * argv; /* a list of arguments to the command, which may be NULL */

  /* redirection handles: these are POSIX filenames */
  char * in_file;
  char * out_file;

  /* execution flags: how should this command be executed */
  BOOL background;
  BOOL pipe_output;
  int pipe_length;
  
  /* Sets argv to the given list.
   * @see getArgv
   * @post argv is an independant copy of the given list 
   * @param self_ the calling object 
   * @param argv (retained) the list to be copied
   * @alloc NO memory allocated by setArgv is the responsibility of self_ 
   * */
  void (*setArgv)(struct Command * self_, List * argv); 

  /* Returns an array containing the command name followed by all of the 
   * arguments. The array returned is NULL terminated and suitable to 
   * be passed to functions in the exec family.   
   * @see setArgv
   * @param self_ the calling object 
   * @alloc YES the caller becomes responsible for freeing the return value 
   * @null NO argv will contain at minimum the name of the command and NULL
   * @crash YES failed to malloc
   * @return a linear array of strings 
   * */
  char ** const (*getArgv)(struct Command * self_);

  /* returns the number of elements in argv. Remember that 
   * argv is a NULL terminated array of strings, so count 
   * will be one less than the number of elements in the 
   * array. */
  int (*getArgc)(struct Command * self_);

  /* Executes the command represented by the callilng object. A Command 
   * object is guaranteed to execute. After that the child process may
   * fail as a result of bad arguments, etc...
   * @pre setArgv has been called 
   * @post the given command has been executed 
   * @param self_ the calling object 
   * @crash YES failed to fork 
   * @return returns the exit status of the forked process */
  int (*execute)(struct Command * self_);

} Command;

/* Allocates an initializes a new Command object encapsulating a given message
 * which may exist in the given PATH or the given context. The message is 
 * checked agains the path and context cwd, and if it does not describe an 
 * executable file then the return value will be NULL. This means that one 
 * of the class invarients of Command is that it represents an executable. 
 * @see release_command 
 * @pre context and list are initialized 
 * @alloc YES the caller is responsible for freeing the return value
 * @dtor YES Command is a Class and instances must be freed by release_command 
 * @crash YES failed to malloc
 * @null YES if the given command does not exist 
 * */
/*@null@*/ Command * init_command(const struct Context * context, const char * message, const List * PATH);

/* deallocates and frees all the fields in the given command instance. 
 * @param command the Command instance to be freed
 * @dtor THIS is the destructor for Class Command */
void release_command(/*@null@*/ /*@only@*/ Command * command);

#endif
