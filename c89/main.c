#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>


/* basic types */

typedef unsigned int uint;
typedef int BOOL;
#define TRUE 1
#define FALSE 0


/* logging */

#define ENABLE_LOGGING 0

#if ENABLE_LOGGING
#define LOG(args) printf("%s:%d - ", __FILE__, __LINE__); printf args
#else
#define LOG(args) /**/
#endif


/* constants */

#define BITS_PER_UINT (sizeof(uint) * 8)

#define MAX_BOARD_WIDTH 1024
#define MAX_BOARD_HEIGHT 1024

typedef enum {
    C_BLANK,
    C_FRAME,
    C_BLACK,
    NUM_C
} ScreenChar;


/* domain types */

typedef struct {
    uint width;
    uint height;
} Size2D;

typedef struct {
    uint top;
    uint left;
    uint bottom;
    uint right;
} Rect2D;

typedef struct {
    Size2D bd_size;
    Size2D term_size;
} Config;

typedef struct {
    Size2D size;
    uint *cells;
} Board;

typedef struct {
    Config cfg;
    Board world;
    Board updated_world;
    Board screen;
    Rect2D visible_world;
    Rect2D display_area;
    BOOL running;
} State;


/* keyboard I/O */

struct termios original_tio;

void io_init(void)
{
    struct termios new_tio;

    /* Save current terminal settings */
    tcgetattr(STDIN_FILENO, &original_tio);
    new_tio = original_tio;
    /* Disable canonical mode and echo */
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    /* Apply new settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void io_tear_down(void)
{
    /* Restore original terminal settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

char io_get_key(void)
{
    char key;
    /* timeout structure passed into select */
    struct timeval tv;
    /* fd_set passed into select */
    fd_set fds;
    /* Set up the timeout */
    tv.tv_sec = 0;
    tv.tv_usec = 50000;

    /* Zero out the fd_set - make sure it's pristine */
    FD_ZERO(&fds);
    /* Set the FD that we want to read */
    FD_SET(STDIN_FILENO, &fds); /*STDIN_FILENO is 0 */
    /* select takes the last file descriptor value + 1 in the fdset to check,
       the fdset for reads, writes, and errors.  We are only passing in reads.
       the last parameter is the timeout.
       select will return if an FD is ready or the timeout has occurred */
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    /* return 0 if STDIN is not ready to be read. */
    if (FD_ISSET(STDIN_FILENO, &fds)) {
        read(STDIN_FILENO, &key, 1);
        return key;
    }
    return 0;
}


void print_usage(void)
{
    printf("Usage: gol <width> <height>\n");
    exit(-1);
}


Size2D get_term_size(void)
{
    Size2D sz;
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    sz.width = w.ws_col;
    sz.height = w.ws_row;

    LOG(("columns %d\n", sz.width));
    LOG(("lines %d\n", sz.height));

    return sz;
}


Config parse_config(int argc, char **argv)
{
    Config cfg;

    if (argc != 3) {
        print_usage();
    }

    if (!sscanf(argv[1], "%u", &cfg.bd_size.width)) {
        print_usage();
    }
    if (!sscanf(argv[2], "%u", &cfg.bd_size.height)) {
        print_usage();
    }

    if (cfg.bd_size.width > MAX_BOARD_WIDTH || cfg.bd_size.height > MAX_BOARD_HEIGHT) {
        LOG(("Max board size is %ux%u\n", MAX_BOARD_WIDTH, MAX_BOARD_HEIGHT));
        exit(-1);
    }

    cfg.term_size = get_term_size();

    return cfg;
}


uint get_row_buf_size(uint width, uint elem_bits)
/* Computes the buffer size, that is number of uints needed to
 * store a row of `width` elements of `elem_bits` bits. */
{
    uint row_num_bits, row_buf_size;

    row_num_bits = width * elem_bits;
    row_buf_size = row_num_bits / BITS_PER_UINT;
    if (row_num_bits % BITS_PER_UINT > 0) {
        row_buf_size += 1;
    }

    return row_buf_size;
}


Board bd_create(Size2D bd_size, size_t elem_bits)
{
    Board bd;
    uint row_buf_size, buf_size;

    assert(bd_size.width < MAX_BOARD_WIDTH && bd_size.height < MAX_BOARD_HEIGHT);

    bd.size = bd_size;
    LOG(("board size is %ux%u\n", bd_size.width, bd_size.height));
    row_buf_size = get_row_buf_size(bd_size.width, elem_bits);
    buf_size = row_buf_size * bd_size.height;
    LOG(("allocating cells buffer of size %u\n", buf_size));
    bd.cells = calloc(buf_size, sizeof(uint));

    return bd;
}


