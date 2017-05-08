/*** INCLUDES ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** DEFINES ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** DATA ***/

/**
 * @brief Global editor config struct.
 */
struct editorConfig {
  /**
   * @brief The original attributes for termios
   */
  struct termios orig_termios;
};

/**
 * @brief Main instance of editorConfig.
 */
struct editorConfig E;

/*** TERMINAL ***/

/**
 * @brief Print an error message and exit the program.
 * @param s The error message
 */
void die(const char *s) {
  // Clear the screen
  write(STDOUT_FILENO, "\x1b[2J", 4);

  // Position the cursor at the top left
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

/**
 * @brief Reset the terminal's attributes so that the user's shell isn't broken.
 */
void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
    die("tcsetattr");
  }
}

/**
 * @brief Enable the terminal's raw mode.
 */
void enableRawMode() {
  // Save the terminal's original attributes, and reset them on exit (so that
  // the user's shell doesn't break).
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  // Copy of the terminal's attributes to be modified.
  struct termios raw = E.orig_termios;

  raw.c_iflag &= ~(
      // Keep break conditions from sending SIGINT.
      BRKINT
      // Ensure that carriage returns (13, '\r') aren't automatically translated
      // into newlines (10, '\n'). This fixes Ctrl-M.
      | ICRNL
      // Disable parity checking.
      | INPCK
      // Prevent the 8th bit of every input byte from being stripped.
      | ISTRIP
      // Ignore XOFF (Ctrl-S) & XON (Ctrl-Q)
      | IXON
  );

  // Ignore all output processing.
  raw.c_oflag &= ~(OPOST);

  // Set the character size to 8 bits per byte.
  raw.c_cflag |= (CS8);

  raw.c_lflag &= ~(
      // Turn off echoing
      ECHO
      // Turn off canonical mode (read input a byte at a time)
      | ICANON
      // Ignore the effects of Ctrl-V
      | IEXTEN
      // Ignore signals like SIGINT (Ctrl-C) & SIGTSTP (Ctrl-Z / Ctrl-Y)
      | ISIG
  );

  // Force read() to return after recieving more than 0 bytes.
  raw.c_cc[VMIN] = 0;

  // Force read() to return after 1/10th of a second (100ms)
  raw.c_cc[VTIME] = 1;

  // Tell the terminal to use our modified attributes.
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

/**
 * @brief Read keypresses from stdin.
 * @return The character read.
 */
char editorReadKey() {
  int nread;
  char c;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  return c;
}

/*** OUTPUT ***/

/**
 * Draw a column of tildes along the left hand side of the screen.
 */
void editorDrawRows() {
  int i;

  for (i = 0; i < 24; i++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

/**
 * @brief Refresh (re-render) the screen.
 */
void editorRefreshScreen() {
  // Clear the screen
  write(STDOUT_FILENO, "\x1b[2J", 4);

  // Position the cursor at the top left
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** INPUT ***/

/**
 * @brief Process keypresses recieved from editorReadKey()
 */
void editorProcessKeypress() {
  char c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
      // Clear the screen
      write(STDOUT_FILENO, "\x1b[2J", 4);

      // Position the cursor at the top left
      write(STDOUT_FILENO, "\x1b[H", 3);

      exit(0);
      break;
  }
}

/*** INIT ***/

/**
 * @brief Main entry point.
 */
int main() {
  enableRawMode();

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
