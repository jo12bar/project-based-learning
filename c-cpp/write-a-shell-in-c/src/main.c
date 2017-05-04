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
 * @brief Loop, getting input and then executing it.
 */
void lsh_loop(void) {
  // TODO
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
