#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>
#include <time.h>

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

int main(int argc, char* argv[]) {
    const char* COMFILE;
    int WSIZE = 6;
    int mode = 0; // FILE OR USER COMMAND
    const char* mqname = "/kkooeee";

    if (argc == 4) {
        printf("User Mode is activated\n");
        if (argc >= 2) {
            mqname = argv[1];
        } else {
            fprintf(stderr, "Usage: %s <mqname> <size>\n", argv[0]);
            return EXIT_FAILURE;
        }
        WSIZE = atoi(argv[3]);
        mode = 1;
    }
    if (argc == 6) {
        printf("File Mode is active\n");
        if (argc >= 2) {
            mqname = argv[1];
        }
        COMFILE = argv[3];
        WSIZE = atoi(argv[5]);
        mode = 2;
    }

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10; // Maximum number of messages in the queue
    attr.mq_msgsize = 1024; // Maximum message size
    mqd_t mqd;
    mqd = mq_open(mqname, O_CREAT | O_RDWR, 0666, &attr);
    if (mqd == (mqd_t)-1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    printf("Message queue opened successfully!\n");

    const char* pipe1 = "cs_pipe";
    const char* pipe2 = "sc_pipe";
    // Creating Pipes
    int status1, status2;
    status1 = mkfifo(pipe1, 0777);
    if (status1 == -1) {
        perror("Error creating CS_PIPE");
        exit(EXIT_FAILURE);
    }

    status2 = mkfifo(pipe2, 0777);
    if (status2 == -1) {
        perror("Error creating SC_PIPE");
        exit(EXIT_FAILURE);
    }

    int fd = open(pipe1, O_RDWR);
    if (fd == -1) {
        perror("open");
        printf("Failed to open pipe '%s' for writing: %s\n", pipe1, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("CS PIPE opened\n");

    int fd1 = open(pipe2, O_RDWR);
    if (fd1 == -1) {
        perror("open");
        printf("Failed to open pipe '%s' for writing: %s\n", pipe2, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("SC PIPE opened\n");

    int clientID;
    int l;
    int size;

    srand(time(NULL));
    clientID = rand() % 1000000 + 1;
    l = lenHelper(clientID);
    char id[l];
    sprintf(id, "%d", clientID);
    size = lenHelper(WSIZE);
    char ws[size];
    sprintf(ws, "%d", WSIZE);

    strcat(id, " ");
    // PIPES NAMES
    strcat(id, pipe1);
    strcat(id, " ");
    strcat(id, pipe2);
    strcat(id, " ");
    strcat(id, ws);

    printf("FLAG1\n");
    strcat(id, ws);
    printf("%s\n", id);
    // CONREQUEST MESSAGE
    struct Message m;

    // Allocate memory for data member
    m.data = (char*)malloc(strlen(id) + 1); // +1 for null terminator

    // Check if memory allocation was successful
    if (m.data == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1; // Return an error code
    }
    // Copy id to data member
    strcpy(m.data, id);
    // Now you can use m.data as required
    printf("%s\n", m.data);
    printf("%lu\n", (unsigned long)strlen(m.data)); // Use %lu for size_t

    int ret = mq_send(mqd, m.data, strlen(m.data) + 1, 0);

    if (ret == -1) {
        perror("mq_send");
        mq_close(mqd);
        exit(EXIT_FAILURE);
    }

    printf("Sended Message: %s\n", m.data);

    // PIPES

    int nbytes;
    char buffer[1024];

    read(fd1, buffer, 1024);

    int la = strlen(buffer);
    char md[la];
    strcpy(md, buffer);

    if (md[0] == 'a') {
        removeChar(md, md[0]);
        printf("Client '%s' server connection established\n", md);
    }

    while (1) {
        struct Message command;
        command.others[0] = 'b';
        command.others[1] = '\0';
        command.others[2] = '\0';
        command.others[3] = '\0';

        char str[100];
        if (mode == 0) {
            // Prompt the user to enter a string
            printf("Enter a string (type 'quit' to exit): ");
            // Read the string from the user
            fgets(str, sizeof(str), stdin);
            // Remove newline character if present
            str[strcspn(str, "\n")] = '\0';

            if (strcmp(str, "quit") == 0) {
                break; // Exit the loop if the user enters 'quit'
            }
        }

        if (mode == 2) {
            // Code for batch mode
            FILE* file = fopen(COMFILE, "r");
            if (!file) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            // Read a line from the file
            if (fgets(str, sizeof(str), file) == NULL) {
                fprintf(stderr, "Error reading from file\n");
                fclose(file);
                exit(EXIT_FAILURE);
            }

            // Close the file
            fclose(file);

            // Remove newline character if present
            str[strcspn(str, "\n")] = '\0';
        }

        command.data = (char*)malloc(strlen(str) + 1); // Allocate memory for the command data

        if (command.data == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }

        strcpy(command.data, str);

        printf("Command to be sent: %s\n", command.data);

        // Send the command to the server
        ret = mq_send(mqd, command.data, strlen(command.data) + 1, 0);
        if (ret == -1) {
            perror("mq_send");
            mq_close(mqd);
            exit(EXIT_FAILURE);
        }

        printf("Command sent to the server\n");

        // Receive and print the server's response
        ssize_t bytes_received = mq_receive(mqd, buffer, sizeof(buffer), NULL);
        if (bytes_received == -1) {
            perror("mq_receive");
            mq_close(mqd);
            exit(EXIT_FAILURE);
        }

        printf("Server Response: %s\n", buffer);

        free(command.data);
    }

    // Cleanup and close resources
    mq_close(mqd);

    return 0;
}
