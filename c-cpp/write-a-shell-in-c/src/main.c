/***************************//**
  
  @file   main.c
  
  @author Johann Barnard

  @date   May 3, 2017

  @brief  Libjohann SHell

******************************/

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Read a line of input from stdin.
 * @return The line from stdin.
 */
char *lsh_read_line(void) {
  // TODO
  return "foo";
}

/**
 * @brief (Niavely) split a line into tokens.
 * @param line The line.
 * @return NULL-terminated array of tokens.
 */
/*char **lsh_split_line(char *line) {
  // TODO
}*/

/**
 * @brief Execute shell built-in or launch program.
 * @param args NULL-terminated list of arguments.
 * @return 1 if the shell should continue running, or 0 if it should terminate.
 */
int lsh_execute(char **args) {
  // TODO
  return 0;
}

/**
 * @brief Loop, getting input and then executing it.
 */
void lsh_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    //line = lsh_read_line();
    //args = lsh_split_line(line);
    //status = lsh_execute(args);
    status = 0; // TODO: Remove this!
    //free(line);
    //free(args);
  } while(status);
}

/**
 * @brief Main entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return status code
 */
int main(int argc, char **argv) {
  // TODO: Load config files (if any)
  
  // Run command loop
  lsh_loop();

  // TODO: Perform any shutdown/cleanup

  return EXIT_SUCCESS;
}
