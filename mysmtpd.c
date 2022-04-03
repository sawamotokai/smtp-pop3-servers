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
struct emailStates
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

void helo()
{
    printf("HELO\n");
}

void ehlo()
{
    printf("EHLO\n");
}

void mail(int fd, char *arg)
{
    if (state.sender != NULL)
    {
        send_formatted(fd, "503 5.5.0 Sender already specified");
        return;
    }
    printf("%s\n", arg);
    char *sender = NULL;
    char *start = strchr(arg, '<') + 1;
    char *end = strchr(arg, '>');
    strncpy(sender, start, end - start);
    printf("%s\n", sender);
}

void rcpt()
{
    printf("RCPT\n");
}

void data()
{
    printf("DATA\n");
}

void rset()
{
    printf("RSET\n");
}

void vrfy()
{
    printf("VRFY\n");
}

void noop(int fd)
{
    char msg[] = "250 OK\r\n";
    send_all(fd, msg, strlen(msg));
}

void handle_client(int fd)
{

    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

    struct utsname my_uname;
    uname(&my_uname);

    /* TO BE COMPLETED BY THE STUDENT */
    nb_read_line(nb, recvbuf);
    char *parts[MAX_LINE_LENGTH + 1];
    split(recvbuf, parts);
    char *command = parts[0];
    printf("%s\n", command);
    while (strcasecmp(command, "QUIT") != 0)
    {
        if (strcasecmp(command, "HELO") == 0)
            helo();
        else if (strcasecmp(command, "EHLO") == 0)
            ehlo();
        else if (strcasecmp(command, "MAIL") == 0)
            mail(fd, parts[1]);
        else if (strcasecmp(command, "RCPT") == 0)
            rcpt();
        else if (strcasecmp(command, "DATA") == 0)
            data();
        else if (strcasecmp(command, "RSET") == 0)
            rset();
        else if (strcasecmp(command, "VRFY") == 0)
            vrfy();
        else if (strcasecmp(command, "NOOP") == 0)
            noop(fd);
        nb_read_line(nb, recvbuf);
        split(recvbuf, parts);
        command = parts[0];
    }
    nb_destroy(nb);
}