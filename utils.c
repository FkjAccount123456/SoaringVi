#include "utils.h"
#include <stdio.h>
#include <sys/time.h>
#include <wchar.h>
#include <locale.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

char u_obuf[U_OBUF_SIZE];
char u_ibuf = 0;

const char *u_style_map[8] = {
    "", "1;", "3;", "1;3;", "4;", "1;4;", "3;4;", "1;3;4;",
};

struct termios u_orig_termios, u_raw_termios;

trie u_ch2keymap;
bool u_ch2keymap_inited = false;

void u_init_term() {
    tcgetattr(STDIN_FILENO, &u_orig_termios);

    u_raw_termios = u_orig_termios;
    u_raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    u_raw_termios.c_oflag &= ~(OPOST);
    u_raw_termios.c_cflag |= (CS8);
    u_raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    u_raw_termios.c_cc[VMIN] = 1;
    u_raw_termios.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &u_raw_termios);
}

void u_fina_term() {
    tcsetattr(STDIN_FILENO, TCSADRAIN, &u_orig_termios);
}

void u_init() {
    u_init_term();

#ifdef DEBUG_MODE
    u_init_log();
#endif

    // setvbuf(stdout, u_obuf, _IOFBF, U_OBUF_SIZE);
    // flush();

    u_init_ch2keymap();

    setlocale(LC_ALL, "");
}

void u_fina() {
    trie_free(&u_ch2keymap);
    u_ch2keymap_inited = false;

    setvbuf(stdout, u_obuf, _IOFBF, U_OBUF_SIZE);

    u_fina_term();

#ifdef DEBUG_MODE
    u_fina_log();
#endif
}

#ifdef DEBUG_MODE

FILE *u_log = NULL;

void u_init_log() {
    u_log = fopen("log.txt", "w");
    if (!u_log)
        error("u_init_log: fopen");
    setvbuf(u_log, NULL, _IONBF, 0);
}

void u_fina_log() {
    fclose(u_log);
    u_log = NULL;
}

#endif // DEBUG_MODE

void gotoxy(size_t y, size_t x) {
    printf("\x1b[%zu;%zuf", y + 1, x + 1);
}

bool cotext_eq(colortext a, colortext b) {
    return a.bg[0] == b.bg[0] && a.bg[1] == b.bg[1] && a.bg[2] == b.bg[2] &&
           a.fg[0] == b.fg[0] && a.fg[1] == b.fg[1] && a.fg[2] == b.fg[2] &&
           a.style == b.style && a.ch == b.ch;
}

bool color_eq(colortext a, colortext b) {
    return a.bg[0] == b.bg[0] && a.bg[1] == b.bg[1] && a.bg[2] == b.bg[2] &&
           a.fg[0] == b.fg[0] && a.fg[1] == b.fg[1] && a.fg[2] == b.fg[2] &&
           a.style == b.style;
}

void cotext_print(colortext c) {
    printf("\x1b[");
    printf("%s", u_style_map[c.style]);
    printf("38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm", c.fg[0], c.fg[1], c.fg[2],
           c.bg[0], c.bg[1], c.bg[2]);
    printf("%lc", c.ch);
}

