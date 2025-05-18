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

// Global variables
pid_t monitor_pid = 0; // PID of the monitor process
int monitor_stopping = 0; // Flag to indicate if monitor is stopping
volatile sig_atomic_t command_processed = 0; // Flag for synchronizing command processing
int cmd_pipe_fd[2]; // Pipe for sending commands to monitor: [0] read, [1] write
int res_pipe_fd[2]; // Pipe for receiving results from monitor: [0] read, [1] write

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
    char buffer[256] = {0};
    int bytes_read = read(cmd_pipe_fd[0], buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) 
    {
        write(res_pipe_fd[1], "Error reading command from pipe\n", 31);
        kill(getppid(), SIGUSR2);
        return;
    }
    buffer[bytes_read] = '\0';

    char result[4096] = {0}; // Buffer for results
    int result_len = 0;

    char command[256];
    char *token = strtok(buffer, " ");
    if (!token) 
    {
        result_len = snprintf(result, sizeof(result), "Invalid command\n");
        write(res_pipe_fd[1], result, result_len);
        kill(getppid(), SIGUSR2);
        return;
    }
    strcpy(command, token);

    if (strcmp(command, "list_hunts") == 0) 
    {
        result_len = snprintf(result, sizeof(result), "Hunts list:\n");
        DIR *dir = opendir(".");
        if (dir == NULL) 
        {
            result_len = snprintf(result, sizeof(result), "Error opening current directory\n");
            write(res_pipe_fd[1], result, result_len);
            kill(getppid(), SIGUSR2);
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
                int msg_len = snprintf(msg, sizeof(msg), "%s: %d treasures\n", entry->d_name, treasure_count);
                strncat(result, msg, sizeof(result) - result_len - 1);
                result_len += msg_len;
            }
        }
        closedir(dir);
    } 
    else if (strcmp(command, "list_treasures") == 0) 
    {
        token = strtok(NULL, " ");
        if (!token) 
        {
            result_len = snprintf(result, sizeof(result), "Error: Hunt ID missing\n");
            write(res_pipe_fd[1], result, result_len);
            kill(getppid(), SIGUSR2);
            return;
        }

        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", token, TREASURE_FILE);

        struct stat st;
        if (stat(file_path, &st) == -1) 
        {
            result_len = snprintf(result, sizeof(result), "Wrong id.\n");
            write(res_pipe_fd[1], result, result_len);
            kill(getppid(), SIGUSR2);
            return;
        }

        result_len = snprintf(result, sizeof(result), "Hunt: %s\nSize: %ld bytes\n", token, st.st_size);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "Last modify: %Y-%m-%d %H:%M:%S\n", localtime(&st.st_mtime));
        strncat(result, timebuf, sizeof(result) - result_len - 1);
        result_len += strlen(timebuf);

        int fd = open(file_path, O_RDONLY);
        if (fd == -1) 
        {
            result_len = snprintf(result, sizeof(result), "Error at file opening\n");
            write(res_pipe_fd[1], result, result_len);
            kill(getppid(), SIGUSR2);
            return;
        }

        Treasure t;
        while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) 
        {
            char msg[2048];
            int msg_len = snprintf(msg, sizeof(msg), "ID: %d\nName: %s\nLat: %.2f Long: %.2f\nClue: %s\nValue: %d\n\n", 
                                   t.id, t.name, t.latitude, t.longitude, t.clue, t.value);
            strncat(result, msg, sizeof(result) - result_len - 1);
            result_len += msg_len;
        }
        close(fd);
    } 
    else if (strcmp(command, "view_treasure") == 0) 
    {
        token = strtok(NULL, " ");
        char *hunt_id = token;
        token = strtok(NULL, " ");
        if (!hunt_id || !token) 
        {
            result_len = snprintf(result, sizeof(result), "Error: Hunt ID or treasure ID missing\n");
            write(res_pipe_fd[1], result, result_len);
            kill(getppid(), SIGUSR2);
            return;
        }

        int search_id = atoi(token);
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);

        int fd = open(file_path, O_RDONLY);
        if (fd == -1) 
        {
            result_len = snprintf(result, sizeof(result), "Error at file opening\n");
            write(res_pipe_fd[1], result, result_len);
            kill(getppid(), SIGUSR2);
            return;
        }

        Treasure t;
        int found = 0;
        while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) 
        {
            if (t.id == search_id) 
            {
                result_len = snprintf(result, sizeof(result), 
                                      "ID: %d\nName: %s\nLat: %.2f Long: %.2f\nClue: %s\nValue: %d\n", 
                                      t.id, t.name, t.latitude, t.longitude, t.clue, t.value);
                found = 1;
                break;
            }
        }
        close(fd);
        if (!found) 
        {
            result_len = snprintf(result, sizeof(result), "Treasure with ID %d not found\n", search_id);
        }
    }

    write(res_pipe_fd[1], result, result_len);
    kill(getppid(), SIGUSR2); // Notify main process
}

