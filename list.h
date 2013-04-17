/* Andre Byrne
 * 100045589 */ 

#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "va_utils.h"

/* Class List
 * brief: list is an arbitrary list ADT which implements a little functionality
 * from this container, and a little from that. Could be used as a Queue, or
 * stack, or you know... whatever I need at the time. At the time of this writing
 * List is specialized to contain strings. 
 *
 * 
 * 
 * */

/* struct Node 
 * Node is the building block of a list. Each node contains a value and points
 * to the next node in the list. 
 * */
typedef struct Node {

  char * string;
  /*@null@*/ struct Node * next;

} Node;

/* struct List 
 * List maintains a list of non-unique, (ideally) immutable nodes and 
 * exposes methods for adding and removing them, as well as information
 * regarding list accounting such as the emptyness or number of nodes. 
 * 
 * At the time of this writing List is specialized to contains strings
 * */
typedef struct List {

  Node * head; /* the first node in the list */
  Node * tail; /* the last node in the list */
  int length; /* the number of nodes in the list */

  /* Appends a node to the end of the list containing the given string 
   * and returns the node appended. If the given string was NULL then
   * no operation will be performed, and append will return NULL. The given
   * string will be independant from its representation in the list.
   * @pre string is not NULL
   * @post string is in the list 
   * @param self_ the calling List object 
   * @param string (retained) the string to be appended
   * @alloc NO any memory allocated by append belongs to self_
   * @null YES if string is NULL append will return NULL
   * @return a pointer to const refering to the node just appended
   * */
  const Node * (*append)(struct List * self_, const char * string);

  /* Adds a node to the beginning of the list containing the given string 
   * and returns the node added in this way. See append for details. 
   * @see append
   * @pre string is not NULL
   * @post string is in the list
   * @param self_ the calling List object
   * @param string (retained) the string to be added
   * @alloc NO any memory allocated by add belongs to self_
   * @null YES if the string is NULL add will return NULL 
   * @return a pointer to const refering to the node just add
   * */
  const Node * (*add)(struct List * self_, const char * string);

  /* Pop removes the node at the head of the list and returns its value. 
   * The value thus returned becomes the responsibility of the caller, 
   * but the node it belonged to will be freed by list.  
   * @pre the list is not empty 
   * @post the second node in the list is now the head of the list 
   *       the list contains one fewer nodes and may now be empty 
   * @param self_ the calling List object 
   * @alloc YES the caller becomes responsible for freeing the return value 
   * @crash YES crashes if self_ is empty 
   * @return the value of the node at the head of the list, by copy
   * */
  char * (*pop)(struct List * self_);

  /* Count returns the number of nodes in the list. 
   * @return length 
   * */
  int (*count)(const struct List * self_);

  /* isEmpty determines whether the list is empty. 
   * @return true if and only if the list is empty 
   * */
  BOOL (*isEmpty)(const struct List * self_);

} List;

/* Allocates an initializes a new empty List object.
 * @see release_list
 * @ctor THIS is the constructor for Clas List
 * @alloc YES the caller is responsible for freeing the return value
 * @dtor YES List is a Class and instances must be freed by release_list 
 * @crash YES failed to malloc 
 * @partial upon initializing list, head and tail are NULL
 * @return an empty List object 
 * */
/*@partial@*/ List * init_list();

/* Allocates and returns a complete and independant copy of the given 
 * list object. The copy will contain independant copies of all of the 
 * nodes in the original list. If the given list was NULL the copy will
 * be NULL 
 * @see release_list
 * @param list the list to be copied 
 * @alloc YES the caller becomes responsible for freeing the return value 
 * @dtor YES List is a Class and instances must be freed by release_list 
 * @creash YES failed to malloc
 * @null YES if the given list was NULL
 * @partial a copy of an empty list will have head and tail equal to NULL
 * @return a complete and independant copy of the given list 
 * */
/*@null@*/ /*@partial@*/ List * copy_list(const List * list);

/* Deallocates and frees the given list and all of its members. 
 * @param the list to be free 
 * @dtor THIS is the destructor for Class List */
void release_list(/*@null@*/ /*@only@*/ const List * list);

/* Appends all of the tokens in the given string described by the given 
 * token separator to the given list. All strings appended in this way are
 * copies, independant of the given string. 
 * 
 * @see append, strtok
 * @post further calls to strtok(NULL, separator) will yield NULL
 * @param list the list to which tokens will be appended
 * @param string the string to be tokenized
 * @param separator a list of separators around which to tokenize 
 * @bang YES the given string is changed in place 
 * */
void appendTokens(List * list, char * string, const char * separator);

/* Much like appendTokens above, addTokens adds all of the tokens described
 * by a given separator in the given string to the given list. 
 * @see appendTokens, strtok
 * @post further calls to strtok(NULL, separator) will yield NULL 
 * @param list the list to which tokens will be added
 * @param string the string to be tokenized
 * @param separator a list of separators around which to tokenize 
 * @bang YES the given string is changed in place 
 * */
void addTokens(List * list, char * string, const char * separator);

/* Appends all of the tokens in the given string described by the given 
 * token separator to the given list along with the specified delimiter. 
 * In a sense these are not tokens at all, they are tokens followed by
 * some delimiter characters. For example: 
 *
 *   "ls & ls| wc;" --> { ls & , ls | , wc ; }
 * 
 * This makes it easier to analize what token delimited a given string 
 *
 * @see append, strtok
 * @post further calls to strtok(NULL, separator) will yield NULL
 * @param list the list to which tokens will be appended
 * @param string the string to be tokenized
 * @param separator a list of separators around which to tokenize 
 * @bang YES the given string is changed in place 
 * */
void appendTokensAndDelimiters(List * list, char * string, const char * separator);

#endif
