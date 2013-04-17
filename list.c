/* Andre Byrne
 * 100045589 */

#include "list.h"

/* instance methods documented in list.h */
static int count(const List * self_);
static BOOL isEmpty(const List * self_);
static const Node * append(List * self_, const char * string);
static /*@null@*/ const Node * add(List * self_, const char * string);
static char * pop(List * self_);

/* Initializes and returns a new Node with the given string, pointing
 * to the given node. Next may be NULL, but string must not be.
 * @see release_node
 * @pre string is not NULL
 * @alloc YES the caller is responsible for freeing the return value
 * @ctor THIS is the constructor for Class Node
 * @dtor YES Node is a class and instances must be free by release_node
 * @null YES if string is NULL
 * @crash YES failed to malloc
 * @return a new Node containing the given string
 * */
static /*@null@*/ Node * init_node(const char * string, /*@null@*/ /*@only@*/ Node * next);

/* Makes a complete and independant copy of the given node, and recursively
 * copies the nodes it is linked to. No nodes that link to the given node
 * are copied, but all nodes linked to by a node linked to by the given
 * node are copied. If node is NULL, then the return value will be NULL.
 * @see release_node
 * @post the return value contains the /only/ pointer to the nodes
 *       recursively copied by this method.
 * @param node the node to be copied
 * @null YES if the given node was NULL the copy will be NULL
 * @crash YES failed to malloc
 * @alloc YES the caller is responsible for freeing the return value
 * @dtor YES Node is a class and instances must be freed by release_node
 * @return a complete and independant copy of the given node
 * */
static /*@null@*/ Node * copy_node(/*@null@*/ Node * node);

/* Deallocates all memory allocated in init_node.
 * @param node the node to be freed
 * @dtor THIS is the destructor for Class Node
 * */
static void release_node(/*@null@*/ /*@only@*/ const Node * node);

static Node * init_node(const char * string, Node * next) {

  Node * node;
  if (NULL == string) {

    return NULL;
  }

  node = (Node*) failSafeMalloc(sizeof(Node), "init_node");

  node->string = string_with_size(strlen(string) + 1, "init_node");
  (void)strcpy(node->string, string); /* node->string is modified in place */
  node->next = next;

  return node;
}

static void release_node(const Node * node) {

  if (NULL != node) {
    free((char *)node->string);
    free((char *)node);
  }

}

static Node * copy_node(Node * node) {

  Node * copy = NULL;

  if (NULL != node) {
    copy = (Node*) failSafeMalloc(sizeof(Node), "copy_node");

    copy->string = string_with_size(strlen(node->string) + 1, "copy_node");
    (void)strcpy(copy->string, node->string); /* copy->string is changed in place */

    copy->next = copy_node(node->next);
  }

  return copy;
}

void appendTokensAndDelimiters(List * list, /*@unused@*/ char * string, const char * separator) {
  char * token;

  /* ISSUE: see list.h */
  while (NULL != (token = va_strtok(string, separator))) {
    (void)list->append(list, token); /* list is changed in place */
    free(token);
  }

}

void appendTokens(List * list, /*@unused@*/ char * string, const char * separator) {
  char * token;

  /* null holder is string during the first iteration and NULL subsequently
   * see strtok(3) */
  char * null_holder = string;

  /* ISSUE: see list.h */
  while (NULL != (token = strtok(null_holder, separator))) {
    null_holder = NULL;
    (void)list->append(list, token); /* list is changed in place */
  }

}

void addTokens(List * list, /*@unused@*/ char * string, const char * separator) {
  char * str;

  /* null holder is string during the first iteration and NULL subsequently
   * see strtok(3) */
  char * null_holder = string;

  /* ISSUE: see list.h */
  while (NULL != (str = strtok(null_holder, separator))) {
    null_holder = NULL;
    (void)list->add(list, str); /* list is changed in place */
  }

}

List * init_list() {

  List * list = (List*) failSafeMalloc(sizeof(List), "init_list");

  list->head = NULL;
  list->tail = NULL;
  list->length = 0;

  list->count = count;
  list->isEmpty = isEmpty;

  list->append = append;
  list->add = add;
  list->pop = pop;

  return list;
}

void release_list(const List * list) {

  if (NULL != list) {
    if (NULL != list->head) {
      Node * current = list->head;
      Node * next = current;

      /* walk the list releasing nodes */
      while(NULL != next) {
        next = current->next;
        release_node(current);
        current = next;
      }
    }
  }

  free((char *)list);
}

List * copy_list(const List * list) {

  List * copy = NULL;

  if (NULL != list) {
    Node * current, * next;

    copy = (List*) failSafeMalloc(sizeof(List), "copy_list");

    copy->head = copy_node(list->head); /* point of entry for recursion */
    current = copy->head;
    next = current;

    /* walk to the tail */
    while(NULL != next) {
      next = current->next;
      copy->tail = current;
      current = next;
    }

    copy->length = list->length;

    copy->count = count;
    copy->isEmpty = isEmpty;

    copy->append = append;
    copy->pop = pop;

  }

  return copy;
}

int count(const List * self_) {

  return self_->length;
}

BOOL isEmpty(const List * self_) {

  return (0 == self_->length);
}

const Node * add(List * self_, const char * string) {
  List * const self = self_;

  if (NULL == string) return NULL;

  /* empty list */
  if (NULL == self->head) {
    self->head = init_node(string, NULL);
    self->tail = self->head;

  } else {
    Node * temp = init_node(string, NULL);

    temp->next = self->head;
    self->head = temp;

  }

  self->length++;

  return self->tail;
}

const Node * append(List * self_, const char * string) {
  List * const self = self_;

  if (NULL == string) return NULL;

  /* empty list */
  if (NULL == self->head) {
    self->head = init_node(string, NULL);
    self->tail = self->head;

  } else {
    self->tail->next = init_node(string, NULL);
    self->tail = self->tail->next;

  }

  self->length++;

  return self->tail;
}

char * pop(List * self_) {

  List * const self = self_;

  Node * node;
  char * string;

  /* empty list */
  if (NULL == self->head) {
    alertAndCrash("pop", "pop from an empty list");

  /* just one node */
  } else if (self->head == self->tail) {
    node = self->head;
    self->head = self->tail = NULL;
    self->length = 0;

  } else {
    node = self->head;
    self->head = node->next;
    self->length--;
  }

  string = string_with_size(strlen(node->string) + 1, "pop");
  strcpy(string, node->string);
  release_node(node);

  return string;
}

/* tiny test driver
int main () {

  List * list = init_list();
  List * list2;

  char * str = "hello";

  list->append(list, str);
  list->append(list, str);
  list->append(list, str);
  list2 = copy_list(list);

  while (0 == list->isEmpty(list)) {
    Node * node = list->pop(list);
    printf("%u\n", list->count(list));
    printf("%s\n", node->string);
  }

  while (0 == list2->isEmpty(list2)) {
    Node * node = list2->pop(list2);
    printf("%u\n", list2->count(list2));
    printf("%s\n", node->string);
  }

  list->append(list, str);

  release_list(list);
  release_list(list2);

  return 0;
}
*/

