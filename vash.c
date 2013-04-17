/* Andre Byrne
 * 100045589 */

#include "vash.h"

/* a table of vash builtins for looking them up by name
 * This tabe has a one to one mapping with the VASH_BUILTIN
 * enum below */
static const char * builtin_lookup_table[NUM_BUILTINS] = {
  "not_a_builtin",
  "cd",
  "mk",
  "exit"
};

/* enums for switching based on builtin type */
typedef enum VASH_BUILTIN {
  NOT_A_BUILTIN,
  CD,
  MK,
  EXIT
} VASH_BUILTIN;

/* documented in vash.h */
static char * getInput(Vash * self_);
static TYPE decode(char * message);
static int start(Vash * self_);
static int callBuiltin(Vash * self_, const char * message, List * argv);
static char * setContext(Vash * self_, /*@only@*/ const char * symbol);
static /*@null@*/ Context * getContext(Vash * self_, const char * symbol);
static const List * getPath(const Vash * self_);
static int changeDirectory(Vash * self_, const List * list);
static int makeBranch(Vash * self_, const List * list);
static void displayPrompt(const Vash * self_);
static void displayContexts(const Vash * self_);

/* private class scope methods */

/* returns true if and only if the given string matches the
 * name of a VASH builtin.
 * */
static VASH_BUILTIN getBuiltin(const char * symbol);

/* writes frmo stdin until new line or EOF are encountered. If
 * EOF is encountered, then the return value becomes "exit"
 * indicating that the user wishes to terminate the session.
 * The string returned will not end in \n. If no input is given
 * or an error was encountered, return value will be NULL.
 * @alloc YES the caller becomes responsible for the return value
 * @crash YES failed to malloc
 * @null YES waitForInput returns null if no input was given or
 *           an error condition was encountered. Check with ferror.
 * */
static char * waitForInput();

/* Private instance scope methods */

/* calls the context constructor with the cwd */
static Context * setupDefaultContext(Vash * self);

/* analyzes input and tries to execute every command
 * that can be identified from the input
 * */
static int handleInput(Vash * vash, char * input);

/* given a line representing a single command to execute,
 * interpretLine decodes and executes that lins.
 * */
static int interpret_phrase(Vash * vash, char * phrase);

Vash * init_vash() {
  Vash * self = (Vash*) malloc(sizeof(Vash));

  if (NULL != self) {

    /* initialize the PATH */
    char * data = getenv("PATH");
    char * raw_path = string_with_size(strlen(data) + 1, "init_vash");
    strcpy(raw_path, data);

    /* setup function pointers first */
    self->start = start;
    self->decode = decode;
    self->callBuiltin = callBuiltin;
    self->setContext = setContext;
    self->getContext = getContext;
    self->getPath = getPath;
    self->changeDirectory = changeDirectory;
    self->makeBranch = makeBranch;
    self->displayPrompt = displayPrompt;
    self->displayContexts = displayContexts;
    self->getInput = getInput;

    /* parse the raw path into the list */
    self->PATH = init_list();
    appendTokens(self->PATH, raw_path, ":");

    self->terminate_session = false;

    self->number_of_contexts = 0;
    self->default_context = setupDefaultContext(self);
    self->current_context = self->default_context;

    free(raw_path);
  } else {
    free(self);
    self = NULL;
  }

  return self;
}

void release_vash(Vash * vash) {
  Vash * self = vash;
  int index;

  if (self != NULL) {

    release_list(self->PATH);

    for (index = 0; index < (self->number_of_contexts); index++) {
      release_context(vash->contexts[index]);
      free(vash->context_names[index]);
    }

    free(self);
  }

}

void the_worst_signal_handler_EVAR(int status, pid_t pid) {

  int exit_status;

  if (WIFEXITED(status)) {
    exit_status = WEXITSTATUS(status);
    switch (exit_status) {
      case 0 :
        fprintf(stderr, "[] %d Done\n", (int)pid);
        break;
      default :
        fprintf(stderr, "[] %d Exit %d\n", (int)pid, exit_status);
        break;
    }

  } else if (WIFSIGNALED(status)) {
    exit_status = WTERMSIG(status);
    switch (exit_status) {
      case SIGKILL :
        fprintf(stderr, "[] %d killed %d\n", (int)pid, exit_status);
        break;
      case SIGTTIN :
        fprintf(stderr, "[] %d stopped %d\n", (int)pid, exit_status);
        kill(pid, SIGSTOP);
        break;
      default :
        fprintf(stderr, "[] %d received signal %d\n", (int)pid, exit_status);
        break;
    }
  } else if (WIFSTOPPED(status)) {

    exit_status = WSTOPSIG(status);
    switch (exit_status) {
      case SIGTTIN :
      case SIGTTOU :
        /* this is not a cannonical way to do this */
        fprintf(stderr, "[] %d stopped %d\n", (int)pid, exit_status);
        kill(pid, SIGKILL);
        break;
      default :
        fprintf(stderr, "[] %d received signal %d\n", (int)pid, exit_status);
        break;
    }
  } else {

  }
}

