/*
 * Vagrantshire (vsh) Shell program.
 * CSC 360, Summer 2021
 * Author: Gina Nasseri 
 *
 * When executed, this program creates an interactive shell which 
 * repeatedly prompts the user for commands to execute. The shell 
 * can handle simple commands with at most 9 arguments. Output 
 * can be redirected to an output file via ::<filename> and input
 * may be redirected from in input file via <filename>::. The 
 * execution time for a command can be obtained by ending a command
 * with "##".
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAX_NUM_ARGS 15
#define MAX_INPUT_LEN 100

/* -----------------------------------------------------------------------
 * Function: tokenize()
 * -----------------------------------------------------------------------
 * Tokenizes the user's input using strtok with " " as the deliminater. 
 * If an output and/or input file is given (token begins or ends with 
 * "::"), then the token is saved to the file array and will be processed 
 * later. If ## is encountered, then hash_flag is set to 1. The maximum 
 * number of arguments allowed for any given command is 9 (note that 
 * "arguments" refers specifically to arguments and options for the 
 * command to be executed, and therefore does not include filenames that 
 * begin or end with "::" or the command itself; however, ## is included 
 * as an argument).
 *   
 * - user_input: the user's input as a string 
 * - args: stores each token in user_input provided that it does not begin 
 *         or end with "::" 
 * - file: file[0] stores memory address of an input filename (token ends with "::")
 *         file[1] stores memory address of an output filename (token begins with "::")
 *         if one or both were provided by the user, otherwise are null. 
 * hash_flag: initially = 0, and set to 1 if "##" encountered as a token.
 * 
 * Returns: -1 if user syntax error occured (too many args, missing filename 
 * or command) otherwise returns the number of tokens in args.  
 * -------------------------------------------------------------------------- */
int tokenize(char user_input[], char *args[], char *file[], int *hash_flag) {
    char *t = strtok(user_input, " ");
    int num_args = 0;

    while(t) {
        if (num_args > 10) {
            fprintf(stderr, "error: maximum number of arguments [9] exceeded.\n");
            return -1;
        }
        else if (!strcmp(t, "::")) {
            fprintf(stderr,"error: filename missing before/after '::'\n");
            return -1;
        }
        else if (!strncmp(t, "::", 2)) {
            file[1] = t; //output file
        }
        else if (!strncmp(&t[strlen(t)-2], "::", 2)) {
            file[0] = t; //input file
        }
        else {
            if (!strncmp(t, "##",2))
                *hash_flag = 1;
            args[num_args] = t;
            num_args ++;
        }
        t = strtok(NULL, " ");
    }
    if (!num_args) {
        fprintf(stderr, "error: no command given.\n");
        return -1;
    }
    return num_args;
}

/* ------------------------------------------------
 * Function: get_filenames()
 * ------------------------------------------------
 * If output and/or input files were given with
 * user input then...
 *
 * file[1]: contains memory address of the output 
 *          filename in the format ::<filename>
 * file[0]: contains memory address of the input 
 *          filename in the format <filename>::
 *
 * otherwise, file[0] and/or file[1] are null.
 * 
 * Purpose: Uses strtok with "::" as the deliminater 
 * to remove the colons from the filename(s) and then
 * resets the address stored in file[i] (i = 0, 1) 
 * to that of the filename with the colons removed.
 * ------------------------------------------------ */
void get_filenames(char *file[]) {
    char *j;
    if (file[0]) {
        j = strtok(file[0],"::");
        file[0] = j;
    }
    if (file[1]) {
        j = strtok(file[1],"::");
        file[1] = j;
    }
    return;
}

/* ----------------------------------------------------
 * Function: complete_path()
 * ----------------------------------------------------
 * Parameters: 
 *  - char *path: pointer to a line in .vshrc
 *  - char *cmnd: pointer to user's command 
 *
 * Purpose: concatinates '/' followed by cmnd to path. 
 * ----------------------------------------------------- */
