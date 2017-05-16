/*** INCLUDES ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

/*** DEFINES ***/

#define KILO_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

/**
 * @brief Control keys, such as arrow keys, for the editor.
 */
enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

/*** DATA ***/

/**
 * @brief A row of text.
 */
typedef struct erow {
  int size;
  char *chars;
} erow;

/**
 * @brief Global editor config struct.
 */
struct editorConfig {
  /** @brief Cursor x position (column). */
  int cx;

  /** @brief Cursor y position (row). */
  int cy;

  /**
   * @brief The height of the window in rows.
   */
  int screenrows;

  /**
   * @brief The width of the window in columns.
   */
  int screencols;

  /** @brief Number of rows of text in this document. */
  int numrows;

  /** @brief Current row that the cursor is on. */
  erow row;

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
int editorReadKey() {
  int nread;
  char c;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  // Handle escape sequences (i.e. <Home>, <Up>, <Right>, <Delete>...)
  if (c == '\x1b') {
    char seq[3];

    // If there aren't at least two more characters after <Escape> (like you
    // typically need in escape sequences - i.e. "<Escape>[A") then just return
    // <Escape>.
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    // Most (if not all) escape sequences need an '[' right after the <Escape>.
    if (seq[0] == '[') {
      // If the character after the '[' is between 0-9...
      if (seq[1] >= '0' && seq[1] <= '9') {
        // If there's nothing after that, just return <Escape>.
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';

        // If there is, and it's a tilde...
        if (seq[2] == '~') {
          // Alias to PAGE_UP, PAGE_DOWN, HOME_KEY, END_KEY, & DEL_KEY.
          switch (seq[1]) {
            case '1': return HOME_KEY;
            case '0': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        // Alias arrow keys to ARROW_UP, ARROW_DOWN, ARROW_RIGHT, & ARROW_LEFT.
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      // Some systems use 'O' instead of '[' in some of their escape codes.
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }

    // Give up and just return <Escape>.
    return '\x1b';
  } else {
    return c;
  }
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

/*** FILE I/O ***/

/**
 * @brief Read the first line of a file into E.row
 * @param filename The file to read.
 */
void editorOpen(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  
  linelen = getline(&line, &linecap, fp);

  if (linelen != 1) {
    while (linelen > 0 &&
        (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')
    ) {
      linelen--;
    }

    E.row.size = linelen;
    E.row.chars = malloc(linelen + 1);
    memcpy(E.row.chars, line, linelen);
    E.row.chars[linelen] = '\0';
    E.numrows = 1;
  }

  free(line);
  fclose(fp);
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
    if (i >= E.numrows) {
      // Draw tildes all the way down, as well as a welcome message 1/3 of the way
      // down.
      if (i == E.screenrows / 3) {
        char welcome[80];

        int welcomelen = snprintf(
          welcome,
          sizeof(welcome),
          "Kilo editor -- version %s",
          KILO_VERSION
        );

        if (welcomelen > E.screencols) welcomelen = E.screencols;

        // Center the greeting.
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }

        while (padding--) {
          abAppend(ab, " ", 1);
        }

        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = E.row.size;
      if (len > E.screencols) len = E.screencols;
      abAppend(ab, E.row.chars, len);
    }

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

  // Move cursor to position set in the global editor state.
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  abAppend(&ab, buf, strlen(buf));

  // Reshow the cursor (again, in supported terminals).
  abAppend(&ab, "\x1b[?25h", 6);

  // Write the draw buffer to stdout, and free it from memory.
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** INPUT ***/

/**
 * @brief Increment or decrement either E.cx or E.cy, based on the char passed.
 * @param key The key pressed. Is mapped ARROW_UP, ARROW_DOWN, ARROW_LEFT, and
 *            ARROW_RIGHT.
 */
void editorMoveCursor(int key) {
  switch (key) {
    case ARROW_LEFT:
      if (E.cx != 0) {
        E.cx--;
      }
      break;

    case ARROW_RIGHT:
      if (E.cx != E.screencols - 1) {
        E.cx++;
      }
      break;

    case ARROW_UP:
      if (E.cy != 0) {
        E.cy--;
      }
      break;

    case ARROW_DOWN:
      if (E.cy != E.screenrows - 1) {
        E.cy++;
      }
      break;
  }
}

/**
 * @brief Process keypresses recieved from editorReadKey()
 */
void editorProcessKeypress() {
  int c = editorReadKey();

  switch (c) {
    // Quit
    case CTRL_KEY('q'):
      // Clear the screen
      write(STDOUT_FILENO, "\x1b[2J", 4);

      // Position the cursor at the top left
      write(STDOUT_FILENO, "\x1b[H", 3);

      exit(0);
      break;

    // <Home>
    case HOME_KEY:
      E.cx = 0;
      break;

    // <End>
    case END_KEY:
      E.cx = E.screencols - 1;
      break;

    // <Page Up> & <Page Down>
    case PAGE_UP:
    case PAGE_DOWN:
      {
        int times = E.screenrows;
        while (times--) {
          editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        }
      }
      break;

    // Cursor movement
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
  }
}

/*** INIT ***/

/**
 * @brief Initialize various properties of the editor.
 */
void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.numrows = 0;

  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

/**
 * @brief Main entry point.
 */
int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();

  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