void u_init_ch2keymap() {
    trie_init(&u_ch2keymap);
    u_ch2keymap_inited = true;
#define A(K, V) trie_insert(&u_ch2keymap, (unsigned char *)K, convert(void *, V))
    A("\x1d", K_C_RSQBR);
    A("\x1c", K_C_BACKSLASH);
    A("\x1f", K_C_SLASH);
    A("\x7f", K_BS);
    A("\r", K_CR);
    A("\x1b\t", K_M_TAB);
    A("\x1b\n", K_M_CR);
    A("\x1b ", K_M_SPACE);
    A("\x1b\x7f", K_M_BS);
    A("\x1b\x1d", K_M_C_RSQBR);
    A("\x1b\x1c", K_M_C_BACKSLASH);
    A("\x1b\x1f", K_M_C_SLASH);
    A("\x1b\x1b", K_M_ESC);
    A("\x1b[A", K_UP);
    A("\x1b[B", K_DOWN);
    A("\x1b[D", K_LEFT);
    A("\x1b[C", K_RIGHT);
    A("\x1b[H", K_HOME);
    A("\x1b[F", K_END);
    A("\x1bOP", K_F1);
    A("\x1bOQ", K_F2);
    A("\x1bOR", K_F3);
    A("\x1bOS", K_F4);
    A("\x1b[1;2A", K_S_UP);
    A("\x1b[1;2B", K_S_DOWN);
    A("\x1b[1;2D", K_S_LEFT);
    A("\x1b[1;2C", K_S_RIGHT);
    A("\x1b[1;2H", K_S_HOME);
    A("\x1b[1;2F", K_S_END);
    A("\x1b[1;5A", K_C_UP);
    A("\x1b[1;5B", K_C_DOWN);
    A("\x1b[1;5D", K_C_LEFT);
    A("\x1b[1;5C", K_C_RIGHT);
    A("\x1b[1;5H", K_C_HOME);
    A("\x1b[1;5F", K_C_END);
    A("\x1b[1;5P", K_C_F1);
    A("\x1b[1;5Q", K_C_F2);
    A("\x1b[1;5R", K_C_F3);
    A("\x1b[1;5S", K_C_F4);
    A("\x1b[1;3A", K_M_UP);
    A("\x1b[1;3B", K_M_DOWN);
    A("\x1b[1;3D", K_M_LEFT);
    A("\x1b[1;3C", K_M_RIGHT);
    A("\x1b[1;3H", K_M_HOME);
    A("\x1b[1;3F", K_M_END);
    A("\x1b[1;3P", K_M_F1);
    A("\x1b[1;3Q", K_M_F2);
    A("\x1b[1;3R", K_M_F3);
    A("\x1b[1;3S", K_M_F4);
    A("\x1b[15~", K_F5);
    A("\x1b[17~", K_F6);
    A("\x1b[18~", K_F7);
    A("\x1b[19~", K_F8);
    A("\x1b[15;5~", K_C_F5);
    A("\x1b[17;5~", K_C_F6);
    A("\x1b[18;5~", K_C_F7);
    A("\x1b[19;5~", K_C_F8);
    A("\x1b[15;3~", K_M_F5);
    A("\x1b[17;3~", K_M_F6);
    A("\x1b[18;3~", K_M_F7);
    A("\x1b[19;3~", K_M_F8);
    A("\x1b[2~", K_INS);
    A("\x1b[2;5~", K_C_INS);
    A("\x1b[2;3~", K_M_INS);
    A("\x1b[3~", K_DEL);
    A("\x1b[3;5~", K_C_DEL);
    A("\x1b[3;3~", K_M_DEL);
    A("\x1b[5;5~", K_C_PAGEUP);
    A("\x1b[5;3~", K_M_PAGEUP);
    A("\x1b[6;5~", K_C_PAGEDOWN);
    A("\x1b[6;3~", K_M_PAGEDOWN);
    A("\x1b[20~", K_F9);
    A("\x1b[21~", K_F10);
    A("\x1b[23~", K_F11);
    A("\x1b[24~", K_F12);
    A("\x1b[20;5~", K_C_F9);
    A("\x1b[21;5~", K_C_F10);
    A("\x1b[23;5~", K_C_F11);
    A("\x1b[24;5~", K_C_F12);
    A("\x1b[20;3~", K_M_F9);
    A("\x1b[21;3~", K_M_F10);
    A("\x1b[23;3~", K_M_F11);
    A("\x1b[24;3~", K_M_F12);
#undef A
}

long long get_usec() {
    struct timeval t = {0};
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000 * 1000 + t.tv_usec;
}

