/* Andre Byrne
 * 100045589 */

#include "va_utils.h"

static size_t va_strspn(const char * str1, const char * str2);

char * string_with_size(size_t size, const char * calling_method) {

  char * string = (char*) malloc(size);

  if (string == NULL) {
    alertAndCrash(calling_method, "failed to malloc");
    exit(EXIT_FAILURE); /* this is only here to silence a splint error... */
  } else {
    string[0] = '\0';
  }

  return string;
}

char * chomp(/*@only@*/ char * string, const char separator) {

  char * position = strrchr(string, separator);

  if (position != NULL) {
    position[0] = '\0'; /* the last occurance of separator is now \0 */
  } 

  return string;
}

int magnitude_of(unsigned int integer) {
  
  int magnitude = 0;
  while (++magnitude && (integer /= 10) > 0);
  
  return magnitude;
}

void alertAndCrash(const char * calling_method, const char * failure) {

  fprintf(stderr, "Aborting: %s in %s\n", failure, calling_method);
  perror("  The following error occured");
  errno = 0;
  exit(EXIT_FAILURE);
}

char * resolve_path(const char * base_path, const char * dir_name) {

  char * absolute_path = NULL;
  char * buffer;
  int index;
  BOOL absolute;
  size_t size;

  /* valgrind hates strspn */
  index = (int)va_strspn(dir_name, " \f\n\r\t\v") - 1; /* index of first non whitespace */

  absolute = (BOOL)('/' == dir_name[index]);
  
  size = (absolute)? 0 : strlen(base_path) + strlen("/"); 
  size += strlen(dir_name) + 1;

  buffer = string_with_size(size, "resolve_path");

  if (false == absolute) {
    strcat(buffer, base_path); 
    strcat(buffer, "/"); 
  }

  strcat(buffer, dir_name); 
 
  if (0 == access(buffer, X_OK)) {
    absolute_path = string_with_size(strlen(buffer) + 1, "validateMessage");    
    strcpy(absolute_path, buffer);
  } 

  free(buffer);

  return absolute_path;
}

static size_t va_strspn(const char * str1, const char * str2) {

  size_t span_length = 0;
  int index;
  BOOL sentinel = true;
  char ch;

  while (sentinel) {
    ch = str1[span_length];

    sentinel = false;
    for (index = 0; index < (int)strlen(str2); index++) {
      if (ch == str2[index]) sentinel = true; 
    }

    span_length++; /* the number of characters in the span */
  }

  return span_length;
}

void * failSafeMalloc(size_t size, const char * calling_method) {
 
  void * pointer = malloc(size);
  
  if (NULL == pointer) {
    fprintf(stderr, "Aborting: %s in %s\n", "failed to malloc", calling_method);
    perror("  The following error occured");
    errno = 0;
    free(pointer);
    exit(EXIT_FAILURE);
  }
 
  return pointer;
}

char * va_strtok(char * string, const char * delimiter) {
  int length = (int)strlen(string);
  int index = 0;
  int start_of_token = -1, end_of_token = -1;
  int end_of_delimiter = -1;
  int found;

  size_t token_length = 0, delimiter_length = 0;

  char * result;
  size_t size;

  /* find the offsets for the token and delimiter */
  found = 0;
  while (index < length && found < 3) {
    switch(found) {
      case 0 :
        if (NULL == strchr(delimiter, string[index])) {
          start_of_token = index;
          found++;
        } else {
          index++;
        }
        break;
      case 1 :
        if (NULL != strchr(delimiter, string[index])) {
          end_of_token = index;
          found++;
        } else {
          index++;
        }
        break;
      case 2 :
        if (NULL == strchr(delimiter, string[index])) {
          end_of_delimiter = index;
          found++;
        } else {
          index++;
        }
        break;
      default :
        break;
    }
  }

  if (-1 == start_of_token) { /* we found no token */
    start_of_token = length;
  }

  if (-1 == end_of_token) { /* if found the end of string, that's fine */
    end_of_token = index;
  }

  if (-1 == end_of_delimiter) { /* delimiters until the end of the string */
    end_of_delimiter = length; 
  }

  token_length = (size_t)(end_of_token - start_of_token);
  delimiter_length = (size_t)(end_of_delimiter - end_of_token);

  size = token_length + delimiter_length + strlen(" ") + 1;

  result = (char*)malloc(size);

  result[0] = '\0';

  strncat(result, &string[start_of_token], token_length); 
  strcat(result, " ");
  strncat(result, &string[end_of_token], delimiter_length); 

  if (0 == strcmp(result, " ")) {
    free(result);
    result = NULL; 
  }

  for(index = 0; index < end_of_token; index++) {
    string[index] = delimiter[0];
  } 


  return result;
}
