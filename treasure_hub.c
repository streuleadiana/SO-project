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

//Copiere din treasure_manager
#define TREASURE_FILE "treasures.bin"

typedef struct 
{
    int id;
    char name[32];
    float latitude; 
    float longitude;
    char clue[1028];
    int value;
} Treasure;

void write_msg(const char *msg) 
{
    write(1, msg, strlen(msg));
}

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
        write_msg("Erorr at file opening\n");
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

void view(const char *hunt_id, const char *id) 
{
    int search_id = atoi(id);
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Erorr at file opening\n");
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

//Functia main partiala
int main() 
{
    
    write_msg("> ");
    /*
    while (fgets(input, sizeof(input), stdin)) 
    {
        input[strcspn(input, "\n")] = '\0';

    
        if (strcmp(input, "start_monitor") == 0) 
        {
            
        } else if (strcmp(input, "list_hunts") == 0) 
        {

        } else if (strncmp(input, "list_treasures ", 15) == 0) 
        {
            
        } else if (strncmp(input, "view_treasure ", 14) == 0) 
        {
            
        } else if (strcmp(input, "stop_monitor") == 0) 
        {
            
        } else if (strcmp(input, "exit") == 0) 
        {

        } 
        else 
        {
            write_msg("Comandă invalidă\n");
        }
        write_msg("> ");
    }
    */
    return 0;
}