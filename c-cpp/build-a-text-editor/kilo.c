#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/**
 * @brief The original attributes for termios
 */
struct termios orig_termios;

/**
 * @brief Reset the terminal's attributes so that the user's shell isn't broken.
 */
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/**
 * @brief Enable the terminal's raw mode.
 */
void enableRawMode() {
  // Save the terminal's original attributes, and reset them on exit (so that
  // the user's shell doesn't break).
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);

  // Copy of the terminal's attributes to be modified.
  struct termios raw = orig_termios;

  raw.c_iflag &= ~(
      // Ensure that carriage returns (13, '\r') aren't automatically translated
      // into newlines (10, '\n'). This fixes Ctrl-M.
      ICRNL
      // Ignore XOFF (Ctrl-S) & XON (Ctrl-Q)
      | IXON
  );

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

  // Tell the terminal to use our modified attributes.
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/**
 * @brief Main entry point.
 */
int main() {
  enableRawMode();

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    if (iscntrl(c)) {
      printf("%d\n", c);
    } else {
      printf("%d ('%c')\n", c, c);
    }
  }

  return 0;
}
