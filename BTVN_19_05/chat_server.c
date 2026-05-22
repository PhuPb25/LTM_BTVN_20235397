/*******************************************************************************
 * @file    chat_server.c
 * @brief   Multithread chat server
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUF_SIZE    512

typedef struct {

    int sock;

    int logged_in;

    char id[50];

    char name[50];

} Client;

Client clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex =
    PTHREAD_MUTEX_INITIALIZER;

/******************************************************************************/
/* Xoa client */
/******************************************************************************/

void remove_client(int sock) {

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i].sock == sock) {

            clients[i].sock = 0;
            clients[i].logged_in = 0;

            memset(clients[i].id, 0,
                   sizeof(clients[i].id));

            memset(clients[i].name, 0,
                   sizeof(clients[i].name));

            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

/******************************************************************************/
/* Broadcast */
/******************************************************************************/

void broadcast(int sender, char *msg) {

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i].sock != 0 &&
            clients[i].logged_in &&
            clients[i].sock != sender) {

            send(clients[i].sock,
                 msg,
                 strlen(msg),
                 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

/******************************************************************************/
/* Lay thoi gian hien tai */
/******************************************************************************/

void get_time_str(char *buf) {

    time_t now = time(NULL);

    struct tm *t = localtime(&now);

    strftime(buf,
             64,
             "%Y/%m/%d %I:%M:%S%p",
             t);
}

/******************************************************************************/
/* Thread xu ly client */
/******************************************************************************/

void *client_thread(void *params) {

    int client = *(int *)params;

    free(params);

    char buf[BUF_SIZE];

    printf("Client connected: %d\n", client);

    send(client,
         "Nhap theo format: client_id: client_name\n",
         44,
         0);

    Client *me = NULL;

    /**************************************************************************/
    /* Tim slot client */
    /**************************************************************************/

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i].sock == 0) {

            clients[i].sock = client;
            me = &clients[i];

            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    if (me == NULL) {

        send(client,
             "Server full\n",
             12,
             0);

        close(client);

        return NULL;
    }

    /**************************************************************************/
    /* Login */
    /**************************************************************************/

    while (!me->logged_in) {

        int len = recv(client,
                       buf,
                       sizeof(buf) - 1,
                       0);

        if (len <= 0) {

            close(client);

            remove_client(client);

            return NULL;
        }

        buf[len] = 0;

        buf[strcspn(buf, "\r\n")] = 0;

        char *colon = strchr(buf, ':');

        if (colon == NULL) {

            send(client,
                 "Sai format. Dung: client_id: client_name\n",
                 46,
                 0);

            continue;
        }

        *colon = 0;

        char *id = buf;

        char *name = colon + 1;

        while (*name == ' ')
            name++;

        if (strlen(id) == 0 ||
            strlen(name) == 0) {

            send(client,
                 "Sai format. Dung: client_id: client_name\n",
                 46,
                 0);

            continue;
        }

        strcpy(me->id, id);

        strcpy(me->name, name);

        me->logged_in = 1;

        char welcome[BUF_SIZE];

        snprintf(welcome,
                 sizeof(welcome),
                 "Welcome %s (%s)\n",
                 me->id,
                 me->name);

        send(client,
             welcome,
             strlen(welcome),
             0);

        printf("Client %d login: %s (%s)\n",
               client,
               me->id,
               me->name);
    }

    /**************************************************************************/
    /* Chat */
    /**************************************************************************/

    while (1) {

        int len = recv(client,
                       buf,
                       sizeof(buf) - 1,
                       0);

        if (len <= 0)
            break;

        buf[len] = 0;

        char timebuf[64];

        get_time_str(timebuf);

        char out[BUF_SIZE];

        snprintf(out,
            sizeof(out),
            "[%s] %.20s: %.400s",
            timebuf,
            me->id,
            buf);

        printf("%s", out);

        broadcast(client, out);
    }

    printf("Client disconnected: %d\n",
           client);

    close(client);

    remove_client(client);

    return NULL;
}

/******************************************************************************/
/* Main */
/******************************************************************************/

int main() {

    int listener =
        socket(AF_INET,
               SOCK_STREAM,
               IPPROTO_TCP);

    if (listener < 0) {

        perror("socket()");
        return 1;
    }

    int opt = 1;

    setsockopt(listener,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;

    addr.sin_addr.s_addr = INADDR_ANY;

    addr.sin_port = htons(8080);

    if (bind(listener,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {

        perror("bind()");
        close(listener);

        return 1;
    }

    if (listen(listener, 10) < 0) {

        perror("listen()");
        close(listener);

        return 1;
    }

    printf("Chat server listening on port 8080...\n");

    while (1) {

        int client =
            accept(listener,
                   NULL,
                   NULL);

        if (client < 0) {

            perror("accept()");
            continue;
        }

        pthread_t tid;

        int *pclient =
            malloc(sizeof(int));

        *pclient = client;

        if (pthread_create(&tid,
                           NULL,
                           client_thread,
                           pclient) != 0) {

            perror("pthread_create()");

            close(client);

            free(pclient);

            continue;
        }

        pthread_detach(tid);
    }

    close(listener);

    return 0;
}