#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);
struct emailState
{
    char *sender;
    user_list_t recipients;
} state = {NULL, NULL};

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

void helo(int fd, char *givenName)
{
    printf("HELO\n");
    char *tempServer = "smtp.cs.ubc.ca";
    char *tempDomain = "pender.students.cs.ubc.ca";
    send_formatted(fd, "250 %s Hello %s [198.162.33.17], pleased to meet you\r\n", tempServer, tempDomain);
}

void heloErr(int fd)
{
    printf("HELO ERR\n");
    send_formatted(fd, "501 5.0.0 HELO requires domain address\r\n", NULL);
}

void ehlo(int fd)
{
    printf("EHLO\n");
    send_formatted(fd, "250 2.0.0 OK\r\n", NULL);
}

void mail(int fd, char *arg)
{
    if (state.sender != NULL)
    {
        send_formatted(fd, "503 5.5.0 Sender already specified\r\n");
        return;
    }
    char *start = strchr(arg, '<') + 1;
    char *end = strchr(arg, '>');
    char sender[end - start + 1];
    strncpy(sender, start, end - start);
    sender[end - start] = '\0';
    state.sender = sender;
    send_formatted(fd, "250 2.1.0 <%s>... Sender ok\r\n", sender);
}

void rcpt(int fd, char *arg)
{
    if (!state.sender)
    {
        send_formatted(fd, "503 5.0.0 Need MAIL before RCPT\r\n");
        return;
    }
    char *start = strchr(arg, '<') + 1;
    char *end = strchr(arg, '>');
    char recipient[end - start + 1];
    strncpy(recipient, start, end - start);
    recipient[end - start] = '\0';
    // validate recipient
    if (!is_valid_user(recipient, NULL))
    {
        send_formatted(fd, "550 5.1.1 <%s>... User unknown\r\n", recipient);
        return;
    }
    // add recipient to state
    add_user_to_list(&state.recipients, recipient);
    send_formatted(fd, "250 2.1.5 <%s>... Recipient ok\r\n", recipient);
}

void data(int fd)
{
    printf("DATA\n");
}

void rset(int fd)
{
    printf("RSET\n");
}

void vrfy(int fd)
{
    printf("VRFY\n");
}

void noop(int fd)
{
    printf("NOOP\n");
    send_formatted(fd, "250 OK\r\n");
}

void quit(int fd)
{
    printf("QUIT\n");
    send_formatted(fd, "221 OK\r\n");
}

void initRes(int fd)
{
    printf("HELLO THERE\r\n");
    char *tempDomain = "smtp.cs.ubc.ca\0";
    send_formatted(fd, "220 %s ESMTP Sendmail\r\n", tempDomain);
}

void errCmd(int fd)
{
    printf("INVALID\n");
    send_formatted(fd, "503 bad sequence of commands\r\n", NULL);
}

void handle_client(int fd)
{

    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

    struct utsname my_uname;
    uname(&my_uname);

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
        split(recvbuf, parts);
        char *command = parts[0];

        if (strcasecmp(command, "HELO") == 0)
        {
            if (parts[1] == NULL)
                heloErr(fd);
            else
                helo(fd, parts[1]);
        }

        else if (strcasecmp(command, "EHLO") == 0)
            ehlo(fd);
        else if (strcasecmp(command, "MAIL") == 0)
            mail(fd, parts[1]);
        else if (strcasecmp(command, "RCPT") == 0)
            rcpt(fd, parts[1]);
        else if (strcasecmp(command, "DATA") == 0)
            data(fd);
        else if (strcasecmp(command, "RSET") == 0)
            rset(fd);
        else if (strcasecmp(command, "VRFY") == 0)
            vrfy(fd);
        else if (strcasecmp(command, "NOOP") == 0)
            noop(fd);
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