void init_world(State *s)
{
    uint w, h, row_buf_size;
    uint *wd = (uint*) s->world.cells;

    w = s->world.size.width;
    h = s->world.size.height;
    row_buf_size = get_row_buf_size(w, 1);

    wd[(h / 2 - 1) * row_buf_size] = 2 << (w / 2 - 1);
    wd[(h / 2) * row_buf_size] = 7 << (w / 2 - 1);
    wd[(h / 2 + 1) * row_buf_size] = 1 << (w / 2 - 1);
}


void init_display_areas(State *s)
{
    uint header_height = 3;
    if (s->world.size.width * 2 <= s->screen.size.width - 2) {
        s->visible_world.left = 0;
        s->visible_world.right = s->world.size.width * 2;
        s->display_area.left = (s->screen.size.width - s->world.size.width * 2) / 2 + 1;
        s->display_area.right = s->display_area.left + s->world.size.width * 2;
    }
    else {
        s->visible_world.left = 0;
        s->visible_world.right = s->screen.size.width - 2;
        s->display_area.left = 1;
        s->display_area.right = s->screen.size.width - 1;
    }
    if (s->world.size.height <= s->screen.size.height - 2 - header_height) {
        s->visible_world.top = 0;
        s->visible_world.bottom = s->world.size.height;
        s->display_area.top = (s->screen.size.height - s->world.size.height - header_height) / 2 + header_height;
        s->display_area.bottom = s->display_area.top + s->world.size.height;
    }
    else {
        s->visible_world.top = 0;
        s->visible_world.bottom = s->screen.size.height - 2 - header_height;
        s->display_area.top = header_height + 1;
        s->display_area.bottom = s->screen.size.height - 1;
    }
}


void show_cursor(BOOL show)
{
    if (show == TRUE) {
        printf("\33[?25h");
    }
    else {
        printf("\33[?25l");
    }
}


void update_screen(State *s)
{
    char *title = "THE GAME OF LIFE";
    uint i, j, m, n, t, l, b, r, w, h, x, y, o;
    uint row_buf_size;
    char *sc = (char*) s->screen.cells;
    uint *wd = (uint*) s->world.cells;

    t = s->display_area.top;
    l = s->display_area.left;
    b = s->display_area.bottom;
    r = s->display_area.right;
    w = s->screen.size.width;
    h = s->screen.size.height;
    x = s->visible_world.left;
    y = s->visible_world.top;
    row_buf_size = get_row_buf_size(s->world.size.width, 1);

    /* fill screen with spaces */
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j ++) {
            sc[i * w + j] = C_BLANK;
        }
    }

    /* draw header */
    for (j = 0; j < w; j++) {
        sc[j] = C_FRAME;
        sc[2 * w + j] = C_FRAME;
    }
    if (strlen(title) < w) {
        sprintf(&sc[1 * w + (w - strlen(title)) / 2], "%s", title);
    }

    /* draw frame */
    for (i = t - 1, j = l - 1; j <= r; j++) {
        sc[i * w + j] = C_FRAME;
    }
    for (++i; i < b; i++) {
        sc[i * w + l - 1] = C_FRAME;
        sc[i * w + r] = C_FRAME;
    }
    for (j = l - 1; j <= r; j++) {
        sc[i * w + j] = C_FRAME;
    }

    /* draw world */
    LOG(("row_buf_size = %u\n", row_buf_size));
    for (i = t, m = y; i < b; i++, m++) {
        for (j = l, n = x; j < r; j += 2, n++) {
            o = m * row_buf_size + n / BITS_PER_UINT;
            if ((wd[o] >> (n % BITS_PER_UINT)) & 0x1) {
                sc[i * w + j] = C_BLACK;
                sc[i * w + j + 1] = C_BLACK;
            }
        }
    }
}


void render_screen(Board screen)
{
    uint i, j, w, h;
    char c;
    char *sc = (char*) screen.cells;

    w = screen.size.width;
    h = screen.size.height;

    LOG(("render_screen\n"));

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j ++) {
            c = sc[i * w + j];
            if (c == C_FRAME) {
                printf("\u2591");
            }
            else if (c == C_BLACK) {
                printf("\u2588");
            }
            else if (c == C_BLANK) {
                printf(" ");
            }
            else {
                printf("%c", c);
            }
        }
    }
    for (i = 0; i < h; i++) {
        printf("\33[F");
    }
}


