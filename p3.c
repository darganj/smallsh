#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

//Foreground flag tracker
int fgFlag = 1;
int skip = 1;


//Function to get status of last command in foreground. 
//Prints either exit status of terminating signal of last foreground command.

void currStatus(int status){
    //int exitStatus = -2; Removed for testing
    if (!WIFEXITED(status)){
        printf("terminated by signal %d\n", status);
        fflush(stdout);
    }
    else {
        //exitStatus = WEXITSTATUS(status); removed for testing
        printf("exit value %d\n", WEXITSTATUS(status)); //exitStatus); Removed for testing
        fflush(stdout);
    }
}

//Function for ctrl-c command to send SIGINT signal. 
//Uses foreground tracker to determine if process is in foreground or not and returns ctrl-c function.

void catchSIGINT(int signo){
    if(fgFlag == 1){
        printf("\n");
        fflush(stdout);
        skip = 0;
    }
    else{
        printf("\n");
        fflush(stdout);
        skip = 0;
    }
}

// Function to catch ctrl-z function command.
// Toggles between standard and foreground mode based on foreground flag.

void catchSIGSTP(int signo){
    if(fgFlag == 1){ //foreground only mode
        fgFlag = 0;
        write(1, "\nEntering foreground-only mode (&is now ignored)\n", 51); //print message indicating foreground only mode.
        skip = 0;
    }
    else{ //To 'standard' mode. 
        fgFlag = 1;
        write(1, "\nExiting foreground-only mode.\n", 31); //print message indicating return to standard mode. 
        skip = 0;
    }
}

// Change Directory function. Array limit of 2048 per project requirements.
// User inputs a directory to change to. If none specified, user is taken to home dir. 

void changeDirectory(char* commandArr[2048], int count){
    if (count == 2){
        chdir(commandArr[1]);
    }
    else {
        chdir(getenv("HOME"));
    }
}

//Function stringPID takes $$ and returns PID.
//User enters $$ string and the pid is returned, per program requirement.

char* stringPID(int PID, const char* inputArg, const char* orig){
    
    int indCount = 0;
    int count = 0;
    int i;
    int pidLength;
    int origLen;
    char pid[100];
    char *replacePID;

    sprintf(pid, "%d", PID);

    pidLength = strlen(pid);
    origLen = strlen(orig);

    for (i = 0; inputArg[i] != '\0'; i++){  // For loop for iterating through user input and looks for $$
        if(strstr(&inputArg[i], orig) == &inputArg[i]){
            count++;
            i += origLen - 1;
        }
    }
    replacePID = (char*)malloc(i + count * (pidLength - origLen) + 1);

    while(*inputArg){   
        if(strstr(inputArg, orig) == inputArg){
            strcpy(&replacePID[indCount], pid); //adds pid to string
            indCount += pidLength;
            inputArg += origLen;
        }
        else{
            replacePID[indCount++] = *inputArg++; //continue iterating through string
        }
    }
    replacePID[indCount] = '\0';
    return replacePID; //return pid string.
}


//smShell function runs the main shell functions
//takes user input and determines which function to run, such as cd, exit, and/or status.

