#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);
struct emailState
{
    int usedEHLO;
    char *sender;
    user_list_t recipients;
} state = {0, NULL, NULL};

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

void helo(int fd, char *domainName, char *givenName)
{
    printf("HELO\n");
    send_formatted(fd, "250 %s Hello %s , pleased to meet you\r\n", domainName, givenName);
}

void heloErr(int fd)
{
    printf("HELO ERR\n");
    send_formatted(fd, "501 5.0.0 HELO requires domain address\r\n");
}

void ehlo(int fd, char *domainName, char *givenName)
{
    printf("EHLO\n");
    helo(fd, domainName, givenName);
    if (state.sender)
    {
        free(state.sender);
        state.sender = NULL;
    }
    if (state.recipients)
    {
        destroy_user_list(state.recipients);
        state.recipients = NULL;
    }
    state.usedEHLO = 1;
}

void mail(int fd, char *arg)
{
    if (!state.usedEHLO)
    {
        send_formatted(fd, "503 5.5.1 Error: send EHLO first\r\n");
        return;
    }
    if (state.sender != NULL)
    {
        send_formatted(fd, "503 5.5.0 Sender already specified\r\n");
        return;
    }
    char *start = strchr(arg, '<') + 1;
    char *end = strchr(arg, '>');
    if (!(start && end))
    {
        send_formatted(fd, "501 5.1.0 Syntax error in parameters\r\n");
        return;
    }
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
    if (!(start && end))
    {
        send_formatted(fd, "501 5.1.0 Syntax error in parameters\r\n");
        return;
    }
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

void data(int fd, net_buffer_t nb)
{
    if (!state.sender || !state.recipients)
    {
        send_formatted(fd, "503 5.0.0 Need Sender/RCPT before DATA\r\n");
        return;
    }
    send_formatted(fd, "354 Enter mail, end with \".\" on a line by itself\r\n");
    char *filename = "temp.txt";
    int file = open(filename, O_WRONLY | O_CREAT, 0644);
    while (1)
    {
        char line[MAX_LINE_LENGTH];
        int n = nb_read_line(nb, line);
        printf("%s", line);
        if (n <= 0)
        {
            // send_formatted(fd, "250 2.0.0 Message accepted for delivery\r\n");
            // break;
            break;
        }
        if (strcmp(line, ".\n") == 0)
        {
            send_formatted(fd, "250 2.0.0 Message accepted for delivery\r\n");
            break;
        }
        write(file, line, n);
    }
    close(file);
    save_user_mail(filename, state.recipients);
    remove(filename);
}

void rset(int fd)
{
    if (state.sender)
    {
        free(state.sender);
        state.sender = NULL;
    }
    if (state.recipients)
    {
        destroy_user_list(state.recipients);
        state.recipients = NULL;
    }
    send_formatted(fd, "250 2.0.0 Reset state\r\n");
}

void vrfy(int fd, char *args[])
{
    printf("VRFY\n");
    if (args[1] == NULL)
    {
        send_formatted(fd, "501 Requested action not taken: mailbox unavailable (e.g., mailbox not found, no access, or command rejected for policy reasons)\r\n");
        return;
    }
    if (args[2] != NULL)
    {
        send_formatted(fd, "501 Requested action not taken: mailbox unavailable (e.g., mailbox not found, no access, or command rejected for policy reasons)\r\n");
        return;
    }

    int isValid = is_valid_user(args[1], args[2]);
    if (isValid == 0)
    {
        send_formatted(fd, "550 Requested action not taken: mailbox unavailable. %s does not exist, or password is incorrect\r\n", args[1]);
    }
    else
    {
        send_formatted(fd, "250 Requested mail action okay, completed\r\n");
    }
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
    send_formatted(fd, "503 bad sequence of commands\r\n");
}

void handle_client(int fd)
{

    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

    struct utsname my_uname;
    uname(&my_uname);
    char *domainName = my_uname.nodename;
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
                helo(fd, domainName, parts[1]);
        }
        else if (strcasecmp(command, "EHLO") == 0)
            if (parts[1] == NULL)
                heloErr(fd);
            else
                ehlo(fd, domainName, parts[1]);
        else if (strcasecmp(command, "MAIL") == 0)
            mail(fd, parts[1]);
        else if (strcasecmp(command, "RCPT") == 0)
            rcpt(fd, parts[1]);
        else if (strcasecmp(command, "DATA") == 0)
            data(fd, nb);
        else if (strcasecmp(command, "RSET") == 0)
            rset(fd);
        else if (strcasecmp(command, "VRFY") == 0)
            vrfy(fd, parts);
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