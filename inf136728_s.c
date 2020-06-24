#include "inf136728_common.h"
#include <errno.h>

int server_queue;

char **clients  = NULL;
int *ids        = NULL;
int *msg_queues = NULL;
bool *logged    = NULL;
Subscription **subscriptions = NULL;

void register_user(Message *m) {
    Message r = {0};
    r.mtype = RESULT;

    foreach(i, clients) {
        if(strcmp(i.t, m->name) == 0) {
            r.success = false;
            sprintf(r.response, "can't register user %s, name already taken", m->name);
            msgsnd(server_queue, &r, MSGSZ, 0);
            return;
        }
    }
    foreach(i, ids) {
        if(i.t == m->id) {
            r.success = false;
            sprintf(r.response, "can't register with id %d, it is already taken", m->id);
            msgsnd(server_queue, &r, MSGSZ, 0);
            return;
        }
    }

    int q = msgget(m->id, IPC_CREAT | 0777);
    if(q == -1) {
        r.success = false;
        sprintf(r.response, "error %d while creating queue %d", errno, m->id);
        msgsnd(server_queue, &r, MSGSZ, 0);
        return;
    }

    printf("registering user\nname: %s\nid: %d\n", m->name, m->id);

    char *name = malloc(sizeof(m->name));
    strcpy(name, m->name);
    stb_arr_push(clients, name);
    stb_arr_push(ids, m->id);

    stb_arr_push(msg_queues, q);
    stb_arr_push(logged, true);

    stb_arr_push(subscriptions, NULL);

    r.success = true;
    sprintf(r.response, "registration successful");
    msgsnd(server_queue, &r, MSGSZ, 0);
}

void login(Message *m) {
    Message r;
    r.mtype = RESULT;

    foreach(i, clients) {
        if(strcmp(i.t, m->name) == 0) {
            if(ids[i.i] != m->id) {
                r.success = false;
                sprintf(r.response, "can't log in, wrong id %d for user %s", m->id, m->name);
                msgsnd(server_queue, &r, MSGSZ, 0);
                return;
            }

            if(logged[i.i]) {
                sprintf(r.response, "can't log in, user %s already logged in", m->name);
                r.success = false;
                msgsnd(server_queue, &r, MSGSZ, 0);
                return;
            }

            strcpy(r.response, "login successful");
            logged[i.i] = true;
            r.success = true;
            msgsnd(server_queue, &r, MSGSZ, 0);
            return;
        }
    }

    r.success = false;
    sprintf(r.response, "can't log in, user %s does not exist", m->name);
    msgsnd(server_queue, &r, MSGSZ, 0);
}

void logout(int cliend_id) {
    logged[cliend_id] = false;
}

void subscribe(int client_id, Subscription s) {
    Message r = {0};
    r.mtype = RESULT;

    foreach(i, subscriptions[client_id]) {
        if(i.t.mtype == s.mtype) {
            sprintf(r.response, "message type %d already subscribed", s.mtype);
            r.success = false;
            msgsnd(msg_queues[client_id], &r, MSGSZ, 0);
            return;
        }
    }
    stb_arr_push(subscriptions[client_id], s);
    r.success = true;
    strcpy(r.response, "subscription successful");
    msgsnd(msg_queues[client_id], &r, MSGSZ, 0);
}

void send_message(int sender_id, Message *m) {
    printf("sending message of type %ld from %d\n", m->type, sender_id);
    m->mtype = m->type;
    foreach(s, subscriptions) {
        bool contains = false;
        bool notify = false;
        foreach(i, s.t) {
            if(s.i == sender_id) continue;
            if(i.t.mtype == m->type) {
                contains = true;
                notify = i.t.notify;
                break;
            }
        }
        if(contains) {
            int queue = msg_queues[s.i];
            printf("sending to queue %d\n", queue);
            int res = msgsnd(queue, m, MSGSZ, 0);
            if(notify) {
                Message notify_m;
                notify_m.mtype = NOTIFY;
                notify_m.type = m->type;
                msgsnd(msg_queues[s.i], &notify_m, MSGSZ, 0);
            }
            continue;
        }
    }
}

int main() {

    server_queue = msgget(1200, IPC_CREAT | 0777);

    auto last_frame = get_wall_clock();
    long dt = 0;

    while(1) {
        auto new_frame = get_wall_clock();
        dt = time_diff(last_frame, new_frame);
        last_frame = new_frame;

        // update subscriptions with timeout
        foreach(s, subscriptions) {
            foreach(i, subscriptions[s.i]) {
                if(s.t[i.i].timeout > 0)
                    s.t[i.i].timeout -= dt;
                if(s.t[i.i].timeout < 0)
                    stb_arr_delete(subscriptions[s.i], i.i);
            }
        }

        // respond to register and login requests
        Message m;
        int success = msgrcv(server_queue, &m, MSGSZ, -3999, IPC_NOWAIT);
        if(success > 0) {
            switch(m.mtype) {
                case LOGIN: {
                    login(&m);
                } break;
                case REGISTER: {
                    register_user(&m);
                } break;
                default: {
                    Message r;
                    r.mtype = RESULT;
                    sprintf(r.response, "incorrect message type: %d", m.mtype);
                    msgsnd(server_queue, &r, MSGSZ, 0);
                }
            }
        }

        // iterate through all client message queues
        foreach(i, msg_queues) {
            success = msgrcv(i.t, &m, MSGSZ, -3999, IPC_NOWAIT);
            if(success > 0) {
                switch(m.mtype) {
                    case SUBSCRIBE: {
                        subscribe(i.i, m.subscription);
                    } break;

                    case SEND_MESSAGE: {
                        send_message(i.i, &m);
                    } break;

                    case LOGOUT: {
                        logout(i.i);
                    } break;

                    default: {
                        Message r = {0};
                        r.mtype = RESULT;
                        printf("unknown message type: %d\n", m.mtype);
                        sprintf(r.response, "unknown message type: %d", m.mtype);
                        msgsnd(server_queue, &r, MSGSZ, 0);
                    } break;
                }
            }
        }

        timespec delta = {0, 33333333};
        nanosleep(&delta, 0);
    }

    return 0;
}
