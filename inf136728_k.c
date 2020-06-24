#include <sys/ioctl.h>
#include <ncurses.h>
#include "inf136728_common.h"
#include "inf136728_client.h"

static int SCREEN_WIDTH  = 80;
static int SCREEN_HEIGHT = 24;

#define WIDTH  30
#define HEIGHT 10

char *choices[] = {
    "Register",
    "Login",
    "Logout",
    "Send message",
    "Subscribe",
    "Check inbox",
    "Exit",
};

int n_choices = sizeof(choices) / sizeof(char *);
void print_menu(WINDOW *menu_win, int highlight);

WINDOW *menu_win;

int main_menu() {
    // clear everything except last line, because we leave
    // error messages there
    for(int i = 0; i < SCREEN_HEIGHT-1; i++) {
        mvprintw(i, 0, "");
        clrtoeol();
    }
    int highlight = 1;
    int choice = 0;
    int c;

    menu_win = newwin(HEIGHT, WIDTH, (SCREEN_HEIGHT-HEIGHT)/2, (SCREEN_WIDTH-WIDTH)/2);
    keypad(menu_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to go up and down, Press enter to select a choice");
    refresh();
    print_menu(menu_win, highlight);

    nodelay(menu_win, true);
    while(!choice) {
        c = wgetch(menu_win);
        switch(c) {
            case KEY_UP:
                if(highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if(highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10:
                choice = highlight;
                break;
            case -1: {
                timespec delta = {0, 33333333};
                nanosleep(&delta, 0);
            } break;
            default:
#if 0
                mvprintw(SCREEN_HEIGHT-1, 0, "Character pressed is = %3d Hopefully it can be printed as '%c'", c, c);
                refresh();
#endif
                break;
        }

        Message m = {0};
        if(msgrcv(client_queue, &m, MSGSZ, NOTIFY, IPC_NOWAIT) > 0) {
            bool contains = false;
            foreach(i, asynchronous) {
                contains |= (m.type == i.t);
            }
            if(contains) {
                Message n = {0};
                msgrcv(client_queue, &n, MSGSZ, m.type, IPC_NOWAIT);
                mvprintw(SCREEN_HEIGHT-1, 0, "new message of type %ld: \"%s\"", m.type, n.text);
            }
            else {
                mvprintw(SCREEN_HEIGHT-1, 0, "new message of type %ld", m.type);
            }
            clrtoeol();
            refresh();
        }
        print_menu(menu_win, highlight);
    }

    nodelay(menu_win, false);
    return choice;
}

void print_menu(WINDOW *menu_win, int highlight) {
    int x, y, i;

    x = 2;
    y = 2;
    box(menu_win, 0, 0);
    for(i = 0; i < n_choices; ++i) {
        if(highlight == i + 1) {
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", choices[i]);
            wattroff(menu_win, A_REVERSE);
        }
        else
            mvwprintw(menu_win, y, x, "%s", choices[i]);
        ++y;
    }
    wrefresh(menu_win);
}

void print_edit(int y, int x, char *s, int cursor) {
    foreach(i, s) {
        if(i.i == cursor) attron(A_REVERSE);
        mvprintw(y, x+i.i, "%c", i.t);
        if(i.i == cursor) attroff(A_REVERSE);
    }
    if(cursor == stb_arr_len(s)) {
        attron(A_REVERSE);
        mvprintw(y, x+stb_arr_len(s), " ");
        attroff(A_REVERSE);
    }
    clrtoeol();
    refresh();
}

char *handle_input(char *buf, int *cursor, bool *end) {
    int c = getch();
    switch(c) {
        case 10:
            *end = true;
            break;

        case 127:
            if(*cursor > 0) {
                stb_arr_delete(buf, (*cursor)-1);
                (*cursor)--;
            }
            break;

        case 27:
            c = getch();
            c = getch();
            switch(c) {
                case 'C':
                    if(*cursor < stb_arr_len(buf)) (*cursor)++;
                    break;
                case 'D':
                    if(*cursor > 0) (*cursor)--;
                    break;
                case '3':
                    if(*cursor < stb_arr_len(buf))
                        stb_arr_delete(buf, *cursor);
                    break;
            }
            break;

        default:
            if(isalnum(c) || c == ' ') {
                stb_arr_insert(buf, *cursor, c);
                (*cursor)++;
            }
            break;
    }

    return buf; // we return it because it may have been reallocated
}

char *read_string(int y, int x) {
    int cursor = 0;
    char *buf = NULL;
    bool end = false;

    while(!end) {
        buf = handle_input(buf, &cursor, &end);
        print_edit(y, x, buf, cursor);
    }
    stb_arr_push(buf, 0);
    return buf;
}

char *string_prompt(char *text) {
    clear();

    char *buf;
    if(strlen(text) >= SCREEN_WIDTH/2-1) {
        mvprintw(SCREEN_HEIGHT/2-1, SCREEN_WIDTH/2-strlen(text)/2, text);
        buf = read_string(SCREEN_HEIGHT/2, SCREEN_WIDTH/2);
    } else {
        mvprintw(SCREEN_HEIGHT/2, SCREEN_WIDTH/2-strlen(text)-1, text);
        buf = read_string(SCREEN_HEIGHT/2, SCREEN_WIDTH/2);
    }

    return buf;
}

int int_prompt(char *text) {
    char *buf = string_prompt(text);
    int id = atoi(buf);
    stb_arr_free(buf);

    return id;
}

bool bool_prompt(char *text) {
    while(1) {
        char *buf = string_prompt(text);
        if(strcmp(buf, "yes") == 0) {
            stb_arr_free(buf);
            return true;
        }
        if(strcmp(buf, "no") == 0) {
            stb_arr_free(buf);
            return false;
        }
    }
}

int main() {
    server_queue = msgget(1200, IPC_CREAT | 0777);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    SCREEN_HEIGHT = w.ws_row;
    SCREEN_WIDTH  = w.ws_col;

    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(false);

    while(1) {
        int choice = main_menu();
        switch(choice) {
            // register
            case 1: {
                if(logged) {
                    mvprintw(SCREEN_HEIGHT-1, 0, "already logged in");
                    clrtoeol();
                    continue;
                }
                char *name = string_prompt("Enter user name:");
                int id = int_prompt("Enter user id:");
                auto r = client_register(name, id);
                mvprintw(SCREEN_HEIGHT-1, 0, r.response);
                stb_arr_free(name);
            } break;
            // login
            case 2: {
                if(logged) {
                    mvprintw(SCREEN_HEIGHT-1, 0, "already logged in");
                    clrtoeol();
                    continue;
                }
                char *name = string_prompt("Enter user name:");
                int id = int_prompt("Enter user id:");
                auto r = client_login(name, id);
                mvprintw(SCREEN_HEIGHT-1, 0, r.response);
                stb_arr_free(name);
            } break;
            // logout
            case 3: {
                if(!logged) {
                    mvprintw(SCREEN_HEIGHT-1, 0, "not logged in");
                    clrtoeol();
                    continue;
                }
                client_logout();
            } break;
            // send message
            case 4: {
                if(!logged) {
                    mvprintw(SCREEN_HEIGHT-1, 0, "can't send messages when not logged in");
                    clrtoeol();
                    continue;
                }
                char *text = string_prompt("Enter message:");
                long type = int_prompt("Enter message type (number between 4000 and 4999):");
                if(type < 4000 || type > 4999)
                    type = int_prompt("Enter message type (number between 4000 and 4999):");
                int priority = int_prompt("Enter message priority:");
                client_send(type, priority, text);
                stb_arr_free(text);
            } break;
            // subscribe
            case 5: {
                if(!logged) {
                    mvprintw(SCREEN_HEIGHT-1, 0, "can't subscribe messages when not logged in");
                    clrtoeol();
                    continue;
                }
                long type = int_prompt("Enter type:");
                bool notify = bool_prompt("Notify about new messages? (type 'yes' or 'no'):");
                if(notify) {
                    if(bool_prompt("Receive asynchronously? (type 'yes' or 'no'):")) {
                        stb_arr_push(asynchronous, type);
                    }
                }
                int timeout = int_prompt("Enter subscription timeout in seconds (0 for permanent):");
                auto r = client_subscribe(type, notify, timeout);
                mvprintw(SCREEN_HEIGHT-1, 0, r.response);
            } break;
            // inbox
            case 6: {
                if(!logged) {
                    mvprintw(SCREEN_HEIGHT-1, 0, "can't receive messages when not logged in");
                    clrtoeol();
                    continue;
                }
                long type = int_prompt("Enter type (number between 4000 and 4999, or 0 for all):");
                auto m = client_inbox(type);
                clear();
                foreach(i, m) {
                    mvprintw(i.i, 0, "type: %ld; \"%s\"", i.t.mtype, i.t.text);
                }
                mvprintw(SCREEN_HEIGHT-1, 0, "press any key to continue...");
                refresh();
                getch();
            } break;
            // exit
            case 7: {
                endwin();
                client_exit();
            } break;
        }
    }

    endwin();
    return 0;
}