// Handles SIGUSR2 in the monitor process to terminate it
void handle_sigusr2_monitor(int sig) 
{
    char msg[] = "The monitor is shutting down...\n";
    write(res_pipe_fd[1], msg, strlen(msg));
    usleep(2000000); // Delay to simulate shutdown
    close(cmd_pipe_fd[0]);
    close(res_pipe_fd[1]);
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

// Sends a command to the monitor via the command pipe and reads the result
void send_command(const char *command) 
{
    if (write(cmd_pipe_fd[1], command, strlen(command)) == -1) 
    {
        write_msg("Error writing command to pipe\n");
        return;
    }

    if (monitor_pid > 0) 
    {
        command_processed = 0;
        kill(monitor_pid, SIGUSR1);
        while (!command_processed) 
        {
            pause(); // Wait for monitor to finish
        }

        // Read result from result pipe
        char buffer[4096] = {0};
        int bytes_read = read(res_pipe_fd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) 
        {
            buffer[bytes_read] = '\0';
            write_msg(buffer);
        }
    }
}

// Executes the external score calculator for a hunt and reads its output
void calculate_score_for_hunt(const char *hunt_id) 
{
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) 
    {
        write_msg("Error creating pipe for score calculation\n");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) 
    {
        // Child process
        close(pipe_fd[0]); // Close read end
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipe_fd[1]);

        execl("./calculate_score", "calculate_score", hunt_id, NULL);
        write_msg("Error executing calculate_score\n");
        exit(-1);
    } 
    else if (pid > 0) 
    {
        // Parent process
        close(pipe_fd[1]); // Close write end
        char buffer[4096];
        int bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) 
        {
            buffer[bytes_read] = '\0';
            char msg[5000];
            snprintf(msg, sizeof(msg), "Hunt %s:\n%s", hunt_id, buffer);
            write_msg(msg);
        }
        close(pipe_fd[0]);
        waitpid(pid, NULL, 0); // Wait for child to finish
    } 
    else 
    {
        write_msg("Error forking for score calculation\n");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }
}

// Implements the calculate_score command
void calculate_score() 
{
    DIR *dir = opendir(".");
    if (dir == NULL) 
    {
        write_msg("Error opening current directory\n");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
        {
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%s", entry->d_name, TREASURE_FILE);
            if (access(file_path, F_OK) == 0) // Check if treasures.bin exists
            {
                calculate_score_for_hunt(entry->d_name);
            }
        }
    }
    closedir(dir);
}

// Main function
int main() 
{
    // Create pipes before forking
    if (pipe(cmd_pipe_fd) == -1 || pipe(res_pipe_fd) == -1) 
    {
        write_msg("Error creating pipes\n");
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
                    close(cmd_pipe_fd[1]); // Close write end of command pipe
                    close(res_pipe_fd[0]); // Close read end of result pipe
                    setup_signals();
                    while (1) 
                    {
                        pause(); // Wait for signals
                    }
                } 
                else if (monitor_pid > 0) 
                {
                    close(cmd_pipe_fd[0]); // Close read end of command pipe
                    close(res_pipe_fd[1]); // Close write end of result pipe
                    write_msg("The monitor turned on.\n");
                } 
                else 
                {
                    write_msg("Error turning on the monitor\n");
                }
            }
        } 
        else if (strcmp(input, "list_hunts") == 0) 
        {
            if (monitor_pid == 0) 
            {
                write_msg("Error: Monitor is not running!\n");
            } 
            else 
            {
                send_command("list_hunts");
            }
        } 
        else if (strncmp(input, "list_treasures ", 15) == 0) 
        {
            if (monitor_pid == 0) 
            {
                write_msg("Error: Monitor is not running!\n");
            } 
            else 
            {
                send_command(input);
            }
        } 
        else if (strncmp(input, "view_treasure ", 14) == 0) 
        {
            if (monitor_pid == 0) 
            {
                write_msg("Error: Monitor is not running!\n");
            } 
            else 
            {
                send_command(input);
            }
        } 
        else if (strcmp(input, "calculate_score") == 0) 
        {
            calculate_score();
        } 
        else if (strcmp(input, "stop_monitor") == 0) 
        {
            if (monitor_pid == 0) 
            {
                write_msg("Error: Monitor is not running!\n");
            } 
            else 
            {
                monitor_stopping = 1;
                kill(monitor_pid, SIGUSR2);
                write_msg("The monitor has been turned off, please wait for completion...\n");
            }
        } 
        else if (strcmp(input, "exit") == 0) 
        {
            if (monitor_pid > 0) 
            {
                write_msg("Error: Monitor is still running! Stop it first.\n");
            } 
            else 
            {
                close(cmd_pipe_fd[1]);
                close(res_pipe_fd[0]);
                write_msg("The program closes\n");
                break;
            }
        } 
        else 
        {
            write_msg("Invalid command\n");
        }
        write_msg("> "); // Display prompt for next command
    }

    return 0;
}