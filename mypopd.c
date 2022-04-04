#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);

int main(int argc, char *argv[]) {

    if (argc != 2) {
	    fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
	    return 1;
    }
    run_server(argv[1], handle_client);
    return 0;
}

void user(int fd, char *userName) {
    printf("USER\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void pass(int fd, char *password) {
    printf("PASS\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void stat(int fd) {
    printf("STAT\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void list(int fd) {
    printf("LIST\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void retr(int fd) {
    printf("RETR\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void dele(int fd) {
    printf("DELE\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void noop(int fd) {
    printf("NOOP\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void rset(int fd) {
    printf("RSET\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void quit(int fd) {
    printf("QUIT\n");
    send_formatted(fd, "+OK dewey POP3 server signing off\r\n");
}

void handle_client(int fd) {
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
    /* TO BE COMPLETED BY THE STUDENT */

    while (1)
    {
        int res = nb_read_line(nb, recvbuf);
        if (res <= 0)
        {
            break;
        }
        char *parts[MAX_LINE_LENGTH + 1];
        split(recvbuf, parts);
        char *command = parts[0];

        
        if (strcasecmp(command, "USER") == 0)
            user(fd, parts[1]);
        else if (strcasecmp(command, "PASS") == 0)
            pass(fd, parts[1]);
        else if (strcasecmp(command, "STAT") == 0)
            stat(fd);
        else if (strcasecmp(command, "LIST") == 0)
            list(fd);
        else if (strcasecmp(command, "RETR") == 0)
            retr(fd);
        else if (strcasecmp(command, "DELE") == 0)
            dele(fd);
        else if (strcasecmp(command, "NOOP") == 0)
            noop(fd);
        else if (strcasecmp(command, "RSET") == 0)
            rset(fd);
        else if (strcasecmp(command, "QUIT") == 0)
        {
            quit(fd);
            break;
        }
        else
            errCmd(fd);
    }

    nb_destroy(nb);
}
