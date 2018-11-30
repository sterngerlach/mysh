
/* raw-mode.c */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios termios_old;

int tty_cbreak_mode();
void tty_reset();

int tty_cbreak_mode()
{
    struct termios termios_cbreak;

    if (tcgetattr(STDIN_FILENO, &termios_old) < 0)
        return -1;

    atexit(tty_reset);

    termios_cbreak = termios_old;
    termios_cbreak.c_lflag &= ~(ECHO | ICANON);
    termios_cbreak.c_cc[VMIN] = 1;
    termios_cbreak.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_cbreak) < 0)
        return -1;

    return 0;
}

void tty_reset()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_old);
}

int main(int argc, char** argv)
{
    char c;
    char* buf;
    
    tty_cbreak_mode();

    while ((c = getchar()) != EOF) {
        if (iscntrl(c)) {
            printf("%d\n", c);
        } else {
            printf("%d ('%c')\n", c, c);
        }

        if (c == 'q')
            break;
    }

    return 0;
}