void complete_path(char *path, char *cmnd) {
    char fwd = '/';
    if (path[strlen(path) - 1] == '\n')
        path[strlen(path) - 1] = '\0';
    strncat(path, &fwd, 1);
    strncat(path, cmnd, MAX_INPUT_LEN);
}

/* ---------------------------------------------------------
 * Function: find_path()
 * ---------------------------------------------------------
 * Uses complete_path() to concatinate '/' + cmnd to a line 
 * in the .vshrc file and then uses access() to test the paths 
 * in .vshrc line by line until a either a path to the command 
 * is found or the end of file is reached. If a path is found, 
 * then the correct and completed path (with the command) will 
 * be stored in path.
 *
 * Parameters: 
 *  - char path[] : buffer, stores potential path of command
 *  - char cmnd[] : first token in user's input 
 *  - FILE *fp    : pointer to the .vshrc file 
 *
 * Returns: 0 if path found, -1 otherwise.  
 * --------------------------------------------------------- */
int find_path(char path[], char cmnd[], FILE *fp) {
    rewind(fp);
    while(fgets(path,MAX_INPUT_LEN,fp)) {
        complete_path(path,cmnd);
        if (!access(path, F_OK)) 
            return 0;
    }
    fprintf(stderr, "%s: command not found\n", cmnd);
    return -1;
}

/* -------------------------------------------------------
 * Function: hash_check()
 * -------------------------------------------------------
 * Parameters: 
 *  - char *args[]: mem addresses of user's command and any
 *    arguments/options given
 *  - hash_flag: if user gave ## as an argument, then 
 *    hash_flag = 1, otherwise hash_flag = 0
 *  - num_tokens: number of entries in args 
 * 
 * Purpose: if ## was given as an argument, then hash_check() 
 * ensures it was given at the end of the command before 
 * removing it from args.
 * 
 * Returns: -1 if hash_flag is 1 and ## was not the last token,
 * otherwise returns 0. 
 * -------------------------------------------------------- */
int hash_check(char *args[], int hash_flag, int num_tokens) {
    if (hash_flag) {
        if (strncmp(args[num_tokens -1], "##",2)) {
            fprintf(stderr, "error: ## must be placed at the end of a command.\n");
            return -1;
        }
        else {
            args[num_tokens - 1] = 0;
        }
    }
    return 0;
}

/* ----------------------------------------------------------
 * Function: report_time()
 * ----------------------------------------------------------
 * Calculates the total execution time (in microseconds) of a 
 * command that was just exectuted. If an output file was given, 
 * then the time is printed to the end of the output file. 
 * Otherwise, the time is printed to stdout. 
 *
 * Parameters: 
 *  - char *file[]: If an output file was provided, then file[1]
 *                  stores the memory address of the filename, 
 *                  otherwise file[1] is null. 
 *  - struct timeval... 
 *      before: time before fork() called
 *      after : time after waitpid() returns 
 *
 * Time calculation provided in Appendix F by Mike Zastre
 * ----------------------------------------------------------- */
void report_time(char *file[], struct timeval before, struct timeval after) {
    FILE *fp;
    unsigned long time;
    time = (after.tv_sec - before.tv_sec) * 1000000 + after.tv_usec - before.tv_usec;

    if(file[1]) {
        fp = fopen(file[1],"a");
        fprintf(fp,"Execution time: %lu microseconds\n", time);
        fclose(fp);
    }
    else {
        printf("Execution time: %lu microseconds\n", time);
    }
    return;
}

/* ---------------------------------------------------------
 * Function: write_to_file()
 * ---------------------------------------------------------
 * Parameters: 
 *  - char *args[]: stores memory address of user's command 
 *                  and any arguments/options given 
 *  - char *file[]: file[1] contains the memory address of the 
 *                  output filename provided by user. 
 *  - hash_flag = 1 if ## was given as last arg, otherwise
 *    hash_flag = 0. 
 *      
 * Purpose: executes user command stored in args and redirects
 * the command output to file[1]. The time immediately before and 
 * after execution is recorded so that that if hash_flag is 1, 
 * then report_time() can be called with these values and print
 * the execution time at the end of the output file. 
 *
 * Based on the Appendix C program provided by Mike Zastre.
 * --------------------------------------------------------- */
