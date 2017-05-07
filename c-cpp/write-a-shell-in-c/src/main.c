/***************************//**
  
  @file   main.c
  
  @author Johann Barnard

  @date   May 3, 2017

  @brief  Libjohann SHell

******************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Function declarations for builtin commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
 * List of builtin commands, followed by their corrosponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
 * Builtin commands implementations
 */

/**
 * @brief Builtin command: change directory.
 * @param args List of args. `args[0]` is "cd". `args[1]` is the directory.
 * @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
 * @brief Builtin command: print help.
 * @param args List of args.  Not examined.
 * @return Always returns 1, to continue executing.
 */
int lsh_help(char **args) {
  int i;

  printf("Johann Barnard's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("\nUse the man command for information on other programs.\n");
  return 1;
}

/**
 * @brief Builtin command: exit.
 * @param args List of args.  Not examined.
 * @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args) {
  return 0;
}

/**
 * @brief Launch a program and wait for it to terminate.
 * @param args NULL-terminated list of arguments (including program)
 * @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args) {
  pid_t pid;
  pid_t wpid;

  int status;
  
  // Duplicate this process.
  pid = fork();

  if (pid == 0) {
    // This is the child process! Replace it with args[0].
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking.
    perror("lsh");
  } else {
    // This is the parent process!
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
 * @brief Execute shell built-in or launch program.
 * @param args NULL-terminated list of arguments.
 * @return 1 if the shell should continue running, or 0 if it should terminate.
 */
int lsh_execute(char **args) {
  int i;

  if (args[0] == NULL) {
    // An empty command was entered, so do nothing and let the shell
    // continue executing.
    return 1;
  }

  // Search through the list of shell builtins.
  // If args[0] is in the list, execute the corrosponding function, and return
  // the result.
  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  // If we're at this point, then args[0] wasn't in the list of shell builtins.
  // Pass off args to lsh_launch, and return the result.
  return lsh_launch(args);
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
    status = lsh_execute(args);

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