uint get_neighbors(uint *wd, uint i, uint j, uint w, uint h) {
    uint n = 0, o;
    uint row_buf_size = get_row_buf_size(w, 1);

    if (i > 0) {
        if (j > 0) {
            o = (i - 1) * row_buf_size + (j - 1) / BITS_PER_UINT;
            n += (wd[o] >> ((j - 1) % BITS_PER_UINT)) & 0x1;
        }
        o = (i - 1) * row_buf_size + j / BITS_PER_UINT;
        n += (wd[o] >> (j % BITS_PER_UINT)) & 0x1;
        if (j < w) {
            o = (i - 1) * row_buf_size + (j + 1) / BITS_PER_UINT;
            n += (wd[o] >> ((j + 1) % BITS_PER_UINT)) & 0x1;
        }
    }
    if (j > 0) {
        o = i * row_buf_size + (j - 1) / BITS_PER_UINT;
        n += (wd[o] >> ((j - 1) % BITS_PER_UINT)) & 0x1;
    }
    if (j < w) {
        o = i * row_buf_size + (j + 1) / BITS_PER_UINT;
        n += (wd[o] >> ((j + 1) % BITS_PER_UINT)) & 0x1;
    }
    if (i < h) {
        if (j > 0) {
            o = (i + 1) * row_buf_size + (j - 1) / BITS_PER_UINT;
            n += (wd[o] >> ((j - 1) % BITS_PER_UINT)) & 0x1;
        }
        o = (i + 1) * row_buf_size + j / BITS_PER_UINT;
        n += (wd[o] >> (j % BITS_PER_UINT)) & 0x1;
        if (j < w) {
            o = (i + 1) * row_buf_size + (j + 1) / BITS_PER_UINT;
            n += (wd[o] >> ((j + 1) % BITS_PER_UINT)) & 0x1;
        }
    }

    LOG(("n(%u,%u)=%u\n", i, j, n));
    return n;
}


void update_world(State *s)
{
    uint i, j, w, h, o, n;
    uint row_buf_size;
    uint *wd = (uint*) s->world.cells;
    uint *uw = (uint*) s->updated_world.cells;

    w = s->world.size.width;
    h = s->world.size.height;
    row_buf_size = get_row_buf_size(w, 1);

    memcpy(uw, wd, row_buf_size * h * sizeof(uint));

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            n = get_neighbors(wd, i, j, w, h);
            o = i * row_buf_size + j / BITS_PER_UINT;
            if (n < 2 || n > 3) {
                uw[o] &= ~(0x1 << (j % BITS_PER_UINT));
                LOG(("n(%u,%u)->0\n", i, j));
            }
            else if (n == 3) {
                uw[o] |= (0x1 << (j % BITS_PER_UINT));
                LOG(("n(%u,%u)->1\n", i, j));
            }
        }
    }

    memcpy(wd, uw, row_buf_size * h * sizeof(uint));
}


void handle_events(State *s)
{
    char key;

    key = io_get_key();
    if (key > 0) {
        LOG(("Key pressed %c\n", key));
        if (key == 'q' || key == 'Q') {
            s->running = FALSE;
        }
    }
}


void main_loop(State s)
{
    /* uint i = 0; */
    uint frame_delay = 50;  /* msec */
    s.running = TRUE;

    init_world(&s);
    init_display_areas(&s);

    while (s.running) {
        update_screen(&s);
        render_screen(s.screen);
        update_world(&s);
        handle_events(&s);
        /* TODO compute frame_delay each frame to ensure same FPS */
        usleep(frame_delay * 1000);
        /* i += 1;
           if (i > 50) {
               s.running = FALSE;
           } */
    }
}


int main(int argc, char **argv)
{
    State s;
    s.cfg = parse_config(argc, argv);

    printf("Welcome to the Game of Life!\n");
    s.world = bd_create(s.cfg.bd_size, 1);
    s.updated_world = bd_create(s.cfg.bd_size, 1);
    s.screen = bd_create(s.cfg.term_size, 8);

    io_init();
    show_cursor(FALSE);

    printf("  press 'q' to exit\n");
    main_loop(s);

    io_tear_down();
    show_cursor(TRUE);

    return 0;
}