void write_to_file(char *args[], char *file[], int hash_flag) {
    char *envp[] = { 0 };
    int pid, status;
    int output_fd;
    struct timeval before, after;

    gettimeofday(&before,NULL);
    pid = fork();
    if (!(pid)) {
        output_fd = open(file[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if (output_fd == -1) {
            fprintf(stderr, "error: cannot open %s for writing.\n",file[1]);
            exit(1);
         }
        dup2(output_fd,1); // replace stdout with output file
        close(output_fd);
        execve(args[0],args,envp);
        exit(EXIT_FAILURE); // should never get here
    }
    waitpid(pid, &status, 0);
    gettimeofday(&after,NULL);
    if (hash_flag)
        report_time(file, before, after);
    return;
}

/* -------------------------------------------------------------
 * Function: read_from_file()
 * -------------------------------------------------------------
 * Parameters: 
 *  - char *args[]: stores memory address(es) of user's command 
 *                  and any arguments/options given 
 *  - char *file[]: file[0] contains the memory address of the 
 *                  input filename provided by user. 
 *  - hash_flag = 1 if ## was given as last arg, otherwise
 *    hash_flag = 0.
 *
 * Purpose: execute user command in args on file[0]. Saves 
 * start and end time in case hash_flag is 1, so that report_time() 
 * can be called to calulate and print execution time to stdout
 *
 * Based on the Appendix C program provided by Mike Zastre.
 * ------------------------------------------------------------- */
void read_from_file(char *args[], char *file[], int hash_flag) {
    char *envp[] = { 0 };
    int pid, status;
    int input_fd;
    struct timeval before, after;

    gettimeofday(&before,NULL);
    pid = fork();
    if (!(pid)) {
        input_fd = open(file[0], O_RDWR, S_IRUSR|S_IWUSR);
        if (input_fd == -1) {
            fprintf(stderr, "error: filename '%s' does not exist.\n",file[0]);
            exit(1);
         }
        dup2(input_fd,0); // replace stdin with input file
        close(input_fd);
        execve(args[0],args,envp);
        exit(EXIT_FAILURE); // should never get here
    }
    waitpid(pid, &status, 0);
    gettimeofday(&after,NULL);
    if (hash_flag)
        report_time(file,before, after);
    return;
}

/* ---------------------------------------------------------
 * Function: read_and_write()
 * ---------------------------------------------------------
 * Parameters:
 *  - char *args[]: stores memory address(es) of user's command 
 *                  and any arguments/options given 
 *  - char *file[]: file[0] and file[1] contain the memory address 
 *                  of the input and output file (respectively) 
 *                  provided by user. 
 *  - hash_flag = 1 if ## was given as last arg, otherwise
 *    hash_flag = 0.
 *
 * Purpose: execute user command in args on file[0] and 
 * redirect output to file[1]. If hash_flag is 1, then 
 * report_time() is called and the execution time is printed 
 * to file[1].
 * --------------------------------------------------------- */
void read_and_write(char *args[], char *file[], int hash_flag) {
    char *envp[] = { 0 };
    int pid, status;
    int file_in, file_out;
    struct timeval before, after;

    gettimeofday(&before,NULL);
    pid = fork();
    if (!pid) {
        file_in = open(file[0], O_RDONLY, S_IRUSR|S_IWUSR);
        if (file_in == -1) {
            fprintf(stderr, "error: filename '%s' does not exist.\n",file[0]);
            exit(1);
        }
        file_out = open(file[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if (file_out == -1) {
            fprintf(stderr, "cannot open '%s' for writing.\n",file[1]);
            exit(1);
        }
        dup2(file_in, 0); // replace stdin with input file 
        dup2(file_out, 1); //replace stdout with output file 
        close(file_in);
        close(file_out);
        execve(args[0], args, envp);
        exit(EXIT_FAILURE); // should never get here
    }
    waitpid(pid, &status, 0);
    gettimeofday(&after,NULL);
    if (hash_flag)
        report_time(file, before, after);
    return;
}

/* ------------------------------------------------------------
 * Function: stdout_process()
 * ------------------------------------------------------------
 * Parameters:
 *  - char *args[]: stores memory address(es) of user's command 
 *                  and any arguments/options given 
 *  - hash_flag = 1 if ## was given as last arg, otherwise
 *    hash_flag = 0.
 *
 * Purpose: execute user command in args. If hash_flag is 1, 
 * then report_time() is called using the values in before and
 * after and the execution time is printed to stdout.
 * 
 * Based on the Appendix B program provided by Mike Zastre.
 * ------------------------------------------------------------- */
void stdout_process(char *args[], int hash_flag) {
    char *envp[] = { 0 };
    char *file[] = { 0, 0 };
    int pid, status;
    struct timeval before, after;
    
    gettimeofday(&before,NULL);
    pid = fork();
    if (!(pid)) {
        execve(args[0],args,envp);
        exit(EXIT_FAILURE); // should never get here
    }
    /* wait for child */
    waitpid(pid, &status, 0);
    gettimeofday(&after,NULL);
    if (hash_flag)
        report_time(file,before, after);
    return;
}

/* ~~~~~~~~~~~~~~~~ main ~~~~~~~~~~~~~~~~~~~~~ */
int main(int argc, char *argv[]) {
    FILE *fp;
    char prompt[] = "vsh% ";
    char input[MAX_INPUT_LEN] = "";
    char path[MAX_INPUT_LEN] = "";
    int hash_flag;
    int num_args;
    int pid;

    /* make sure .vshrc file exists */
    fp = fopen(".vshrc","r");
    if (!fp) {
        fprintf(stderr, "ERROR: .vshrc file not in directory\n");
        exit(EXIT_FAILURE);
    }

/* ~~~~~~~~~~~~~ main loop ~~~~~~~~~~~~~~~~~~ */
    for (;;) {
        char *args[MAX_NUM_ARGS] = { 0 };
        char *files[] = { 0, 0 };
        memset(input, 0, MAX_INPUT_LEN);
        memset(path, 0, MAX_INPUT_LEN);
        hash_flag = 0;
        num_args = 0;

        /* prompt user for input */
        fprintf(stdout, "%s", prompt);
        fflush(stdout);
        fgets(input, MAX_INPUT_LEN, stdin);

        /* test case: user hits enter */
        if (!(strcmp(input, "\n")))
            continue;

        /* test case: user exits */
        if (input[strlen(input) - 1] == '\n')
            input[strlen(input) - 1] = '\0';
        if (!(strcmp(input,"exit")))
            break;

        /* tokenize input and check for syntax errors */
        num_args = tokenize(input, args, files, &hash_flag);
        if (num_args < 0)
            continue;
        
        /* format filenames */
        get_filenames(files);

        /* make sure valid command given */ 
        if (find_path(path, args[0], fp) < 0)
            continue;
        args[0] = path; // now contains path to command 

        /* check hash syntax/remove from args */
        if (hash_check(args, hash_flag, num_args) < 0)
            continue;

        /* --------- execute command ---------- */
        if (files[1] && !(files[0]))
            write_to_file(args,files, hash_flag);

        else if (files[0] && !(files[1])) 
            read_from_file(args,files,hash_flag);

        else if (files[0] && files[1]) 
            read_and_write(args, files, hash_flag);

        else 
            stdout_process(args, hash_flag);
    }
    fclose(fp);
    return 0;
}
