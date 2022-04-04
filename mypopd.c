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
} state = {0, 0, 0, NULL};

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
    if (state.authenticated)
    {
        send_response(fd, "-ERR Maildrop already locked\r\n");
        state.awaitingPass = 0;
        return;
    }
    if (is_valid_user(parts[1], NULL))
    {
        send_formatted(fd, "+OK enter pass\r\n");
        state.awaitingPass = 1;
        state.user = parts[1];
    }
    else
    {
        send_formatted(fd, "-ERR no mailbox for %s here\r\n", parts[1]);
        state.awaitingPass = 0;
    }
}

void pass(int fd, char *password)
{
    printf("PASS\n");
    if (state.authenticated)
    {
        send_formatted(fd, "-ERR Maildrop already locked\r\n");
        return;
    }
    if (state.awaitingPass)
    {
        // TODO: Check if password is valid
        send_formatted(fd, "+OK maildrop has x messages\r\n");
    }
    else
    {
        send_formatted(fd, "-ERR USER command is required immediately before.\r\n");
    }
}

void stat(int fd, char *args[])
{
    printf("STAT\n");
    if (args[1] != NULL)
    {
        send_formatted(fd, "-ERR invalid arguments\r\n");
        return;
    }

    send_formatted(fd, "+OK 2 320");
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
        int argCount = split(recvbuf, parts);
        char *command = parts[0];

        if (strcasecmp(command, "USER") == 0)
            user(fd, parts, argCount);
        else if (strcasecmp(command, "PASS") == 0)
            pass(fd, parts[1]);
        else if (strcasecmp(command, "STAT") == 0)
            stat(fd, parts);
        else if (strcasecmp(command, "LIST") == 0)
            list(fd);
        else if (strcasecmp(command, "RETR") == 0)
            retr(fd, parts);
        else if (strcasecmp(command, "DELE") == 0)
            dele(fd, parts);
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
        {
            errRes(fd);
        }
    }

    nb_destroy(nb);
}
