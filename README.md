# CSC 360, Summer 2021
## Assignment #1 README NOTE: NOT RELATED TO CSH

* Name: `Nasseri, Gina`
* VNum: `V00799673`
* Netlink: `gnasseri`

---

This README file describes the methods used to implement each feature in assignment 1.

---
### Preliminaries

__Variables initialized in main:__

* `FILE *fp`: to be used with the .vshrc file
* `char prompt[] = "vsh% "`: prompt printed at each iteration
* `char input[MAX_INPUT_LEN]`: used as a buffer to get user input
* `char path[MAX_INPUT_LEN]`: used as a buffer to get command path 
* `int hash_flag`: initialized as 0 and set to 1 if "##" is encountered as an argument
* `int num_args`: keeps track of the number of arguments given with the user's command

    After initializing these variables, `vsh` checks to see that the `.vshrc` file is in the current working directory. If the file is not located, then an error message is printed and vsh will terminate. 

__Variables initialized/reset at the start of each iteration in the main loop:__
* `char *args[MAX_NUM_ARGS] = { 0 }`: to be populated by (the memory addresses of) the individual words (which are not filenames) that make up the user's input, where args[0] is for the user's command and the remaining indices are for any accompanying arguments or options up to a maximum of 9 args -- i.e., the maximum number of entries in args including the user's command will be 10.
* `char *files[] = { 0, 0 }`: files[0] will store the memory address of an input filename and files[1] will store the memory address of an output filename if one or both are provided by the user. 
* `input`,`path` are both cleared 
* `hash_flag`, `num_args` are reset to 0.

__Processing user input:__

The shell repeatedly prompts the user for a command to execute by printing the prompt `vsh% ` at the start of each iteration.  The user's input is scanned into the char array `input`. If the user hits enter, the next iteration begins (via `continue`) so that the prompt will reappear, otherwise the input string is null-terminated (`\n` char at end of `input` is replaced with `\0`). If the user's input is `exit`, then the loop exits (via `break`), and the program is terminated. Each case is tested for using `strcmp()`. 

If neither of the above cases are encountered, then the user's input is split up into individual tokens by calling the `tokenize()` function. This function takes the user's input stored in `input` and assigns `char *t = strtok(input," ")` so that `t` is at the memory address of the first word in the user's input. This word is assigned to `args[0]` and any remaining words (which are not filenames) are assigned to `args[num_args]` using successive calls of `t = strtok(NULL, " ")` until no more tokens exist. 
The following cases are tested for while populating `args`: 
* If `num_args` exceeds 10, then the user has entered more than 9 arguments and/or options with their given command, causing the function to print a `max args exceeded` error and return `-1`. 
* If the current token begins (output file) or ends (input file) with `::`, then that token is stored into `file[1]` or `file[0]`, respectively. Filenames in this format are **not** included in the argument count (`num_args` will not be incremented). 
* If a token = `::` then the function will print a `missing filename` error and return `-1`.
* If `##` is encountered as a token then `hash_flag` is set to 1 to indicate that the time of execution has been requested with this command. 
* If `num_args` is 0 and this function has been called, then a filename has been provided and no command has been given and the function will print a `missing command` error and return `-1`.  
If `tokenize()` returns a negative value, then some error occurred--which will be printed--and the next iteration will begin (prompt will be printed). Otherwise, `tokenize()` will return the number of arguments (length of `args` provided by the user) and `args` will be populated with the (memory addresses of the) user's command and any arguments or options provided with the command. As well, if filenames were provided, then `files` will be populated with the memory address(es) of the filenames(s) in the appropriate index.   

Following this, `get_filenames()` is called with the `files` array where: if `file[i]` where `i = 0,1` exists, then `char *j = strtok(file[i], "::")` is assigned so that `j` is at the memory address of the filename separated from the `::` and `file[i]` is reassigned with `j` (recall, `file[0]` -> input filename, `file[1]` -> output filename).

---

### Implementing the Four Features

__Part b:__ Execute simple commands with at most nine arguments. 

If this part in the code is reached, then the requirement that the command may only have at most 9 arguments has already been ensured. 

Firstly, to execute a command, the program must ensure that a path to the command exists as a line in the `.vshrc` file. To do this, the function `find_path()` is called with the buffer `path`, `args[0]` (the user's command which we are searching for), and `fp` which is the file pointer for the `.vshrc` file.

`find_path()` ensures that `fp` is at the first line in `.vshrc` using `rewind()` and then scans the file line by line to test if a path to the command (in `args[0]`) exists as a line in the file. A complementary function `complete_path()` is called at each iteration, which conjoins a line in the `.vshrc` file with `/` and the user's command (e.g., if the user's command is `ls`, and the line in `.vshrc` is `/usr/bin`, then `complete_path()` will convert `path` from `/usr/bin` to `/usr/bin/ls`) so that `access()` can be called with `path` to check if the current line in `.vshrc` is a valid path to the command file.  If a path is found, then the function returns `0` and the correct path will now be in `path` upon returning. Otherwise, `find_path()` prints an error message and returns `-1`.

