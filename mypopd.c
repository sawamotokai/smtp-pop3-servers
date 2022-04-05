#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);

struct serverState
{
    int authenticated;
    int awaitingPass;
    char *username;
    mail_list_t emails;
} state = {0, 0, NULL};

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
        return 1;
    }
    run_server(argv[1], handle_client);
    return 0;
}

void user(int fd, char *parts[], int argCount)
{
    if (argCount != 2)
    {
        send_formatted(fd, "-ERR Syntax error in parameters or arguments\r\n");
        state.awaitingPass = 0;
        return;
    }
    if (state.awaitingPass)
    {
        send_formatted(fd, "-ERR Already awaiting password for a different user\r\n");
        state.awaitingPass = 0;
        return;
    }
    if (state.authenticated)
    {
        send_formatted(fd, "-ERR Maildrop already locked\r\n");
        state.awaitingPass = 0;
        return;
    }
    if (is_valid_user(parts[1], NULL))
    {
        send_formatted(fd, "+OK enter pass\r\n");
        state.awaitingPass = 1;
        state.username = malloc(strlen(parts[1]) + 1);
        strcpy(state.username, parts[1]);
    }
    else
    {
        send_formatted(fd, "-ERR no mailbox for %s here\r\n", parts[1]);
        state.awaitingPass = 0;
    }
}

void pass(int fd, char *input)
{
    input[strlen(input) - 1] = '\0'; // remove the line break
    if (strlen(input) < 6)           // "PASS password" contains at least 6 characters
    {
        send_formatted(fd, "-ERR Syntax error in parameters or arguments\r\n");
        state.awaitingPass = 0;
        return;
    }
    char *password = input + 5; // actual password starts at the 6th character
    if (state.authenticated)
    {
        send_formatted(fd, "-ERR Maildrop already locked\r\n");
        return;
    }
    if (state.awaitingPass)
    {
        if (is_valid_user(state.username, password))
        {
            state.emails = load_user_mail(state.username);
            state.authenticated = 1;
            int cnt = get_mail_count(state.emails, 0);
            send_formatted(fd, "+OK maildrop has %d messages\r\n", cnt);
        }
        else
        {
            send_formatted(fd, "-ERR invalid password\r\n");
        }
    }
    else
    {
        send_formatted(fd, "-ERR USER command is required immediately before.\r\n");
    }
}

void stat(int fd, char *args[])
{
    if (!state.authenticated)
    {
        send_formatted(fd, "-ERR user not authenticated\r\n");
        return;
    }
    int count = get_mail_count(state.emails, 0);
    int size = get_mail_list_size(state.emails);
    send_formatted(fd, "+OK %d %d\r\n", count, size);
}

void list(int fd)
{
    printf("LIST\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void retr(int fd, char *args[])
{
    printf("RETR\n");
    if (args[1] == NULL)
    {
        send_formatted(fd, "-ERR invalid arguments\r\n");
        return;
    }
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void dele(int fd, char *args[])
{
    printf("DELE\n");
    if (args[1] == NULL)
    {
        send_formatted(fd, "-ERR invalid arguments\r\n");
        return;
    }
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void noop(int fd)
{
    printf("NOOP\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void rset(int fd)
{
    printf("RSET\n");
    send_formatted(fd, "+OK maildrop has x messages\r\n");
}

void quit(int fd)
{
    printf("QUIT\n");
    if (state.username)
        free(state.username);
    if (state.emails)
        destroy_mail_list(state.emails);
    state.awaitingPass = 0;
    state.authenticated = 0;
    send_formatted(fd, "+OK dewey POP3 server signing off\r\n");
}

void initRes(int fd)
{
    printf("INIT RES\n");
    send_formatted(fd, "+OK dewey POP3 server signing in\r\n");
}

void errRes(int fd)
{
    printf("ERR RES\n");
    send_formatted(fd, "-ERR dewey POP3 server signing in\r\n");
}

void handle_client(int fd)
{
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
    /* TO BE COMPLETED BY THE STUDENT */
    initRes(fd);
    while (1)
    {
        int res = nb_read_line(nb, recvbuf);
        if (res <= 0)
        {
            break;
        }
        char *parts[MAX_LINE_LENGTH + 1];
        char copy[MAX_LINE_LENGTH + 1];
        strcpy(copy, recvbuf);
        int argCount = split(copy, parts);
        if (strncasecmp(recvbuf, "USER", 4) == 0)
            user(fd, parts, argCount);
        else if (strncasecmp(recvbuf, "PASS", 4) == 0)
            pass(fd, recvbuf);
        else if (strncasecmp(recvbuf, "STAT", 4) == 0)
            stat(fd, parts);
        else if (strncasecmp(recvbuf, "LIST", 4) == 0)
            list(fd);
        else if (strncasecmp(recvbuf, "RETR", 4) == 0)
            retr(fd, parts);
        else if (strncasecmp(recvbuf, "DELE", 4) == 0)
            dele(fd, parts);
        else if (strncasecmp(recvbuf, "NOOP", 4) == 0)
            noop(fd);
        else if (strncasecmp(recvbuf, "RSET", 4) == 0)
            rset(fd);
        else if (strncasecmp(recvbuf, "QUIT", 4) == 0)
        {
            quit(fd);
            break;
        }
        else
        {
            errRes(fd);
        }
        if (strncasecmp(recvbuf, "USER", 4) != 0)
        {
            state.awaitingPass = 0;
        }
    }

    nb_destroy(nb);
}
