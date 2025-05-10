#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// File name
#define TREASURE_FILE "treasures.bin"

// Treasure structure
typedef struct 
{
    int id;
    char name[32];
    float latitude; 
    float longitude;
    char clue[1028];
    int value;
} Treasure;

// Calculates the total score for a hunt
void calculate_score(const char *hunt_id) 
{
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        printf("Error: Could not open treasures file for hunt %s\n", hunt_id);
        return;
    }

    Treasure t;
    int total_score = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        total_score += t.value;
    }

    close(fd);
    printf("Total score: %d\n", total_score);
}

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    {
        printf("Try: %s <hunt_id>\n", argv[0]);
        return -1;
    }

    calculate_score(argv[1]);
    return 0;
}