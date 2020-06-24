static int server_queue;
static int client_queue;
static bool logged = false;
static long *asynchronous;

Message client_register(char *name, int id) {

    Message m = {0};
    m.mtype = REGISTER;
    strcpy(m.name, name);
    m.id = id;
    
    msgsnd(server_queue, &m, MSGSZ, 0);
    msgrcv(server_queue, &m, MSGSZ, RESULT, 0);

    if(m.success) {
        client_queue = msgget(id, IPC_CREAT | 0777);
        logged = true;
    }

    return m;
}

int msgcmp(const void *a, const void *b) {
    int pa = ((Message *)a)->priority;
    int pb = ((Message *)b)->priority;
    if(pa > pb) return 1;
    else if(pa == pb) return 0;
    else return -1;
}

void client_exit() {
    Message m;
    m.mtype = LOGOUT;
    msgsnd(client_queue, &m, MSGSZ, 0);
    logged = false;
    exit(0);
}

void client_send(long type, int priority, char *text) {
    Message m;
    m.mtype = SEND_MESSAGE;
    m.type = type;
    m.priority = priority;
    strcpy(m.text, text);

    int res = msgsnd(client_queue, &m, MSGSZ, 0);
}

Message client_login(char *name, int id) {
    Message m = {0};
    m.mtype = LOGIN;
    strcpy(m.name, name);
    m.id = id;
    msgsnd(server_queue, &m, MSGSZ, 0);

    Message r = {0};
    msgrcv(server_queue, &r, MSGSZ, RESULT, 0);
    if(r.success) {
        logged = true;
        client_queue = msgget(m.id, IPC_CREAT | 0777);
    }
    return r;
}

void client_logout() {
    Message m;
    m.mtype = LOGOUT;
    msgsnd(client_queue, &m, MSGSZ, 0);
    logged = false;
}

Message client_subscribe(long type, bool notify, long timeout) {
    if(!logged) {
        Message r = {0};
        r.mtype = RESULT;
        r.success = false;
        strcpy(r.response, "can't subscribe when not logged in");
        return r;
    }

    Message m = {0};
    m.mtype = SUBSCRIBE;
    m.subscription.mtype = type;
    m.subscription.notify = notify;
    m.subscription.timeout = timeout;
    if(m.subscription.mtype < 4000 || m.subscription.mtype > 4999) {
        Message r = {0};
        r.mtype = RESULT;
        r.success = false;
        strcpy(r.response, "legal type values are 4000..4999");
        return r;
    }
    m.subscription.timeout *= 1000000000;
    msgsnd(client_queue, &m, MSGSZ, 0);

    Message r = {0};
    msgrcv(client_queue, &r, MSGSZ, RESULT, 0);
    return r;
}

Message *client_inbox(long type) {
    Message *m = NULL;
    Message r = {0};
    if(type == 0) type = -4999;
    while(msgrcv(client_queue, &r, MSGSZ, type, IPC_NOWAIT) > 0) {
        stb_arr_push(m, r);
    }
    qsort(m, stb_arr_len(m), sizeof(Message), msgcmp);

    return m;
}

int client_commandline(int argc, char **argv) {
    server_queue = msgget(1200, IPC_CREAT | 0777);

    char buf[512];
    while(1) {
        printf("enter command: ");
        scanf(" %[^\n]s ", buf);

        if(stb_regex("^exit$", buf)) {
            client_exit();
        } 

        else if(stb_regex("^register [a-zA-Z]+ [0-9]+$", buf)) {
            if(logged) {
                puts("can't register while logged in");
                continue;
            }
            char name[32] = {0};
            int id = 0;
            sscanf(buf, "%*s %s %d", name, &id);
            auto m = client_register(name, id);
            puts(m.response);
        }

        else if(stb_regex("^send [0-9]+ [0-9]+ .", buf)) {
            if(!logged) {
                puts("can't send messages without logging in");
                continue;
            }

            Message m;
            m.mtype = SEND_MESSAGE;
            sscanf(buf, "%*s %ld %d %[^\n]s", &m.type, &m.priority, m.text);
            if(m.type < 4000 || m.type > 4999) {
                printf("legal type values are 4000..4999\n");
                continue;
            }
            int res = msgsnd(client_queue, &m, MSGSZ, 0);
        }

        else if(stb_regex("^login [a-zA-Z]+ [0-9]+", buf)) {
            char *name;
            int id;
            sscanf(buf, "%*s %s %d", name, &id);
            auto r = client_login(name, id);
            puts(r.response);
        }

        else if(strcmp("logout", buf) == 0) {
            client_logout();
        }

        else if(stb_regex("^subscribe [0-9]+ [0-1] [0-9]+", buf)) {
            long type;
            bool notify;
            long timeout;
            sscanf(buf, "%*s %ld %d %ld", &type, &notify, &timeout);
            auto r = client_subscribe(type, notify, timeout);
            puts(r.response);
        }

        else if(strcmp("inbox", buf) == 0) {
            if(!logged) {
                puts("need to log in to receive messages");
                continue;
            }

            auto m = client_inbox(0);
            foreach(i, m) {
                puts("message:");
                puts(i.t.text);
            }

            stb_arr_free(m);
        }

        else puts("cannot parse the command");
    }

    return 0;
}