If `find_path()` returns `-1`, then an error will be printed and the user will be prompted for another command. Otherwise, `path` now contains path of user's command which was stored in `args[0]`, and `args[0]` is reassigned to `path`. Any options or arguments given with the user's command are still contained in the rest of the array unmodified. 

Since `args[0]` now contains a valid path to the user's command, `args` can now be used in `execve()` to execute the command.  To execute command to `stdout`, `stdout_process()` is called with `args`.  In this function, `fork()` is called and then `execve(args[0], args, envp)` is called to replace its inherited text and data sections with new binary in `args[0]` where `args` contains the options and arguments for `args[0]`, and `envp` which is a `NULL` char * array since the `vsh` shell is already doing the work of the environment. Since this function is only called if a valid command is given, the command in `args[0]` will be executed.  

Note: `stdout_process()` is called only if `file[0]` and `file[1]` are null. 

__Part c:__ If commmand includes an argument with `::` followed immediately by a filename, then the command output is stored in that file.

Any token which starts with `::` will have already been saved in `file[1]` in the `tokenize()` function. If we have got to this point in the code, then we know that `file[1]` contains the filename of the file to write to and that `args[0]` contains a valid path to the user's command. The function `write_to_file()` can now be called with `args` and `files` so that the command and arguments in `args` can be executed and the result output to `file[1]`. 

The process works similarly to the `stdout_process()`, however, after calling `fork()`, the output file in `file[1]` is opened by assigning `int output_fd` to the opened file, and `dup2(output_fd,1)` is called which replaces the `stdout` stream with the `output_fd` stream so that the output of the command will be sent to `file[1]` rather than `stdout`.
Note that the parameters used with `open()` ensure that if the file does not exist, then it is created (`O_CREAT`), and that the contents of the file will be written over entirely anytime output is sent to the file (`O_TRUNC`).

Note: `write_to_file()` is only called if `file[1]` exists and `file[0]` is null.

__Part d:__ If commmand includes an argument having a filename immediately followed by `::`, then the command input is to be retrieved from that file.

Similarly to part c, any token which ends with `::` will have already been saved in `file[0]` in the `tokenize()` function. If we have got to this point in the code, then we know that `file[0]` contains the filename of the file to read from and that `args[0]` contains a valid path to the user's command. The function `read_from_file()` can now be called with `args` and `files` so that the command and arguments in `args` can be executed on the contents of `file[0]`.

This process is similar to that accomplished by `write_to_file()`, but in this case, `O_CREAT` is not used as a parameter in `open()`, so if the filename in `file[0]` does not exist, then an error message is printed and the process exits with status `EXIT_FAILURE` (the function will return to main and the prompt will be printed).  Provided the file exists in the  current working directory, then `dup2(input_fd, 0)` is called which replaces the `stdin` stream with `input_fd` (the fd for `file[0]`) so that when `execve()` is called with `args`, the command is executed on the contents of the `file[0]` file. 

Note: `read_fom_file()` is only called if `file[0]` exists and `file[1]` is null.

__Part c and d combined__:

If both an input file and output file were provided (`file[0]` and `file[1]` are not null), then the function `read_and_write()` is called with `args` and `files` and combines the methods of part c and part d. `file[1]` and `file[0]` are each opened as they were in parts c and d (respectively), and then `dup2(file_in,0)` is called to replace the `stdin` stream with `file[0]` and then `dup2(file_out,1)` is called to replace the `stdout` stream with `file[1]`. When `execve()` is called with args, then the command in `args` is executed on the contents in `file[0]` and the results are output to `file[1]`.

__Part e and f:__ If the command ends with `##`, then the time taken to execute the command will be printed out after the command is completed by `vsh`.

In the function `tokenize()`, if `##` is encountered as an argument, then `hash_flag` is switched from `0` to `1`.  The function `hash_check()` (called after a successful return from `find_path()`) checks if `hash_flag` is `1`, and if it is-- it ensures that `##` was the last argument given by the user (otherwise an error is printed and the user will be prompted for another command) and then removes `##` from the last position in `args`. 

Each function `stdout_process()`, `write_to_file()`, `read_from_file()`, `read_and_write()` has the parameter `hash_flag` passed into it. In each function, just before `fork()` is called, the time is recorded in `struct timeval before`, and upon returning from `waitpid()`, the time is recorded into `struct timeval after`. If `hash_flag` is set to 1, then in each function, `report_time()` is called with `before` and `after` and `files`. The function calculates the time of execution (using the formula provided in Appendix F by Mike Zastre) and  then does 1 of two things:

If a file exists in `file[1]` (an output file), then the file is opened with `fopen()` and the execution time is added to the end of the file using `fprintf()`.

Otherwise, the time of execution is printed to `stdout`. 
