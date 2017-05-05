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
 * @brief Execute shell built-in or launch program.
 * @param args NULL-terminated list of arguments.
 * @return 1 if the shell should continue running, or 0 if it should terminate.
 */
int lsh_execute(char **args) {
  // TODO
  return 0;
}

#define LSH_RL_BUFSIZE 1024
/**
 * @brief Read a line of input from stdin.
 * @return The line from stdin.
 */
char *lsh_read_line(void) {
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF or '\n', replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }

    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      
      buffer = realloc(buffer, bufsize);

      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
 * @brief (Niavely) split a line into tokens.
 * @param line The line.
 * @return NULL-terminated array of tokens.
 */
char **lsh_split_line(char *line) {
  int bufsize = LSH_TOK_BUFSIZE;
  int position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);

  while (token != NULL) {
    tokens[position] = token;

    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;

      tokens = realloc(tokens, bufsize * sizeof(char*));

      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }

  tokens[position] = NULL;

  return tokens;
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
    line = lsh_read_line();
    args = lsh_split_line(line);
    //status = lsh_execute(args);
    status = 0; // TODO: Remove this!
    free(line);
    free(args);
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
