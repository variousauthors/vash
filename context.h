/* Andre Byrne
 * 100045589 */ 

#ifndef CONTEXT_H
#define CONTEXT_H

#include "list.h"
#include "vash.h"
#include "command.h"

/* Class Context 
 * brief: Vash has a the concept of "execution contexts". These are implemented
 * as Context objects. Each context object has a current working directory (cwd)
 * and the ability to execute arbitrary commands. Commands executed by a 
 * context always check the current context first for the existence of an 
 * executable. Context has a one-to-many relationship with Vash: a single
 * Vash instance may contain references to many contexts, and each context 
 * contains a reference back to the Vash that instantiated it. 
 * */

/* forward declaration */
struct Vash;

typedef struct Context {

  char * cwd; /* The current working directory for this context */
  char * old_cwd; /* The previous current working directory */

  const struct List * PATH; /* The Vash that created this context */

  /* Calls the command matching the given string with the arguments 
   * in the given list, if such a command exists. CallCommand collects
   * the return value from the execution, if it exists, and propagates 
   * it to the controller. 
   * @param self_ the calling object
   * @param message (retained) the name of or path to an executable file
   * @param argv (retained) a list of arguments to pass to the executable, the
                            first of which is expected to be the name of 
                            or path to the executable 
   * @alloc NO all memory allocated by callCommand is freed by callCommand
   * @return the exit status of the executable or 1 if no executable existed 
   * */
  int (*callCommand)(struct Context * self_, const char * message, List * argv);

  /* Sets the cwd of this context to the given directory path. If dir_path is
   * an empty string, dir_path will be set to the current working directory as
   * returned by getcwd(char *, size_t). This is used to "clean up" the cwd.
   * @post cwd points to the value of dir_path 
   * @post old_cwd points to the value of cwd before this call 
   * @param self_ the calling object 
   * @param dir_path (retained) the path to become cwd
   * @alloc NO any memory not freed in the method will be freed by release_context 
   * @crash YES failed to malloc 
   * */
  void (*setCWD)(struct Context * self_, const char * dir_path);

} Context;

/* Allocates and returns a Context object representing the given director path.
 * If the given path is not a valid directory, the return value will be NULL. 
 * @see release_context
 * @param cwd (retained) the valid current working directory of this context 
 * @alloc YES the caller becomes responsible for the return value
 * @dtor YES Context is a Class and instances must be freed with release_context 
 * @null YES if cwd is NULL the return value will be NULL
 * @crash YES failed to malloc 
 * @return a new instance of Context 
 * */
/*@null@*/ Context * init_context(const struct Vash * parent, const char * cwd);

/* Deallocates all memory and frees the given context object. 
 * @param context the context object to be freed 
 * @dtor THIS is the destructor for Class Context 
 * */
void release_context(/*@null@*/ /*@only@*/ const Context * context);

#endif
