#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>

// File names for treasure data and log
#define TREASURE_FILE "treasures.bin"
#define LOG_FILE "logged_hunt"

// Structure to store treasure information
typedef struct 
{
    int id;
    char name[32];
    float latitude; 
    float longitude;
    char clue[1028];
    int value;
} Treasure;

// Reads input from stdin into buffer, up to max_len, until newline
// Without scanf
void read_input(char *buffer, size_t max_len) 
{
    int i = 0;
    char c;
    while (i < max_len - 1 && read(0, &c, 1) > 0 && c != '\n') 
    {
        buffer[i++] = c;
    }
    buffer[i] = '\0';
}

// Writes a message to stdout
// Without printf
void write_msg(const char *msg) 
{
    write(1, msg, strlen(msg));
}

// Logs an action with timestamp to the hunt's log file
void log_action(const char *hunt_id, const char *action) 
{
    char log_path[512];
    // Construct path to log file
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);

    // Open log file in append mode, create if it doesn't exist
    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0777);
    if (fd == -1) 
        exit(-1);

    // Get current time
    time_t now = time(NULL);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    char log_entry[1024];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", time_buf, action);
    write(fd, log_entry, strlen(log_entry));
    close(fd);
}

// Creates a symbolic link to the hunt's log file
void create_symlink(const char *hunt_id) 
{
    char link_name[256];
    // Name of the symbolic link
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);
    char target[512];
    snprintf(target, sizeof(target), "%s/%s", hunt_id, LOG_FILE);
    symlink(target, link_name);
}
// Adds a new treasure to the hunt
void add(const char *hunt_id) 
{
    char dir_path[256];
    char file_path[512];
    char input[1050];

    // Create directory path for the hunt
    snprintf(dir_path, sizeof(dir_path), "./%s", hunt_id);
    
    // Create directory if it doesn't exist
    if (mkdir(dir_path, 0777) == -1 && errno != EEXIST) 
    {
        write_msg("Erorr at directory opening\n");
        exit(-1);
    }

    // Path to treasure file
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, TREASURE_FILE);
    // Open treasure file in append mode
    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0777);
    if (fd == -1)
    {
        write_msg("Erorr at file opening\n");
        exit(-1);
    }

    Treasure t;

    // Read treasure details from user
    write_msg("ID: ");
    read_input(input, sizeof(input));
    t.id = atoi(input);

    write_msg("Name: ");
    read_input(t.name, sizeof(t.name));

    write_msg("Latitude: ");
    read_input(input, sizeof(input));
    t.latitude = atof(input);

    write_msg("Longitude: ");
    read_input(input, sizeof(input));
    t.longitude = atof(input);

    write_msg("Clue: ");
    read_input(t.clue, sizeof(t.clue));

    write_msg("Value: ");
    read_input(input, sizeof(input));
    t.value = atoi(input);

    // Write treasure to file
    write(fd, &t, sizeof(Treasure));
    close(fd);

    // Log the action and create symlink
    log_action(hunt_id, "Added a treasure");
    create_symlink(hunt_id);
    write_msg("Successfully added treasure.\n");
}

// Lists all treasures in the hunt
void list(const char *hunt_id) 
{
    char file_path[512];
    // Path to treasure file
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);

    struct stat st;
    // Check if treasure file exists
    if (stat(file_path, &st) == -1) 
    {
        write_msg("Wrong id.\n");
        exit(-1);
    }

    // Display hunt info
    char msg[2048];
    snprintf(msg, sizeof(msg), "Hunt: %s\nSize: %ld bytes\n", hunt_id, st.st_size);
    write_msg(msg);

    // Display last modified time
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "Last modify: %Y-%m-%d %H:%M:%S\n", localtime(&st.st_mtime));
    write_msg(timebuf);

    // Open treasure file for reading
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Erorr at file opening\n");
        exit(-1);
    }

    Treasure t;
    // Read and display each treasure
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        snprintf(msg, sizeof(msg), "ID: %d\nName: %s\nLat: %.2f Long: %.2f\nClue: %s\nValue: %d\n\n", t.id, t.name, t.latitude, t.longitude, t.clue, t.value);
        write_msg(msg);
    }
    close(fd);
    log_action(hunt_id, "Listed treasures");
}

