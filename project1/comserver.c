#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/types.h>

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>  // Include the header for waitpid

#define MAXARGS 15

struct Message {
    int length;
    char others[4];
    char* data;
};

// Function to remove a character from a string
void removeChar(char *str, char c) {
    int i, j;
    int len = strlen(str);
    for (i = j = 0; i < len; i++) {
        if (str[i] != c) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

// Function to calculate the length of an integer
int lenHelper(int x) {
    if (x >= 1000000) return 7;
    if (x >= 100000) return 6;
    if (x >= 10000) return 5;
    if (x >= 1000) return 4;
    if (x >= 100) return 3;
    if (x >= 10) return 2;
    return 1;
}

int main(int argc, char *argv[]) {
    mqd_t mqd;

    const char* mqname = "/kkooeee";

    if (argc == 2) {
        mqname = argv[1];
    }

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10; // Maximum number of messages in the queue
    attr.mq_msgsize = 1024; // Maximum message size

    mqd = mq_open(mqname, O_CREAT | O_RDWR, 0666, &attr);
    if (mqd == (mqd_t) -1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    printf("Message queue opened successfully!\n");

    char received_msg[2046];
    unsigned int msg_prio;

    ssize_t bytes_received = mq_receive(mqd, received_msg, sizeof(received_msg), &msg_prio );
    if (bytes_received == -1) {
        perror("mq_receive");
        exit(EXIT_FAILURE);
    }

    printf("Received Message: %s\n", received_msg);

    char delim[] = " ";
    char *clientId = strtok(received_msg, delim);
    if (clientId == NULL) {
        printf("Error: No clientID found\n");
        exit(EXIT_FAILURE);
    }
    printf("clientID: %s\n", clientId);
    int cid = atoi(clientId);
    printf("Parsed integer: %d\n", cid);

    char* cs_p = strtok(NULL, delim);
    if (cs_p == NULL) {
        printf("Error: No cs_pipe found\n");
        exit(EXIT_FAILURE);
    }
    printf("cs_pipe: %s\n", cs_p);

    char* sc_p = strtok(NULL, delim);
    if (sc_p == NULL) {
        printf("Error: No sc_pipe found\n");
        exit(EXIT_FAILURE);
    }
    printf("sc_pipe: %s\n", sc_p);

    char* wsize = strtok(NULL, delim);
    if (wsize == NULL) {
        printf("Error: No WSIZE found\n");
        exit(EXIT_FAILURE);
    }
    printf("WSIZE: %s\n", wsize);
    int ws = atoi(wsize);
    printf("Parsed integer: %d\n", ws); 

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        printf("Entering the child process\n");
        int fd = open(cs_p, O_RDWR);
        if (fd == -1) {
            perror("open");
            printf("Failed to open pipe '%s' for writing: %s\n", cs_p, strerror(errno));
            exit(EXIT_FAILURE);
        }

        int fd1 = open(sc_p, O_RDWR);
        if (fd1 == -1) {
            perror("open");
            printf("Failed to open pipe '%s' for writing: %s\n", sc_p, strerror(errno));
            exit(EXIT_FAILURE);
        }

        printf("Writing to the pipe...\n");

        // Connection established message
        struct Message mb;
        mb.others[0] = 'a';
        mb.others[1] = '\0';
        mb.others[2] = '\0';
        mb.others[3] = '\0';

        mb.data = strdup(clientId); // Allocate memory and copy clientId

        if (mb.data == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        strcat(mb.others,mb.data);
        char* message;
        message = (char*)malloc(strlen(mb.others) + 1); // +1 for null terminator

        if (message == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        strcpy(message, mb.others);

        // Write to the pipe
        write(fd1, message, strlen(message));

        char buffer[1024];
        read(fd, buffer, 1024);

        int la = strlen(buffer);
        char md[la];
        strcpy(md, buffer);

        if (md[0] == 'b') {
            removeChar(md, md[0]);
            printf("Command'%s'is taken\n", md);

            int tmpout = open("templateoutput.txt", O_RDWR | O_CREAT, 0777);

            if (tmpout == -1) {
                perror("open");
            }

            if (strchr(md, '|') != NULL) {
                //**************************** TWO COMMANDS *************

                char* first = strtok(md, "|");
                printf("FIRST: %s" , first);
                char* second = strtok(NULL, " ");
                printf("SECOND: %s" , second);

                char* args1[MAXARGS];
                char* token = strtok(first, " "); // Splitting by space character
                args1[0] = token;
                int i = 1;
                while (token != NULL) {
                    token = strtok(NULL, " ");
                    args1[i] = token;
                    i++;
                }
                for(int i = 0; args1[i] != NULL ; i++){
                    printf("ARGS: %s" , args1[i]);
                }

                char* args2[MAXARGS];
                char* token1 = strtok(second, " "); // Splitting by space character
                args2[0] = token1;
                int j = 1;
                while (token1 != NULL) {
                    token1 = strtok(NULL, " ");
                                        args2[j] = token1;
                    j++;
                }
                for(int j = 0; args2[j] != NULL ; j++){
                    printf("ARGS: %s" , args2[j]);
                }

                int fid[2];

                // Create the pipe
                if (pipe(fid) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                pid_t cid1;
                cid1 = fork();

                if (cid1 < 0) {
                    printf ("could not create child\n");
                    exit (1);
                }
                
                if (cid1 == 0) {// FIRST CHILD PROCESS (RUNNER PROCESS FOR THE FIRST COMMAND)
                    // Redirect standard output to the pipe
                    dup2(fid[1], STDOUT_FILENO);
                    close(fid[0]);
                    close(fid[1]);

                    // Execute the first command specified by args1
                    execvp(args1[0], args1);
                    perror("execvp for first command");
                    exit(EXIT_FAILURE);
                }

                // PARENT PROCESS
                pid_t cid2;
                cid2 = fork();

                if (cid2 < 0) {
                    printf ("could not create child\n");
                    exit (1);
                }

                if (cid2 == 0) {// SECOND CHILD PROCESS (RUNNER PROCESS FOR THE SECOND COMMAND)
                    // Redirect standard input from the pipe
                    dup2(fid[0], STDIN_FILENO);
                    close(fid[0]);
                    close(fid[1]);

                    // Redirect standard output to the output file
                    int tmpout = open("templateoutput.txt", O_RDWR | O_CREAT, 0777);
                    if (tmpout == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    dup2(tmpout, STDOUT_FILENO);
                    close(tmpout);

                    // Execute the second command specified by args2
                    execvp(args2[0], args2);
                    perror("execvp for second command");
                    exit(EXIT_FAILURE);
                }

                // PARENT PROCESS
                close(fid[0]);
                close(fid[1]);

                // Wait for the completion of both runner processes
                waitpid(cid1, NULL, 0);
                waitpid(cid2, NULL, 0);

                printf("Both runner processes completed\n");

                // Read the content of the output file
                int tmpout = open("templateoutput.txt", O_RDWR);
                char outputbuffer[1024];
                ssize_t bytes_read = read(tmpout, outputbuffer, sizeof(outputbuffer));
                close(tmpout);

                if (bytes_read == -1) {
                    perror("read from output file");
                    exit(EXIT_FAILURE);
                }

                // Send the output to the client via the sc pipe
                write(fd1, outputbuffer, bytes_read);

                // Delete the output file
                unlink("templateoutput.txt");

                printf("Result sent to the client\n");
            }
            else {
                // ONE COMMAND

                pid_t cpid;
                cpid = fork();
                char* args[MAXARGS];
                char* token = strtok(md, " "); // Splitting by space character
                args[0] = token;
                int i = 1;
                while (token != NULL) {
                    token = strtok(NULL, " ");
                    args[i] = token;
                    i++;
                }
                for(int i = 0; args[i] != NULL ; i++){
                    printf("ARGS: %s" , args[i]);
                }

                if(cpid < 0){
                    printf("Fork failed\n");
                }
                else if(cpid == 0){// CHILD PROCESS 
                    // Redirect standard output and standard error to the file descriptor tmpout
                    int tmpout = open("templateoutput.txt", O_RDWR | O_CREAT, 0777);
                    if (tmpout == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    dup2(tmpout, STDOUT_FILENO);
                    dup2(tmpout, STDERR_FILENO);
                    close(tmpout);

                    // Execute the command specified by args
                    execvp(args[0], args);
                    perror("execvp for single command");
                    exit(EXIT_FAILURE);
                }
                else { // PARENT PROCESS
                    // Wait for the completion of the runner process
                    waitpid(cpid, NULL, 0);

                    printf("Runner process completed\n");

                    // Read the content of the output file
                    int tmpout = open("templateoutput.txt", O_RDWR);
                    char outputbuffer[1024];
                    ssize_t bytes_read = read(tmpout, outputbuffer, sizeof(outputbuffer));
                    close(tmpout);

                    if (bytes_read == -1) {
                        perror("read from output file");
                        exit(EXIT_FAILURE);
                    }

                    // Send the output to the client via the sc pipe
                    write(fd1, outputbuffer, bytes_read);

                    // Delete the output file
                    unlink("templateoutput.txt");

                    printf("Result sent to the client\n");
                }
            }
        }
        close(fd1);
    }

    mq_close(mqd);

    return 0;
}

