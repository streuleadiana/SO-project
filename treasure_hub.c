#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

// File name for storing treasure data
#define TREASURE_FILE "treasures.bin"

// Struct from treasure_manager.c
typedef struct 
{
    int id;
    char name[32];
    float latitude; 
    float longitude;
    char clue[1028];
    int value;
} Treasure;

// Function from treasure_manager.c
void write_msg(const char *msg) 
{
    write(1, msg, strlen(msg));
}

// Function from treasure_manager.c
void list(const char *hunt_id) 
{
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;
    if (stat(file_path, &st) == -1) 
    {
        write_msg("Wrong id.\n");
        exit(-1);
    }

    char msg[2048];
    snprintf(msg, sizeof(msg), "Hunt: %s\nSize: %ld bytes\n", hunt_id, st.st_size);
    write_msg(msg);

    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "Last modify: %Y-%m-%d %H:%M:%S\n", localtime(&st.st_mtime));
    write_msg(timebuf);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Error at file opening\n");
        exit(-1);
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        snprintf(msg, sizeof(msg), "ID: %d\nName: %s\nLat: %.2f Long: %.2f\nClue: %s\nValue: %d\n\n", t.id, t.name, t.latitude, t.longitude, t.clue, t.value);
        write_msg(msg);
    }
    close(fd);
}

// Function from treasure_manager.c
void view(const char *hunt_id, const char *id) 
{
    int search_id = atoi(id);
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Error at file opening\n");
        exit(-1);
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        if (t.id == search_id) 
        {
            char msg[2048];
            snprintf(msg, sizeof(msg), "ID: %d\nName: %s\nLat: %.2f Long: %.2f\nClue: %s\nValue: %d\n", t.id, t.name, t.latitude, t.longitude, t.clue, t.value);
            write_msg(msg);
            break;
        }
    }
    close(fd);
}

// Global variables
pid_t monitor_pid = 0; // PID of the monitor process
int monitor_stopping = 0; // Flag to indicate if monitor is stopping
volatile sig_atomic_t command_processed = 0; // Flag for synchronizing command processing
int pipe_fd[2]; // File descriptors for pipe: [0] for reading, [1] for writing

// Handles SIGCHLD signal when the monitor process terminates
void handle_sigchld(int sig) 
{
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid == monitor_pid) 
    {
        write_msg("The monitor ended with status: ");
        char status_msg[16];
        snprintf(status_msg, sizeof(status_msg), "%d\n", WEXITSTATUS(status));
        write_msg(status_msg);
        monitor_pid = 0;
        monitor_stopping = 0;
    }
}

// Handles SIGUSR2 in the main process to signal command completion
void handle_sigusr2_main(int sig) 
{
    command_processed = 1; // Monitor has finished processing the command
}

// Handles SIGUSR1 in the monitor process to process commands
void handle_sigusr1(int sig) 
{
    // Read the command from the pipe
    char buffer[256] = {0};
    int bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) 
    {
        write_msg("Error reading command from pipe\n");
        kill(getppid(), SIGUSR2); // Notify main process
        return;
    }
    buffer[bytes_read] = '\0';

    // Parse and execute the command
    char command[256];
    char *token = strtok(buffer, " ");
    if (token) 
    {
        strcpy(command, token);
        if (strcmp(command, "list_hunts") == 0) 
        {
            write_msg("Hunts list:\n");
            DIR *dir = opendir(".");
            if (dir == NULL) 
            {
                write_msg("Error opening current directory\n");
                kill(getppid(), SIGUSR2); // Notify main process
                return;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) 
            {
                if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
                {
                    char file_path[512];
                    snprintf(file_path, sizeof(file_path), "%s/%s", entry->d_name, TREASURE_FILE);

                    int fd = open(file_path, O_RDONLY);
                    int treasure_count = 0;
                    if (fd != -1) 
                    {
                        Treasure t;
                        while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) 
                        {
                            treasure_count++;
                        }
                        close(fd);
                    }

                    char msg[512];
                    snprintf(msg, sizeof(msg), "%s: %d treasures\n", entry->d_name, treasure_count);
                    write_msg(msg);
                }
            }
            closedir(dir);
        } else if (strcmp(command, "list_treasures") == 0) 
        {
            token = strtok(NULL, " ");
            if (token) 
            {
                list(token);
            }
        } else if (strcmp(command, "view_treasure") == 0) 
        {
            token = strtok(NULL, " ");
            char *hunt_id = token;
            token = strtok(NULL, " ");
            if (hunt_id && token) 
            {
                view(hunt_id, token);
            }
        }
    }
    kill(getppid(), SIGUSR2); // Notify main process that processing is complete
}

