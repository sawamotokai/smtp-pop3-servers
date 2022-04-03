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
    printf("HELO\r\n");

    
    char arg0[MAX_LINE_LENGTH] = "250 ";
    // char *arg1 = name.nodename;
    char *arg2 = " Hello ";
    char *arg3 = " [invalid.ip.for.now] ";
    char *arg4 = ", pleased to meet you\r\n";
    strcat(arg0, arg2);
    strcat(arg0, givenName);
    strcat(arg0, arg3);
    strcat(arg0, arg4);
    
    send_all(fd, arg0, strlen(arg0));
}

void heloErr(int fd)
{
    printf("HELO ERR\n");
    char *output = "501 5.0.0 HELO requires domain address\r\n";
    send_all(fd, output, strlen(output));
}

void ehlo()
{
    printf("EHLO\n");
}

void mail()
{
    printf("MAIL\n");
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

void noop()
{
    printf("NOOP\n");
}

void quit(int fd){
    printf("QUIT\n");
    char arg0[MAX_LINE_LENGTH] = "221 2.0.0 ";
    // char *arg1 = name.nodename;
    char *arg1 = " closing connection\r\n";
    strcat(arg0, arg1);
    send_all(fd, arg0, strlen(arg0));
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
    printf("init message: %s\n", command);
    while (strcasecmp(command, "QUIT") != 0)
    {
        if (strcasecmp(command, "HELO") == 0){
            if(parts[1] == NULL)
                heloErr(fd);
            else
                helo(fd, parts[1]);
        }
            
        else if (strcasecmp(command, "EHLO") == 0)
            ehlo();
        else if (strcasecmp(command, "MAIL") == 0)
            mail();
        else if (strcasecmp(command, "RCPT") == 0)
            rcpt();
        else if (strcasecmp(command, "DATA") == 0)
            data();
        else if (strcasecmp(command, "RSET") == 0)
            rset();
        else if (strcasecmp(command, "VRFY") == 0)
            vrfy();
        else if (strcasecmp(command, "NOOP") == 0)
            noop();
        nb_read_line(nb, recvbuf);
        split(recvbuf, parts);
        command = parts[0];
    }
    quit(fd);
    nb_destroy(nb);
}