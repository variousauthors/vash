/* Andre Byrne
 * 100045589 */ 

#ifndef VA_UTILS_H
#define VA_UTILS_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>

#define SHELL_NAME "vash"

typedef enum BOOL {false, true} BOOL;

/* allocates and returns a pointer to memory for a string of the given
 * size. The name of the calling method is used in case the allocation
 * fails, to notify the user where the failure occured. 
 * @post the memory was succesfully allocated
 * @param size the number of bytes to allocate 
 * @param calling_method the method from which this function was called 
 * @alloc YES caller becomes responsible for deleting the return value 
 * @abort YES abort with EXIT_FAILURE if malloc fails
 * @return a pointer to the allocated memory 
 * */
char * string_with_size(size_t size, const char * calling_method);

/* chomp modifies the given string in place. returns the given string 
 * with the record separator removed from the last position. If the 
 * given separator is not found in the string then the string is 
 * unmodified and NULL is returned instead. 
 * @param string the string to be chomped
 * @param separator the record separator to be removed 
 * @bang chomp modifies the contents of string 
 * @return the succesfully mutated string or NULL 
 * */
/*@null@*/ char * chomp(char * string, const char separator);

/* determines the number of digits in a given unsigned integer, 
 * which can be useful when you are trying to allocate strings 
 * dynamically with numbers that might be very long. */
int magnitude_of(unsigned int integer);

void alertAndCrash(const char * calling_method, const char * failure);

/*@null@*/ char * resolve_path(const char * base_path, const char * file_name);

/*@out@*/ void * failSafeMalloc(size_t size, const char * calling_method);

char * va_strtok(char * string, const char * delimiter);

#endif
