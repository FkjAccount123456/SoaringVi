/*
实在等不了了，赶紧先写一个最小版本
*/
#ifdef _WIN32
#define NOMINMAX
#define NOINTERFACE
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include "drawer.h"
#include "wcwidth/wcwidth.h"
#include <assert.h>
#include <wchar.h>

#define colortext_statusline(x) (colortext){.fg = {0, 0, 0}, .bg = {192, 192, 192}, .style = 0, .ch = x}

int main(int argc, char *argv[]) {
    u_init();

    size_t term_h, term_w;
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    term_h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    term_w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    term_h = w.ws_row, term_w = w.ws_col;
#endif

    textmgr mgr;
    text_init(&mgr);
    drawer dr;
    screen scr;
    scr.data = scr.prev = NULL;
    screen_init(&scr, term_h, term_w);
    drawer_init(&dr, &scr, &mgr, 0, 0, term_h - 1, term_w, DRAWER_MODE_LINWRAP);
    dr.cfg.hscroff = dr.cfg.vscroff = 2;
    dr.cfg.linum = true;

    char file[1024] = "untitled";
    FILE *fp = NULL;
    if (argc > 1) {
        strcpy(file, argv[1]);
        fp = fopen(file, "r");
        if (fp) {
            rawstr text = seq_init(rawstr);
            char_t ch;
            while ((ch = fgetwc(fp)) != EOF) {
                if (ch != '\r')
                    seq_append(text, ch);
            }
            text_insert(&mgr, coord_new(0, 0), text);
            free(text.v);
            fclose(fp);
        }
    }

    char_t insertion[1];
    rawstr insertion_str = {insertion, 1, 2, sizeof(char_t)};

    bool running = true;
    size_t y = 0, x = 0, ideal_x = 0;
    while (running) {
        coord cursor = drawer_setcursor(&dr, y, x);
        drawer_draw(&dr);

        screen_change(&scr, term_h - 1, 0, colortext_statusline(' '));
        size_t i;
        for (i = 1; file[i - 1]; i++)
            screen_change(&scr, term_h - 1, i, colortext_statusline(file[i - 1]));
        for (i++; i < term_w; i++)
            screen_change(&scr, term_h - 1, i, colortext_statusline(' '));

        screen_flush(&scr);
        gotoxy(cursor.y, cursor.x);
        flush();
        char_t ch = u_getch();
        if (ch == K_ESC) {
            running = false;
        } else if (ch == K_BS) {
            coord l;
            if (x) {
                l = coord_new(y, x - 1);
            } else if (y) {
                l = coord_new(y - 1, mgr.text.v[y - 1].len);
            } else {
                goto end;
            }
            text_delete(&mgr, l, coord_new(y, x));
            y = l.y, ideal_x = x = l.x;
        end:;
        } else if (ch == K_UP) {
            if (y) {
                y--;
                x = mgr.text.v[y].len;
                if (x > ideal_x)
                    x = ideal_x;
            }
        } else if (ch == K_DOWN) {
            if (y < mgr.text.len - 1) {
                y++;
                x = mgr.text.v[y].len;
                if (x > ideal_x)
                    x = ideal_x;
            }
        } else if (ch == K_LEFT) {
            if (x) {
                ideal_x = --x;
            }
        } else if (ch == K_RIGHT) {
            if (x < mgr.text.v[y].len) {
                ideal_x = ++x;
            }
        } else if (ch == K_CTRL('z')) {
            coord l = text_undo(&mgr);
            y = l.y, ideal_x = x = l.x;
        } else if (ch == K_CTRL('y')) {
            coord l = text_redo(&mgr, -1);
            if (l.x != -1 && l.y != -1) {
                y = l.y, ideal_x = x = l.x;
            }
        } else if (isprintable(ch) || ch > 128 && wcwidth(ch)) {
            insertion[0] = ch;
            coord nxt = text_insert(&mgr, coord_new(y, x), insertion_str);
            y = nxt.y, x = ideal_x = nxt.x;
        }
    }

    text_free(&mgr);
    screen_free(&scr);

    u_fina();
    return 0;
}
