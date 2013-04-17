/* Andre Byrne
 * 100045589 */ 

#ifndef VASH_H
#define VASH_H

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "va_utils.h"
#include "context.h"
#include "list.h"

#define MAX_CONTEXTS 16
#define MAX_INPUT_LENGTH 256
#define NUM_BUILTINS 4
#define MAX_ARGC 256
#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

/* a message may represent a Vash builtin or a system command (or it may be invalid) */
typedef enum TYPE {BUILTIN, COMMAND, INVALID} TYPE;

/* Class Vash
 * brief: Vash is the Double Dollar Shell. Vash is a command line interpreter
 * with double the number of dollar signs commonly found in a shell.
 * */
typedef struct Vash {

  List * PATH; /* the environment PATH */

  BOOL terminate_session; /* if set, Vash will terminate gracefully */

  /* The context system is unique to Vash 
   * by default, all commands are executed in the default context */
  struct Context * default_context;
  struct Context * current_context;

  /* In addition to the default context, a Vash
   * user can execute commands in up to 16 other
   * contexts.
   * */
  struct Context * contexts[MAX_CONTEXTS];
  char * context_names[MAX_CONTEXTS];
  int number_of_contexts;

  /* Begin the VASH instance, which will run until VASH received 
   * exit, quit, logout, or [Ctrl-d] 
   * @param self_ the calling object
   * @alloc NO any memory allocated by Vash will be freed by Vash
   * @crash YES failed to malloc; failed to get cwd; failed to fork
   * @return tends to return 0 :)
   * */
  int (*start)(struct Vash * self_);

  /* wait while the user enters input to the keyboard 
   * @post the context list is displayed to standard out
   *       the current context is reset to default
   *       displays the patented Vash double dollar prompt  
   * @param self_ the calling object 
   * @alloc YES caller becomes responsible for return value 
   * @crash YES failed to malloc; 0 != ferror(stdin)
   * @return the \n terminated string input by the user 
   * */
  char * (*getInput)(struct Vash * self_);

  /* analyzes the given message and: modifies the message in place to 
   * remove the context part; and finally determines the TYPE of the 
   * message. 
   * @see TYPE defined above
   * @return a TYPE corresponding to the given message
   * */
  TYPE (*decode)(char * string);

  /* sets the current context to the context with the 
   * given context name if it exists. A string with a context will
   * have the context part separated from the command part
   * by a colon.
   * 
   *   eg source:ls
   *
   * the context is source, the command is ls. This method does
   * not affect the given string, and returns the part after the :
   * or null if it did not exist. 
   * @pre symbol has been allocated at least 
   * @post current context will have been set based on the symbol
   * @param self_ the calling object
   * @param symbol the string from which the context will be read 
   * @alloc YES the caller becomes responsible for the return value  
   * @null YES if the symbol is of the form "branch:" with nothing
   *           after the ":", then the return is null 
   * @return the instruction part (after the ":") is returned 
   * */
  char * (*setContext)(struct Vash * self_, /*@only@*/ const char * symbol);  

  /* attempts to call the given message as a vash builtin.
   * The given string is looked up in the Vash builtin table
   * by name, and if it exists, then the given argument list
   * is passed to the builtin. 
   * @pre list is initialized 
   * @post a vash builtin has been executed 
   * @param self_ the calling object
   * @param message the name of the builtin to be queried
   * @param list arguments to the builtin
   * @alloc NO
   * @return the exit status of the builtin or 1 if no builtin 
   *         exists
   * */
  int (*callBuiltin)(struct Vash * self_, const char * message, List * list);

  /* returns the PATH list 
   * @param self_ the calling object 
   * @return a const reference to the PATH 
   * */
  const List * (*getPath)(const struct Vash * self_);

  /* Displays the Vash Double Dollar prompt ellegantly 
   * @post you are amazed 
   * @param the calling object 
   * */
  void (*displayPrompt)(const struct Vash * self_);

  /* Displays the context list 
   * @post the context list is printed to standard out 
   * @param self_ the calling object */
  void (*displayContexts)(const struct Vash * self_);

  /* Changes the cwd of the current context based on the first 
   * parameter in the given argument list. 
   * @pre list is initialized 
   * @post the cwd member of the current context may have been updated 
   * @param self_ the calling object 
   * @param list the parameters passed to cd
   * @return always returns 0 :) 
   * */
  int (*changeDirectory)(struct Vash * self_, const struct List * list);

  /* creates a new context in the given Vash shell with name and
   * path determined by the arguments in the given list. Contexts 
   * are a powerful feature of the Vash Double Dollar shell. 
   * Users unfamiliar with Vash may want to try creating a context. 
   * they are created as follows: 
   * 
   *   $$ mk bin /usr/bin 
   * 
   * In this case, the new context name is "bin" and it points to the
   * given directory. From then on, the user may call commands in the 
   * given context in much the same way that a method is called on an 
   * object: 
   *
   *  $$ bin:cd ..
   *  $$ bin:pwd
   *
   * Those two commands would move the bin context to bin/.. and then 
   * execute pwd in that directory. 
   * @pre list is initialized
   * @post a context with the given name was created if possible
   * @param self_ the calling object
   * @param list arguments passed to mk
   * @return the success or failuer of mk
   * */
  int (*makeBranch)(struct Vash * self_, const struct List * list);

  struct Context * (*getContext)(struct Vash * self_, const char * symbol);

} Vash;

/* initializes and returns a pointer to a new instance of vash 
 * @alloc YES the caller becomes responsible for the return value
 * @dtor YES Vash is a Class and must be freed with release_vash
 * @null YES init_vash returns null if initialization was not complete
 * @return a pointer to an instance of vash, or null
 * */
/*@null@*/ Vash * init_vash();

/* given a pointer to a non-null vash object, release_vash frees all
 * memory associated with the vash instance pointed to. Finally, the
 * pointer is set to NULL 
 * @param vash the Vash object to be freed
 * @dtor THIS is the destructor for Class Vash 
 * */
void release_vash(/*@null@*/ /*@only@*/ Vash * vash);

#endif

