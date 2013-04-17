/* Andre Byrne
 * 100045589 */

#include "context.h"
#include "command.h"

/* instance methods documented in context.h */
static int callCommand(Context * self_, const char * message, List * argv);
static void setCWD(Context * self_, const char * dir_path);

Context * init_context(const Vash * parent, const char * cwd) {

  Context * context;

  /* check that the given cwd is good relative the current path */
  char * path = resolve_path("", cwd);

  if (NULL == path) {
    return NULL;
  }

  free(path);

  context = (Context *) failSafeMalloc(sizeof(Context), "init_context");  

  context->cwd = string_with_size(strlen(cwd) + 1, "init_context");
  strcpy(context->cwd, cwd);
  context->old_cwd = string_with_size(strlen(cwd) + 1, "init_context");
  strcpy(context->old_cwd, cwd);

  context->PATH = copy_list(parent->getPath(parent));
  if (NULL == context->PATH) {
    context->PATH = init_list();
  }

  context->callCommand = callCommand;
  context->setCWD = setCWD;

  return context;
}

void release_context(const Context * context) {

  if (NULL != context) {

    if (NULL != context->PATH) {
      release_list(context->PATH);
    }

    free((char *)context->cwd);
    free((char *)context->old_cwd);
  } 
  
  free((char *)context);
}

int callCommand(Context * self_, const char * message, List * argv) {
  Context * const self = self_;
  /* try to instantiate a command */
  Command * command = init_command(self, message, self->PATH);
  int exit_status = 1;

  /* command may be NULL if message is not an executable file */
  if (NULL != command) {
    command->setArgv(command, argv);
    exit_status = command->execute(command);
  }

  release_command(command);

  return exit_status; 
}

void setCWD(Context * self_, const char * dir_path) {
  Context * const self = self_;

  BOOL changed = true; /* for setting old_cwd */

  char * buffer = string_with_size(PATH_MAX, "setCWD");

  /* store the initial cwd and free self->cwd */
  char * initial = string_with_size(strlen(self->cwd) + 1, "setCWD");
  strcpy(initial, self->cwd);
  free(self->cwd);

  /* empty string means, basically, clean up the cwd */
  if (0 == strcmp(dir_path, "")) {
    (void)getcwd(buffer, PATH_MAX); 
    changed = false;
  } else {
    strcpy(buffer, dir_path);
  }

  self->cwd = string_with_size(strlen(buffer) + 1, "setCWD");
  strcpy(self->cwd, buffer);
  
  /* if the CWD did change, update old_cwd */
  /*@ -boolcompare @*/
  if (true == changed) {
  /*@ =boolcompare @*/
    free(self->old_cwd);
    self->old_cwd = string_with_size(strlen(initial) + 1, "setCWD");
    strcpy(self->old_cwd, initial);
  }

  free(buffer);
  free(initial);
}
