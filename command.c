/* Andre Byrne
 * 100045589 */

#include "command.h"

/* documented in command.h */
static void setArgv(Command * self_, List * argv);
static /*@null@*/ char ** const getArgv(Command * self_);
static int getArgc(Command * self_);
static int execute(Command * self_); 

/* Private class scope method */

/* ensures that, within the given context and PATH, the given message represents
 * an executable file. Finally, the absolute path to the executable file is 
 * returned if such a file exists.
 * @pre Context is initialized, List is initialized 
 * @post message is known to be an executable or not 
 * @param context provides the cwd to check in
 * @param message the string of interest 
 * @param PATH a list of directories in which message may reside
 * @alloc YES the caller is responsible for the return value 
 * @null YES if no executable file could be resolved
 * @return the absolute path of the given executable  
 * */
static /*@null@*/ char * validateMessage(const Context * context, const char * message);

/* interprets the status returned by execv and displays relevant message */
int status_report(Command * command, int *status, pid_t pid);

Command * init_command(const Context * context, const char * message, const List * PATH) {

  Command * self = (Command *) failSafeMalloc(sizeof(Command), "init_command");

  char * executablePath = validateMessage(context, message);

  if (NULL != executablePath) {

    char * cwd = string_with_size(strlen(context->cwd) + 1, "init_command");
    strcpy(cwd, context->cwd);
    self->cwd = cwd;

    self->context = context; /* weak reference @see Command */

    self->executablePath = string_with_size(strlen(executablePath) + 1, "init_command");
    strcpy(self->executablePath, executablePath);

    free(executablePath); 

    self->argv = NULL;
    self->in_file = NULL;
    self->out_file = NULL;

    self->background = false;
    self->pipe_output = false;
    self->pipe_length = 1;

    self->setArgv = setArgv;
    self->getArgv = getArgv;
    self->getArgc = getArgc;
    self->execute = execute;

  } else {
    free(self);
    self = NULL;
    fprintf(stderr, "%s: %s\n", message, "command not found");
  }

  return self;
}

void release_command(Command * command) {
  
  if (NULL != command) {
    release_list(command->argv);
    free((char *)command->cwd);
    free((char *)command->in_file);
    free((char *)command->out_file);
  
    if (NULL != command->executablePath) {
      free(command->executablePath);
    }

    free(command);
  }
}

char * validateMessage(const Context * context, const char * message) {
  
  const List * PATH = context->PATH;
  char * executablePath = NULL;

  /* check the current directory */
  if (NULL != (executablePath = resolve_path(".", message))) {

  /* check the context cwd */
  } else if (NULL != (executablePath = resolve_path(context->cwd, message))) {

  } else {
    
    /* iterate over PATH */
    Node * node = PATH->head;
    while (NULL != node && NULL == executablePath) {
      executablePath = resolve_path(node->string, message);
      node = node->next;
    }
  }

  return executablePath;
}

void setArgv(Command * self_, List * argv) {
  Command * const self = self_;

  char * token, * executable_path; /* for pipes */
  
  self->argv = init_list();
  
  /* pop each tokens from list and examine it */
  while(!argv->isEmpty(argv)) {
    token = argv->pop(argv);

    if (1 == strlen(token)) { /* we may have a special characters */
      switch (token[0]) {
        case '<' :
          if (!argv->isEmpty(argv)) {
            free(token);
            token = argv->pop(argv);
            self->in_file = string_with_size(strlen(token) + 1, "setArgv");
            strcpy(self->in_file, token);
          }
          break;
        case '>' :
          if (!argv->isEmpty(argv)) {
            free(token);
            token = argv->pop(argv);
            self->out_file = string_with_size(strlen(token) + 1, "setArgv");
            strcpy(self->out_file, token);
          }
          break;
        case '&' :
          self->background = true;
          break;
        case '|' :
          /* pipes are all executed together in a loop */  
          self->pipe_output = true;
          (void)self->argv->append(self->argv, token); /* execute will need the | token */
          if (!argv->isEmpty(argv)) {
            free(token);
            token = argv->pop(argv); /* this should be a valid path */
            executable_path = validateMessage(self->context, token);
            /* if validateMessage returns NULL then that's OK 
             * execute will do the first part of the pipe and stop 
             * when it encounters a bad command */
            if (NULL == executable_path) {
              (void)self->argv->append(self->argv, token);
            } else {
              self->pipe_length++;
              (void)self->argv->append(self->argv, executable_path);
            }
            free(executable_path);
          }
          break;
        case ';' : /* don't care about these */
          break;
        default : /* just some crap I don't know */
          (void)self->argv->append(self->argv, token);
          break;
      }
    /* its an argument so just append it */
    } else {
      (void)self->argv->append(self->argv, token);
    }

    free(token);
  }
}

char ** const getArgv(Command * self_) {
  Command * const self = self_;
  
  char ** argv = NULL;

  if (NULL != self->argv) {
    int count = self->argv->count(self->argv);
    int index = 1;
    Node * node = self->argv->head;

    /* alloc enough space for count + 1 char pointers + the NULL */ 
    argv = (char**) failSafeMalloc((sizeof(char*)) * (count + 2), "getArgv");

    /* for each char * starting at 1 allocate enough space for the right string */
    while (NULL != node) {
      if (0 == strcmp(node->string, "|")) {
        /* replace the pipe with a NULL for exec to stop on */
        argv[index] = NULL;
      } else {
        argv[index] = string_with_size(strlen(node->string) + 1, "getArgv"); 
        strcpy(argv[index], node->string);
      }

      node = node->next;
      index++;
    }

    argv[0] = string_with_size(strlen(self->executablePath) + 1, "getArgv");
    strcpy(argv[0], self->executablePath);
    argv[count + 1] = NULL;
  }

  return argv;
}

