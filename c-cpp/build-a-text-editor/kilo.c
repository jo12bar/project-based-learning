/*** INCLUDES ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
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
   * @brief The height of the window in rows.
   */
  int screenrows;

  /**
   * @brief The width of the window in columns.
   */
  int screencols;

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

/**
 * @brief Get the current cursor position.
 * @param rows Pointer to how many rows over the cursor is.
 * @param cols Pointer to how many columns down the cursor is.
 * @return -1 on failure, 0 on success.
 */
int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}

/**
 * @brief Get current window size in rows and columns.
 * @param rows Pointer to the number of rows
 * @param cols Pointer to the number of columns
 * @return -1 if failed (rows & cols will not be set), 0 if success (rows & cols
 *         will be set)
 */
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);
  } else {
   *cols = ws.ws_col;
   *rows = ws.ws_row;
   return 0;
  }
}

/*** APPEND BUFFER ***/

/**
 * @brief Appendable buffer of chars (a.k.a. a dynamic string).
 */
struct abuf {
  /**
   * @brief Pointer to the buffer in memory.
   */
  char *b;

  /**
   * @brief Length of the buffer.
   */
  int len;
};

/**
 * @brief An empty abuf (appendable buffer).
 */
#define ABUF_INIT { NULL, 0 }

/**
 * @brief Append a string of char's to an instance of abuf.
 * @param abuf A pointer to the appendable buffer.
 * @param s The string of char's to be appended.
 * @param len The length of the string to be appended.
 */
void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;

  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

/**
 * @brief Free an appendable buffer from memory.
 */
void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** OUTPUT ***/

/**
 * @brief Draw a column of tildes along the left hand side of the screen.
 * @param abuf Appendable buffer to draw to.
 */
void editorDrawRows(struct abuf *ab) {
  int i;

  for (i = 0; i < E.screenrows; i++) {
    abAppend(ab, "~", 1);

    // Clear the row to the right of the cursor.
    abAppend(ab, "\x1b[K", 3);

    // Write \r\n on every line *except* the last one (to prevent the terminal
    // from scrolling & hiding the topmost tilde).
    if (i < E.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

/**
 * @brief Refresh (re-render) the screen.
 */
void editorRefreshScreen() {
  // All write() operations will be stored in this buffer, to be drawn at the
  // end of this function.
  struct abuf ab = ABUF_INIT;

  // Hide the cursor (in supported terminals).
  abAppend(&ab, "\x1b[?25l", 6);

  // Position the cursor at the top left
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);

  abAppend(&ab, "\x1b[H", 3);

  // Reshow the cursor (again, in supported terminals).
  abAppend(&ab, "\x1b[?25h", 6);

  // Write the draw buffer to stdout, and free it from memory.
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
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
 * @brief Initialize various properties of the editor.
 */
void initEditor() {
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

/**
 * @brief Main entry point.
 */
int main() {
  enableRawMode();
  initEditor();

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
