#pragma once

#include <unistd.h>

typedef struct {
    // Command
    int sep;         // Separator
    int argc;        // Argument count
    char *argv[32];  // Argument list
    char *ofs;       // Output file string
    char *ifs;       // Input file string
    // Child process
    pid_t pid;       // Process ID
    int wait;        // Wait in foreground
    int exitcode;    // Exit code
    int ofd;         // Output file descriptor
    int ifd;         // Input file descriptor
} args_t;

char *autocomplete(char *str, int suggest);
int parse_cmdline(char *str, args_t *args);