static int getArgc(Command * self_) {
  const Command * const self = self_;

  return self->argv->count(self->argv) + 1; /* +1 argv also contains the command name */
}

int redirect_to_file(char * file_name, int file_number, int options) {

  int file_handle;
  int exit_status = 0;

  /* actually a null file handle is no problem: it just means we didn't 
   * actually want to redirect */
  if (NULL == file_name) {
    return 0;
  }

  file_handle = open(file_name, O_RDWR | options, S_IWUSR | S_IRUSR);

  if (-1 != file_handle) {
    dup2(file_handle, file_number);
    close(file_handle);
  } else {
    fprintf(stderr, "%s: %s: " , SHELL_NAME, file_name);
    perror("");
    exit_status = 1;
  }

  return exit_status;
}

int execute(Command * self_) {
  Command * self = self_;

  /* by this point we know executablePath is an executable file */
  int * status;
  int in_file = -1, out_file = -1;
  int leftPipe[2], rightPipe[2], temp[2];
  int std_in = dup(STDIN_FILENO), std_out = dup(STDOUT_FILENO);
  int exit_status = 0; /* if this is set to 1 before the exec, we bail */
  char ** argv_start = NULL;
  char ** argv = NULL;
  int index = 0;
  int iteration;
  int argc = self->getArgc(self);
  pid_t pid;
  int options = (self->background)? WNOHANG : 0;

  status = (int *) failSafeMalloc(sizeof(int), "execute");

  /* memory leak: actually this is freed most of the time... but sometimes
   * mysteriously it is not. I think it is because of the lame ass way
   * in which I fiddle with the array pointer later on */
  argv = self->getArgv(self);
  argv_start = &argv[0];

  /* by default we read from stdin and write to stdout */
  leftPipe[0] = STDIN_FILENO;

  /* we'll execute at least one, but as many as pipe_length programs */
  /* if for some reason we fail, we'll break from this loop */
  for(iteration = 0; iteration < self->pipe_length; iteration++) {

    /* here we sneaky open the pipe when no one is watching */
    if (iteration < self->pipe_length - 1) {
      pipe(rightPipe);
    } else {
      rightPipe[1] = STDOUT_FILENO;
    }
     
    switch ((pid = fork())) {
      case -1 :
        perror("fork");
        exit(1);
      case 0 :
        if (self->background) {
          setpgid(pid, 0);
        }
        dup2(leftPipe[0], STDIN_FILENO);
        dup2(rightPipe[1], STDOUT_FILENO);

        exit_status = redirect_to_file(self->in_file, STDIN_FILENO, 0);
        exit_status = redirect_to_file(self->out_file, STDOUT_FILENO, O_CREAT | O_TRUNC);

        if (1 != exit_status) {
          if (SIG_ERR == signal(SIGINT, SIG_DFL)) {
            perror("vash");
            exit(EXIT_FAILURE);
          }

          if (-1 == execv(self->executablePath, argv)) {
            perror("vash");
            exit(EXIT_FAILURE);
          }
        }
          
        exit(EXIT_FAILURE);
      default : 
        close(rightPipe[1]);
        if (-1 != in_file) {
          close(in_file);
        }
        
        if (-1 != out_file) {
          close(out_file);
        }

        /* if we're in the background we won't waitpid */
        if (!self->background) {
          if (-1 == waitpid(pid, status, options)) {
            fprintf(stderr, "%s: %s", SHELL_NAME, self->executablePath);
            perror(""); 
            errno = 0;
          }
        } else {
          free(status);
          status = NULL;
        }
    }

    /* set up the next iteration if there is one */
    /* this is probably the source of that memory leak above */
    if (1 < self->pipe_length && iteration < self->pipe_length -1) {
  
      /* setup the next command */
      /* walk until the next null */
      while (NULL != argv_start[index]) {
        index++;
      }
    
      /* If index >= argc then index + 1 is out of bounds 
       * even otherwise, there is still a chance index + 1 is out of bounds */
      if (index >= argc || NULL == argv_start[index + 1]) {
        break; /* bail out */

      } else {
        index++; /* we know there is something after the NULL */
        argv = &argv_start[index];
        free(self->executablePath);
        self->executablePath = argv[0]; /* we already validated this path */
      }
    }

    /* if there is another iteration, swap the pipes */
    if (iteration < self->pipe_length - 1) {
      memcpy(temp,   leftPipe,  sizeof temp);
      memcpy(leftPipe,  rightPipe, sizeof leftPipe);
      memcpy(rightPipe, temp,   sizeof rightPipe);
    }
  }

  /* restore stdin and stdout because I bin messin */
  dup2(std_in, STDIN_FILENO);
  dup2(std_out, STDOUT_FILENO);

  exit_status = status_report(self, status, pid);

  free(status);
  free(*argv_start); /* see here I free what argv_start points to... */
  free(argv_start);

  return exit_status; 
}

int status_report(Command * command, int *status, pid_t pid) {

  int exit_status = 1;

  if (NULL == status) {
    fprintf(stderr, "[] %d\n", (int)pid);
    return 0; 
  }

  /* still not entirely clear on this stuff... */
  if (WIFEXITED(*status)) {
    exit_status = WEXITSTATUS(*status);

  } else if (WIFSIGNALED(*status)) {
    exit_status = WTERMSIG(*status);

  }

  return exit_status;
}