int start(Vash * self_) {
  Vash * const self = (Vash *)self_;
  int exit_status;

  if (SIG_ERR == signal(SIGINT, SIG_IGN)) {
    perror("vash");
  }

  /* TODO handle SIGCHLD: I don't get it */

  while (false == self->terminate_session) {
    char * input;
    int status;
    pid_t pid;

    /* TODO this is not how we handle SIGCHLD */
    if (0 < (pid = waitpid(-1, &status, WNOHANG | WUNTRACED))) {
      /* uh oh something happened: better call the worst signal handler
       * we can imagine! */
      the_worst_signal_handler_EVAR(status, pid);
    }

    input = self->getInput(self);

    if (NULL != input && 0 < strlen(input)) {
      exit_status = handleInput(self, input);
    }

    free(input);
  }

  return exit_status;
}

int handleInput(Vash * vash, char * input) {

  int index, length;
  int exit_status = 1;
  List * tokens = init_list();

  appendTokensAndDelimiters(tokens, input, ";&");
  length = tokens->count(tokens);

  /* We are dealing with three things:
   * ; delimited instructions     -> phrases
   * & delimited instructions and -> asides
   * | delimited instructions     -> links (in a chain)
   *
   * First we parse on ; then on &,
   * then on | recursively so that
   * we will finally be executing left to right */
  for (index = 0; index < length; index++) {

    char * phrase = tokens->pop(tokens);
    exit_status = interpret_phrase(vash, phrase);

    free(phrase);
  }

  release_list(tokens);

  return exit_status;
}

int interpret_phrase(Vash * vash, char * phrase) {

  int exit_status = 1;
  char * first, * message;
  List * tokens = init_list();
  Context * context;

  /* tokenize on spaces */
  appendTokens(tokens, phrase, " ");
  first = tokens->pop(tokens);

  /* strtok may return null */
  if (NULL == first) return 1;

  /* set the context if the first token contains ":" */
  message = vash->setContext(vash, first);
  context = vash->current_context;

  switch (vash->decode(message)) {
    case BUILTIN :
      exit_status = vash->callBuiltin(vash, message, tokens);
      break;
    case COMMAND :
      exit_status = context->callCommand(context, message, tokens);
      break;
    default :
      exit_status = 1;
      break;
  }

  /* setContext just returns null here */
  free(vash->setContext(vash, "default:"));

  release_list(tokens);
  free(message);
  free(first);

  return exit_status;
}

void displayContexts(const Vash * self_) {
  const Vash * const self = self_;
  int index;

  printf("\n%s: ", "Active Contexts");
  for (index = 0; index < self->number_of_contexts; index++) {
    printf("%s; ", self->context_names[index]);
  }
  printf("\n");
}

void displayPrompt(const Vash * self_) {
  const Vash * const self = self_;

  printf("(Vash) %s %s ", self->current_context->cwd, "$$");
}

char * waitForInput() {

  char * input = string_with_size(MAX_INPUT_LENGTH, "waitForInput");

  input = fgets(input, MAX_INPUT_LENGTH, stdin);

  if (0 != feof(stdin)) {
    /* user indicated eof, which is equivalent to "exit" message */
    free(input);
    input = (char*) malloc(strlen("exit") + 1);
    input = strcpy(input, "exit");

  } else if (NULL != input) {
    (void)chomp(input, '\n'); /* remove trailing newline */

  }

  return input;
}

char * getInput(Vash * self_) {
  Vash * const self = self_;

  char * input;

  self->displayContexts(self);

  self->current_context = self->default_context;
  if (-1 == chdir(self->current_context->cwd)) {
    perror("vash: getInput");
  }

  /* set the current context to getcwd */
  self->current_context->setCWD(self->current_context, "");

  self->displayPrompt(self);
  input = waitForInput();

  if (0 != ferror(stdin)) {
    alertAndCrash("getInput", "0 != ferror(stdin)");
  }

  return input;
}

TYPE decode(char * message) {

  enum TYPE type;

  VASH_BUILTIN builtin = getBuiltin(message);

  /* is the message in the builtin table */
  if (NOT_A_BUILTIN != builtin) {
    type = BUILTIN;

  /* the message may be a command */
  } else {
    type = COMMAND;
  }

  return type;
}

int callBuiltin(Vash * self_, const char * message, List * list) {
  Vash * const self = self_;

  VASH_BUILTIN builtin = getBuiltin(message);
  int exit_status = 1;

  switch(builtin) {
    case EXIT :
      self->terminate_session = true;
      exit_status = 0;
      break;
    case CD :
      exit_status = self->changeDirectory(self, list);
      break;
    case MK :
      exit_status = self->makeBranch(self, list);
      break;
    default :
      exit_status = 1;
      break;
  }

  return exit_status;
}

