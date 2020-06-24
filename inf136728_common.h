#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <time.h>
#define STB_DEFINE
#include "inf136728_stb.h"

#define auto __auto_type

// for loop trough all elements of a stb_arr, where
// i.i = element index; i.t = element
#define foreach(V, A) for(struct { int i; typeof(*(A)) t; } (V) = {0};\
                       (V).i < stb_arr_len(A) && ((V).t = (A)[(V).i], true);\
                       (V).i++)

typedef struct timespec timespec;

long time_diff(timespec a, timespec b) {
    return (b.tv_sec - a.tv_sec)*1000000000 + b.tv_nsec - a.tv_nsec;
}

timespec get_wall_clock() {
        timespec ts = {0};
        clock_gettime(CLOCK_REALTIME, &ts);
        return ts;
}

timespec time_sub(timespec a, timespec b) {
    timespec result = {b.tv_sec - a.tv_sec, b.tv_nsec - a.tv_nsec};
    if(result.tv_nsec < 0) {
        result.tv_sec--;
        result.tv_nsec += 1000000000;
    }
    return result;
}

typedef enum {
    SEND_MESSAGE = 1300,
    REGISTER,
    LOGIN,
    LOGOUT,
    SUBSCRIBE,
    NOTIFY = 5000,
    RESULT
} MType;

typedef struct {
    long mtype;
    long timeout;
    bool notify;
} Subscription;

typedef struct {
    long mtype;
    union {
        // SEND_MESSAGE
        struct {
            long type;
            int  priority;
            char text[128];
        };

        // REGISTER and LOGIN
        struct {
            char name[32];
            int  id;
        };

        // SUBSCRIBE
        struct {
            Subscription subscription;
        };

        // RESULT
        struct {
            bool success;
            char response[128];
        };
    };
} Message;

#define MSGSZ sizeof(Message) - sizeof(long)
