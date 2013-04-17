/* Andre Byrne
 * 100045589 */

/* Note space:
 *   List must conform to ARG_MAX
 *   strings should be less than MAX_ARG_STRLEN
 *   for every library call, check its return value!
 *   for those that may return NULL, make a safe version */

#include "vash.h"

int main () {

  Vash * vash = init_vash();
  int result;

  if (NULL != vash) {
    result = vash->start(vash);
    /* input = vash->prompt(vash);
    command_list = commandFactory->makeCommands(commandFactory, vash, input);
    vash->execute(command_list); */
  }

  release_vash(vash);

  return result;
}