char * setContext(Vash * self_, const char * symbol) {
  Vash * const self = self_;

  char * mutable_copy = string_with_size(strlen(symbol) + 1, "setContext");
  char * index_of_separator;
  char * index_after_separator;
  char * instruction_part, * branch_name;

  strcpy(mutable_copy, symbol);
  index_of_separator = strpbrk(mutable_copy, ":");

  /* if there is a separator, proceed with separation */
  if (NULL != index_of_separator) {
    Context * context;
    index_of_separator[0] = '\0'; /* break the string up logically */
    index_after_separator = &index_of_separator[1];

    if ('\0' != index_after_separator[0]) {
      instruction_part = string_with_size(strlen(index_after_separator) + 1, "setContext");
      strcpy(instruction_part, index_after_separator);
    } else {
      instruction_part = NULL;
    }

    branch_name = mutable_copy;

    context = self->getContext(self, branch_name);
    if (NULL != context) {
      self->current_context = context;
      if (-1 == chdir(self->current_context->cwd)) {
        perror("vash: setContext:");
      };
    }

  /* if there is no separator, then just copy the symbol */
  } else {
      instruction_part = string_with_size(strlen(symbol) + 1, "setContext");
      strcpy(instruction_part, symbol);
  }

  free(mutable_copy);

  return instruction_part;
}

static const List * getPath(const Vash * self_) {

  return self_->PATH;
}

static int changeDirectory(Vash * self_, const List * list) {
  Vash * const self = self_;

  /* much like cd, we will just try to cd into the first arg and ignore the rest */

  /* if no argument or ~ are given, go HOME */
  if (NULL == list->head || 0 == strcmp(list->head->string, "~")) {
    char * environment_home = getenv("HOME");
    char * home;

    if (NULL == environment_home) {
      home = string_with_size(strlen("/") + 1, "changeDirectory");
      strcpy(home, "/");
    } else {
      home = string_with_size(strlen(environment_home) + 1, "changeDirectory");
      strcpy(home, environment_home);
    }

    self->current_context->setCWD(self->current_context, home);
    free(home);

  /* if - is given go to the previous */
  } else if (0 == strcmp(list->head->string, "-")) {
    self->current_context->setCWD(self->current_context, self->current_context->old_cwd);

  /* try resolving the path to see if it is a real path */
  } else {

    char * absolute_path = resolve_path(self->current_context->cwd, list->head->string);

    if (NULL != absolute_path) {
      self->current_context->setCWD(self->current_context, absolute_path);
    } else {
      fprintf(stderr, "%s: %s: %s: %s\n",
              SHELL_NAME,
              builtin_lookup_table[CD],
              list->head->string,
              " No such file or directory");
    }

    free(absolute_path);
  }

  return 0;
}

static int makeBranch(Vash * self_, const List * list) {
  Vash * const self = self_;

  int exit_status = 0;

  if (2 > list->count(list)) {
      fprintf(stderr, "vash: mk: usage: cd branch_name dir \n");
      exit_status = 1;

  } else if (MAX_CONTEXTS > self->number_of_contexts) {
    char * name = list->head->string;
    char * dir_name = list->head->next->string;

    /* like cd, we are just going to try the first arg given and bail if NO */

    self->contexts[self->number_of_contexts] = init_context(self, dir_name);

    if (NULL != self->contexts[self->number_of_contexts]) {
      self->context_names[self->number_of_contexts] = string_with_size(strlen(name) + 1, "makeBranch");
      strcpy(self->context_names[self->number_of_contexts], name);
      self->number_of_contexts++;

    } else {
      fprintf(stderr, "vash: mk: failed to create context: directory bad access\n");
      exit_status = 1;
    }

  } else {
    fprintf(stderr, "vash: mk: failed to create context: too many contexts\n");
    exit_status = 1;
  }

  return exit_status;
}

/* returns true if and only if the given string matches the
 * name of a VASH builtin. */
VASH_BUILTIN getBuiltin(const char * symbol) {

  VASH_BUILTIN builtin = NOT_A_BUILTIN;
  int index = 0, differ;

  while (NOT_A_BUILTIN == builtin && index < NUM_BUILTINS) {
    differ = strcmp(symbol, builtin_lookup_table[index]);

    if (0 == differ) {
      builtin = (VASH_BUILTIN)index;
    }

    index++;
  }

  return builtin;
}

/* returns true if and only if the given string matches the
 * name of a VASH builtin. */
Context * getContext(Vash * self_, const char * symbol) {
  Vash * const self = self_;

  Context * context = NULL;
  int index = 0, differ;

  while (NULL == context && index < self->number_of_contexts) {
    differ = strcmp(symbol, self->context_names[index]);

    if (0 == differ) {
      context = self->contexts[index];
    }

    index++;
  }

  return context;
}

Context * setupDefaultContext(Vash * self_) {
  Vash * const self = self_;

  Context * context;
  char * cwd = string_with_size(PATH_MAX, "setupDefaultContext");
  List * args = init_list();

  if (NULL == getcwd(cwd, PATH_MAX)) {
    alertAndCrash("setupDefaultContext", "failed to get cwd");
  }

  /* make the default branch and return it */
  (void)args->append(args, "default");
  (void)args->append(args, cwd);
  self->makeBranch(self, args);
  context = self->contexts[0];

  free(cwd);
  release_list(args);

  return context;
}