unsigned char u_basic_getch() {
    unsigned char res = u_ibuf;
    if (res)
        u_ibuf = 0;
    else
        read(STDIN_FILENO, &res, 1);
    return res;
}

bool u_kbhit() {
    if (u_ibuf)
        return true;
    u_raw_termios.c_cc[VMIN] = 0;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &u_raw_termios);
    if (read(STDIN_FILENO, &u_ibuf, 1) == 0)
        u_ibuf = 0;
    u_raw_termios.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &u_raw_termios);
    return u_ibuf;
}

unsigned char u_inputs[10];

byte u_getch() {
    char i = 0;
    unsigned char ch = u_basic_getch();
    if (u_ch2keymap.child[ch] && u_ch2keymap.child[ch]->is_leaf) {
        return convert(byte, u_ch2keymap.child[ch]->data);
    } else if (isprintable(ch)) {
        return ch;
    } else if (1 <= ch && ch <= 26) {
        return ch;
    } else if (ch == 27) {
        if (!u_kbhit()) {
            return K_ESC;
        }
        ch = u_basic_getch();
        trie *t = u_ch2keymap.child['\x1b'];
        if (t->child[ch] && t->child[ch]->is_leaf) {
            return convert(byte, t->child[ch]->data);
        } else if (0 <= ch && ch <= 26) {
            return K_M_CTRL(ch + 'a');
        } else if (!t->child[ch]) {
            return K_UNKNOWN;
        }
        t = t->child[ch];
        while (t) {
            ch = u_basic_getch();
            t = t->child[ch];
            if (!t)
                break;
            else if (t->is_leaf)
                return convert(byte, t->data);
        }
        return K_UNKNOWN;
    } else {
        char nxt = utf8_next(ch);
        if (nxt == -1)
            return K_UNKNOWN;
        u_inputs[i++] = ch;
        for (; i < nxt; i++)
            u_inputs[i] = u_basic_getch();
        byte res;
        utf8_cvt(u_inputs, &res);
        return res;
    }
}

char utf8_next(unsigned char ch) {
    if (ch < 0x80)
        return 1;
    if ((ch & 0xE0) == 0xC0)
        return 2;
    if ((ch & 0xF0) == 0xE0)
        return 3;
    if ((ch & 0xF8) == 0xF0)
        return 4;
    return -1;
}

char utf8_cvt(const unsigned char *code, byte *output) {
    char nbytes;
    wchar_t res = *code;
    if (res < 0x80)
        nbytes = 1, *output = res;
    else if ((res & 0xE0) == 0xC0)
        nbytes = 2, res &= 0x1F;
    else if ((res & 0xF0) == 0xE0)
        nbytes = 3, res &= 0x0F;
    else if ((res & 0xF8) == 0xF0)
        nbytes = 4, res &= 0x07;
    else
        return -1;

    for (char i = 1; i < nbytes; i++) {
        // if ((code[i] & 0xC0) != 0x80)
        //     return -1;
        res = (res << 6) | (code[i] & 0x3F);
    }
    *output = res;

    return nbytes;
}

void trie_insert(trie *t, unsigned char *key, void *data) {
    unsigned char *p = key;
    while (*p) {
        trie *child = t->child[*p];
        if (!child) {
            child = malloc(sizeof(trie));
            trie_init(child);
            t->child[*p] = child;
        }
        t = child;
        p++;
    }
    t->is_leaf = true;
    t->data = data;
}

void **trie_get(trie *t, unsigned char *key) {
    unsigned char *p = key;
    while (*p) {
        trie *child = t->child[*p];
        if (!child)
            return NULL;
        t = child;
        p++;
    }
    if (t->is_leaf)
        return &t->data;
    return NULL;
}

void trie_free(trie *t) {
    for (short i = 0; i < 256; i++) {
        if (t->child[i]) {
            trie_free(t->child[i]);
            free(t->child[i]);
        }
    }
}
