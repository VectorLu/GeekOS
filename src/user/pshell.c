/*
 * A really, really simple shell program
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.21 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <conio.h>
#include <process.h>
#include <string.h>
#include <signal.h>

#define BUFSIZE 79
#define DEFAULT_PATH "/c:/a"

volatile int sigchld_may_be_valid = 0;
volatile int background_sigchlds = 0;

#define ISSPACE(c) ((c) == ' ' || (c) == '\t')

struct Process {
    char program[BUFSIZE + 1];
    char *command;
    int pid;
};

char *Strip_Leading_Whitespace(char *s);
void Trim_Newline(char *s);
char *Copy_Token(char *token, char *s);
void Spawn_Single_Command(struct Process *proc, const char *path);


int exitCodes = 0;

int main() {
    char commandBuf[BUFSIZE + 1];
    struct Process proc;
    char path[BUFSIZE + 1] = DEFAULT_PATH;
    char *command;

    /* Set attribute to gray on black. */
    Print("\x1B[37m");

    while (true) {
        /* Print shell prompt (bright cyan on black background) */
        Print("\x1B[1;36m#\x1B[37m ");

        /* Read a line of input */
        Read_Line(commandBuf, sizeof(commandBuf));
        command = Strip_Leading_Whitespace(commandBuf);
        Trim_Newline(command);

        /*
         * Handle some special commands
         */
        if(strcmp(command, "exit") == 0) {
            /* Exit the shell */
            break;
        } else if(strcmp(command, "pid") == 0) {
            /* Print the pid of this process */
            Print("%d\n", Get_PID());
            continue;
        } else if(strcmp(command, "exitCodes") == 0) {
            /* Print exit codes of spawned processes. */
            exitCodes = 1;
            continue;
        } else if(strncmp(command, "path=", 5) == 0) {
            /* Set the executable search path */
            strcpy(path, command + 5);
            continue;
        } else if(strcmp(command, "") == 0) {
            /* Blank line. */
            continue;
        }

        proc.command = Strip_Leading_Whitespace(command);

        if(!Copy_Token(proc.program, proc.command)) {
            Print("Error: invalid command\n");
            continue;
        }
        Spawn_Single_Command(&proc, path);
    }

    Print_String("DONE!\n");
    return 0;
}

/*
 * Skip leading whitespace characters in given string.
 * Returns pointer to first non-whitespace character in the string,
 * which may be the end of the string.
 */
char *Strip_Leading_Whitespace(char *s) {
    while (ISSPACE(*s))
        ++s;
    return s;
}

/*
 * Destructively trim newline from string
 * by changing it to a nul character.
 */
void Trim_Newline(char *s) {
    char *c = strchr(s, '\n');
    if(c != 0)
        *c = '\0';
}

/*
 * Copy a single token from given string.
 * If a token is found, returns pointer to the
 * position in the string immediately past the token:
 * i.e., where parsing for the next token can begin.
 * If no token is found, returns null.
 */
char *Copy_Token(char *token, char *s) {
    char *t = token;

    while (ISSPACE(*s))
        ++s;
    while (*s != '\0' && !ISSPACE(*s))
        *t++ = *s++;
    *t = '\0';

    return *token != '\0' ? s : 0;
}




/*
 * Spawn a single command.
 */
void Spawn_Single_Command(struct Process *proc, const char *path) {
    int pid;
    int background;
    if((background = (proc->command[strlen(proc->command) - 1] == '&')))
        proc->command[strlen(proc->command) - 1] = '\0';

    sigchld_may_be_valid += 1;
    if(background)
        background_sigchlds += 1;

    pid = proc->pid =
        Spawn_With_Path(proc->program, proc->command, path, background);
    if(pid < 0) {
        Print("Could not spawn process: %s\n", Get_Error_String(pid));
    } else {
        if(background) {
            Print("[%d]\n", pid);
        } else {
            int exitCode = Wait(pid);
            sigchld_may_be_valid -= 1;
            if(sigchld_may_be_valid < 0)
                Print("Internal child count consistency error\n");
            if(exitCodes)
                Print("Exit code was %d\n", exitCode);
        }
    }
}