// Handles SIGUSR2 in the monitor process to terminate it
void handle_sigusr2_monitor(int sig) 
{
    write_msg("The monitor is shutting down...\n");
    usleep(2000000); // Delay to simulate shutdown
    close(pipe_fd[0]); // Close pipe read end in monitor
    exit(0);
}

// Sets up signal handlers for the monitor process
void setup_signals() 
{
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_sigusr2_monitor;
    sigaction(SIGUSR2, &sa, NULL);
}

// Sends a command to the monitor via the pipe and SIGUSR1
void send_command(const char *command) 
{
    // Write command to pipe
    if (write(pipe_fd[1], command, strlen(command)) == -1) 
    {
        write_msg("Error writing command to pipe\n");
        return;
    }

    // Signal monitor to process the command
    if (monitor_pid > 0) 
    {
        command_processed = 0; // Reset flag
        kill(monitor_pid, SIGUSR1);
    }
}

// Main function
int main() 
{
    // Create pipe before forking
    if (pipe(pipe_fd) == -1) 
    {
        write_msg("Error creating pipe\n");
        return -1;
    }

    // Set up signal handlers for main process
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    sa.sa_handler = handle_sigusr2_main;
    sigaction(SIGUSR2, &sa, NULL);

    char input[256];
    write_msg("> "); // Display initial prompt
    while (fgets(input, sizeof(input), stdin)) 
    {
        input[strcspn(input, "\n")] = '\0'; // Remove newline

        if (monitor_stopping) 
        {
            write_msg("Error: Monitor is closing, please wait!\n");
            write_msg("> ");
            continue;
        }

        if (strcmp(input, "start_monitor") == 0) 
        {
            if (monitor_pid > 0) 
            {
                write_msg("The monitor is already running!\n");
            } 
            else 
            {
                monitor_pid = fork();
                if (monitor_pid == 0) 
                {
                    close(pipe_fd[1]); // Close write end in monitor
                    setup_signals();
                    while (1) 
                    {
                        pause(); // Wait for signals
                    }
                } 
                else if (monitor_pid > 0) 
                {
                    close(pipe_fd[0]); // Close read end in main process
                    write_msg("The monitor turned on.\n");
                } 
                else 
                {
                    write_msg("Error turning on the monitor\n");
                }
            }
        } else if (strcmp(input, "list_hunts") == 0) 
        {
            if (monitor_pid == 0) 
            {
                write_msg("Error: Monitor is not running!\n");
            } else 
            {
                send_command("list_hunts");
                while (!command_processed) 
                {
                    pause(); // Wait for monitor to finish
                }
            }
        } else if (strncmp(input, "list_treasures ", 15) == 0) 
        {
            if (monitor_pid == 0) 
            {
                write_msg("Error: Monitor is not running!\n");
            } else 
            {
                send_command(input);
                while (!command_processed) 
                {
                    pause(); // Wait for monitor to finish
                }
            }
        } else if (strncmp(input, "view_treasure ", 14) == 0) 
        {
            if (monitor_pid == 0) 
            {
                write_msg("Error: Monitor is not running!\n");
            } else 
            {
                send_command(input);
                while (!command_processed) 
                {
                    pause(); // Wait for monitor to finish
                }
            }
        } else if (strcmp(input, "stop_monitor") == 0) 
        {
            if (monitor_pid == 0) 
            {
                write_msg("Error: Monitor is not running!\n");
            } else 
            {
                monitor_stopping = 1;
                kill(monitor_pid, SIGUSR2);
                write_msg("The monitor has been turned off, please wait for completion...\n");
            }
        } else if (strcmp(input, "exit") == 0) 
        {
            if (monitor_pid > 0) 
            {
                write_msg("Error: Monitor is still running! Stop it first.\n");
            } else 
            {
                close(pipe_fd[1]); // Close write end in main process
                write_msg("The program closes\n");
                break;
            }
        } else 
        {
            write_msg("Invalid command\n");
        }
        write_msg("> "); // Display prompt for next command
    }

    return 0;
}