// Displays a specific treasure by ID
void view(const char *hunt_id, const char *id) 
{
    int search_id = atoi(id);
    char file_path[512];
    // Path to treasure file
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);

    // Open treasure file for reading
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Erorr at file opening\n");
        exit(-1);
    }

    Treasure t;
    // Search for treasure with matching ID
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
    log_action(hunt_id, "Viewed treasure");
}

// Removes a specific treasure by ID
void remove_treasure(const char *hunt_id, const char *id) 
{
    int search_id = atoi(id);
    char file_path[512], temp_path[512];
    // Paths for treasure file and temporary file
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);
    snprintf(temp_path, sizeof(temp_path), "%s/temp.bin", hunt_id);

    // Open files for reading and writing
    int fd = open(file_path, O_RDONLY);
    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1 || temp_fd == -1)
    {
        write_msg("Error at file opening\n");
        exit(-1);
    }

    Treasure t;
    // Copy all treasures except the one to remove
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        if (t.id != search_id) 
            write(temp_fd, &t, sizeof(Treasure));
    }
    close(fd);
    close(temp_fd);
    // Replace original file with updated one
    rename(temp_path, file_path);

    log_action(hunt_id, "Removed treasure");
    write_msg("Treasure removed.\n");
}

// Removes a hunt directory and preserves log via a new file
void remove_hunt(const char *hunt_id) 
{
    char treasure_path[512];
    char log_path[512];
    char new_log_path[512];
    char link_name[256];

    // Construct paths for files and symlink
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s", hunt_id, TREASURE_FILE);
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);
    snprintf(new_log_path, sizeof(new_log_path), "logged_hunt-%s.log", hunt_id);
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);

    // Copy log file to new location to preserve it
    int src_fd = open(log_path, O_RDONLY);
    if (src_fd != -1) {
        int dst_fd = open(new_log_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (dst_fd == -1) 
        {
            write_msg("Error copying log file.\n");
            close(src_fd);
            exit(-1);
        }

        // Transfer log contents to new file
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) 
        {
            write(dst_fd, buffer, bytes_read);
        }
        close(src_fd);
        close(dst_fd);

        // Update symbolic link to point to the new log file
        unlink(link_name); // Remove old symlink
        if (symlink(new_log_path, link_name) == -1) 
        {
            write_msg("Error updating symbolic link.\n");
            exit(-1);
        }
    }

    // Remove the treasure file
    if (remove(treasure_path) == -1 && errno != ENOENT) 
    {
        write_msg("Error removing treasure file.\n");
        exit(-1);
    }

    // Remove the log file
    if (remove(log_path) == -1 && errno != ENOENT) 
    {
        write_msg("Error removing log file.\n");
        exit(-1);
    }

    // Remove the directory
    if (rmdir(hunt_id) == -1) 
    {
        write_msg("Error removing directory.\n");
        exit(-1);
    }

    write_msg("Hunt removed.\n");
}

// Main function to handle command-line arguments
int main(int argc, char *argv[]) 
{
    // Check for minimum required arguments
    if (argc < 3) 
    {
        write_msg("To few arguments.\n");
        return -1;
    }

    // Dispatch commandss
    if (strcmp(argv[1], "add") == 0) 
    {
        add(argv[2]);
    } 
    else if (strcmp(argv[1], "list") == 0)
    {
        list(argv[2]);
    }
    else if(strcmp(argv[1], "view")==0)
    {
        view(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "remove_treasure")==0)
    {
        remove_treasure(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "remove_hunt")==0)
    {
        remove_hunt(argv[2]);
    }
    else
    {
        write_msg("Wrong comand.\n");
    }

    return 0;
}