/*
实在等不了了，赶紧先写一个最小版本
*/
#include "drawer.h"
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <wchar.h>

#define colortext_statusline(x) (colortext){.fg = {0, 0, 0}, .bg = {192, 192, 192}, .style = 0, .ch = x}

int main(int argc, char *argv[]) {
    u_init();

    size_t term_h, term_w;
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    term_h = w.ws_row, term_w = w.ws_col;

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
            byte ch;
            while ((ch = fgetwc(fp)) != EOF) {
                if (ch != '\r')
                    seq_append(text, ch);
            }
            text_insert(&mgr, coord_new(0, 0), text);
            free(text.v);
            fclose(fp);
        }
    }

    byte insertion[1];
    rawstr insertion_str = {insertion, 1, 2, sizeof(byte)};

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
        char *key = NULL;
        byte ch = u_getch(&key);
        if (!key) {
            insertion[0] = ch;
            coord nxt = text_insert(&mgr, coord_new(y, x), insertion_str);
            y = nxt.y, x = ideal_x = nxt.x;
        } else {
            if (!strcmp(key, "<esc>")) {
                running = false;
            } else if (!strcmp(key, "<cr>")) {
                insertion[0] = '\n';
                coord nxt = text_insert(&mgr, coord_new(y, x), insertion_str);
                log("nxt.y: %zu, nxt.x: %zu\n", nxt.y, nxt.x);
                y = nxt.y, x = ideal_x = nxt.x;
            } else if (!strcmp(key, "<space>")) {
                insertion[0] = ' ';
                coord nxt = text_insert(&mgr, coord_new(y, x), insertion_str);
                y = nxt.y, x = ideal_x = nxt.x;
            } else if (!strcmp(key, "<bs>")) {
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
            } else if (!strcmp(key, "<up>")) {
                if (y) {
                    y--;
                    x = mgr.text.v[y].len;
                    if (x > ideal_x)
                        x = ideal_x;
                }
            } else if (!strcmp(key, "<down>")) {
                if (y < mgr.text.len - 1) {
                    y++;
                    x = mgr.text.v[y].len;
                    if (x > ideal_x)
                        x = ideal_x;
                }
            } else if (!strcmp(key, "<left>")) {
                if (x) {
                    ideal_x = --x;
                }
            } else if (!strcmp(key, "<right>")) {
                if (x < mgr.text.v[y].len) {
                    ideal_x = ++x;
                }
            } else if (!strcmp(key, "<C-z>")) {
                coord l = text_undo(&mgr);
                y = l.y, ideal_x = x = l.x;
            } else if (!strcmp(key, "<C-y>")) {
                coord l = text_redo(&mgr, -1);
                if (l.x != -1 && l.y != -1) {
                    y = l.y, ideal_x = x = l.x;
                }
            }
        }
    }

    text_free(&mgr);
    screen_free(&scr);

    u_fina();
    return 0;
}