void smShell(char* user_input){
    int valid = 1;
    int val_input, val_output, indCount, currPID;
    int status, i, process_tracker = 0;
    char* parsed_string;
    char* command_line[2048];
    char clean_string[512];
    char comment_check[100];
    char* pid_check;
    pid_t spawnPID = -2;
    struct sigaction SIGINT_action = {0};
    struct sigaction SIGSTP_action = {0};

    SIGINT_action.sa_handler = catchSIGINT;
    sigfillset(&(SIGINT_action.sa_mask));
    SIGINT_action.sa_flags = 0;

    SIGSTP_action.sa_handler = catchSIGSTP;
    sigfillset(&(SIGSTP_action.sa_mask));
    SIGSTP_action.sa_flags = 0;

    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGSTP_action, NULL);

    while(valid == 1){ //Loops until user exits shell via exit command. 
        printf(": "); //Prints out shell command prompt.
        fflush(stdout);
        fgets(user_input, 512, stdin); // takes user input
        parsed_string = strtok(user_input, " ");

        indCount = 0;
        char input_file[10000] = {0};
        char output_file[10000] = {0};

        while (parsed_string != NULL){ //Begin looping through parsed user input string.
            if(strcmp(parsed_string, "<") == 0){ //checks if input < is entered. if so, stores input_file name.
                parsed_string = strtok(NULL, " ");
                sscanf(parsed_string, "%s", input_file);
                parsed_string = strtok(NULL, " ");
            }
            else if(strcmp(parsed_string, ">") == 0){ //checks if input > is entered. If so, stores output_file name.
                parsed_string = strtok(NULL, " ");
                sscanf(parsed_string, "%s", output_file);
                parsed_string = strtok(NULL, " ");
            }
            else if(strstr(parsed_string, "$$") != NULL){  //stores file name skipping $$. Then calls stringPID function.
                sscanf(parsed_string, "%s", clean_string);
                command_line[indCount++] = strdup(stringPID(getpid(), clean_string, "$$"));
                parsed_string = strtok(NULL, " ");
            }
            else {
                sscanf(parsed_string, "%s", clean_string); //add input to array for use. 
                command_line[indCount] = strdup(clean_string);
                parsed_string = strtok(NULL, " ");
                indCount++;
            }
        }

        strcpy(comment_check, (command_line[0])); //copy first user arg to a variable for use.

        if (user_input[0] == '\n'){
            command_line[0] = strdup("emptyComment"); // check if user entered empty comment, per program requirements.
        }

        if (strcmp(command_line[indCount - 1], "&") == 0){ // check if user entered & character.
            command_line[indCount - 1] = NULL;
            process_tracker = 1;
        }
        else {
            command_line[indCount] = NULL;
            process_tracker = 0;
        }

        if(strcmp(command_line[0], "exit") == 0){ // check if user entered exit command.
            valid = 0;
            exit(0);
        }
        else if(strcmp(command_line[0], "cd") == 0){ // check if user entered cd command.
            changeDirectory(command_line, indCount);
        }
        else if(strcmp(command_line[0], "status") == 0){ // check if user entered status command.
            currStatus(status);
        }
        else if(strcmp(command_line[0], "#") == 0 || strcmp(command_line[0], "emptyComment") == 0 || comment_check[0] == '#'){
            //Nothing. It's just a comment. Carry on.
        }
        else if(strcmp(command_line[0], "echo") == 0 && command_line[1] != NULL && strcmp(command_line[1], "$$") == 0){
            printf("%d\n", getpid());
        }
        else if(skip == 0){
            skip = 1;
        }
        else{
            spawnPID = fork(); //execute command by forking a child process.

            switch (spawnPID)
            {
            case -1:
                {
                    perror("Something went terribly wrong.\n");
                    fflush(stdout);
                    status = 1;
                    break;
                }
            case 0:
            {
                if(process_tracker == 0 || fgFlag == 0){ // if in foreground mode, reset SIGINT
                    SIGINT_action.sa_handler = SIG_DFL;
                    sigaction(SIGINT, &SIGINT_action, NULL);
                }

                if(input_file[0] != 0){ // validate input file for functions such as wc in testScript.
                    val_input = open(input_file, O_RDONLY);
                    if(val_input == -1){
                        printf("cannot open %s for input\n", input_file); // if file cannot be opened, print error message w/ file name.
                        fflush(stdout);
                        _exit(1);
                    }
                    if(dup2(val_input, 0) == -1){
                        perror("Error with valid_input\n");
                        fflush(stdout);
                        _exit(1);
                    }
                    close(val_input); // close current file.
                }

                if(output_file[0] != 0){ //similar to input_file validation, validate output_file 
                    val_output = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0744); // open w/ read, create, truncate permissions.
                    if(val_output == -1){
                        printf("cannot open %s for output\n", output_file); // if file cannot be opened, print error message w/ file name.
                        fflush(stdout);
                        _exit(1);
                    }
                    if(dup2(val_output, 1) == -1){
                        perror("Error with valid_output\n");
                        fflush(stdout);
                        _exit(1);
                    }
                    close(val_output); //close current file
                }

                if(execvp(command_line[0], command_line) < 0){ // execvp used for other functions and detmerine if file or dir is found. Prints message if not found.
                    printf("%s: no such file or directory\n", command_line[0]);
                    fflush(stdout);
                    _exit(1);
                }
                break;
            }
            
            default:
                if(process_tracker == 0 || fgFlag == 0){ // checks foreground process flag
                    waitpid(spawnPID, &status, 0); // Parent process will wait on child process.
                }
                else {
                    printf("background pid is %d\n", spawnPID); // return background PID
                    fflush(stdout);
                    usleep(200000);
                }
                break;
            }
        }
    }
    usleep(200000);
    spawnPID = waitpid(-1, &status, WNOHANG);

    while(spawnPID > 0){
        printf("background pid %d is done: ", spawnPID); // return background PID for child and exit status.
        fflush(stdout);
        currStatus(status);
        spawnPID = waitpid(-1, &status, WNOHANG);
    }
 }

// Main function takes user input and runs primary program function, smShell

int main(int argc, char const *argv[]){
    char user_input[2048]; // user input max 2048 characters, per program requirement.
    smShell(user_input);
    return 0;
}
