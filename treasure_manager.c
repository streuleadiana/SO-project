#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

// Structul pentru date
typedef struct 
{
    int id;
    char name[32];
    float latitude; 
    float longitude;
    char clue[1028];
    int value;
} Treasure;

// Citire de la tastatura fara scanf
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

// Afisare la tastatura fara printf
void write_msg(const char *msg)
{
    write(1, msg, strlen(msg));
}

// Functia de adaugare a unui treasure in hunt-ul specificat
void add(const char *hunt_id) 
{
    char dir_path[256];
    char file_path[512];
    char input[1050];

    snprintf(dir_path, sizeof(dir_path), "./%s", hunt_id);
    
    if (mkdir(dir_path, 0777) == -1 && errno != EEXIST) 
    {
        write_msg("Erorr at directory opening\n");
        exit(-1);
    }

    snprintf(file_path, sizeof(file_path), "%s/treasure.txt", dir_path);
    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) 
    {
        write_msg("Erorr at file opening\n");
        exit(-1);
    }

    Treasure t;

    write_msg("ID: ");
    read_input(input, sizeof(input));
    t.id = atoi(input);

    write_msg("Treasure name: ");
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

    char buffer[2048];
    int len = snprintf(buffer, sizeof(buffer), "%d,%s,%.3f,%.3f,%s,%d,\n",
                       t.id, t.name, t.latitude, t.longitude, t.clue, t.value);

    write(fd, buffer, len);
    close(fd);
    write_msg("Successfully added treasure.\n");
}

// Functia de listare a hunt-ului (id, size, ultima modificare si treasure)
void list(const char *hunt_id)
{
    char file[512];
    snprintf(file, sizeof(file), "./%s/treasure.txt", hunt_id);

    struct stat st;
    if (stat(file, &st) == -1) 
    {
        write_msg("Wrong id.\n");
        exit(-1);
    }

    // Afisare nume hunt
    write_msg("Name: ");
    write_msg(hunt_id);
    write_msg("\n");

    // Afisare size fisier
    char buf[256];
    snprintf(buf, sizeof(buf), "File size: %ld bytes\n", st.st_size);
    write_msg(buf);

    // Afisare ultima modificare
    char timebuf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "Last modify: %Y-%m-%d %H:%M:%S\n", tm_info);
    write_msg(timebuf);

    // Citire si afisare treasures
    int fd = open(file, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Erorr at file opening\n");
        exit(-1);
    }

    write_msg("Treasures:\n");
    char line[2048];
    int idx = 0;
    char ch;
    while (read(fd, &ch, 1) > 0) 
    {
        if (ch == '\n' || idx >= sizeof(line) - 1) 
        {
            line[idx] = '\0';
            if (strlen(line) > 0) 
            {
                int t_id, t_value;
                char t_name[32], t_clue[1028];
                float t_lat, t_long;

                // Extragem informațiile manual, fără scanf
                char *token = strtok(line, ",");
                t_id = atoi(token);

                token = strtok(NULL, ",");
                strncpy(t_name, token, sizeof(t_name));

                token = strtok(NULL, ",");
                t_lat = atof(token);

                token = strtok(NULL, ",");
                t_long = atof(token);

                token = strtok(NULL, ",");
                strncpy(t_clue, token, sizeof(t_clue));

                token = strtok(NULL, ",");
                t_value = atoi(token);

                // Afișăm informațiile despre comoară
                char treasure_msg[2048];
                snprintf(treasure_msg, sizeof(treasure_msg),
                         "\nID: %d\nName: %s\nLatitude: %.3f, Longitude: %.3f\nClue: %s\nValue: %d\n",
                         t_id, t_name, t_lat, t_long, t_clue, t_value);
                write_msg(treasure_msg);
            }
            idx = 0;
        } 
        else 
        {
            line[idx++] = ch;
        }
    }

    close(fd);
}

void view(const char *hunt_id, const char* id)
{
    char file[512];
    snprintf(file, sizeof(file), "./%s/treasure.txt", hunt_id);

    int fd = open(file, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Erorr at file opening\n");
        exit(-1);
    }

    char line[2048];
    int idx = 0;
    char ch;
    int ok=0;
    while (read(fd, &ch, 1) > 0) 
    {
        if (ch == '\n' || idx >= sizeof(line) - 1) 
        {
            line[idx] = '\0';
            if (strlen(line) > 0) 
            {
                int t_id, t_value;
                char t_name[32], t_clue[1028];
                float t_lat, t_long;

                // Extragem informațiile manual, fără scanf
                char *token = strtok(line, ",");
                t_id = atoi(token);

                token = strtok(NULL, ",");
                strncpy(t_name, token, sizeof(t_name));

                token = strtok(NULL, ",");
                t_lat = atof(token);

                token = strtok(NULL, ",");
                t_long = atof(token);

                token = strtok(NULL, ",");
                strncpy(t_clue, token, sizeof(t_clue));

                token = strtok(NULL, ",");
                t_value = atoi(token);

                if(t_id==atoi(id))
                {
                    char treasure_msg[2048];
                    snprintf(treasure_msg, sizeof(treasure_msg),
                            "ID: %d\nName: %s\nLatitude: %.3f, Longitude: %.3f\nClue: %s\nValue: %d\n",
                            t_id, t_name, t_lat, t_long, t_clue, t_value);
                    write_msg(treasure_msg);
                    ok=1;
                    break;
                }
            }
            idx = 0;
        } 
        else 
        {
            line[idx++] = ch;
        }
    }
    if(ok==0)
    {
        write_msg("ID doesn't exist.\n");
    }
    
    close(fd);
}

void remove_treasure(const char *hunt_id, const char *id)
{
    char file[512];
    snprintf(file, sizeof(file), "./%s/treasure.txt", hunt_id);

    int fd = open(file, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Error at file opening\n");
        exit(-1);
    }

    char buffer[4096];
    int bytes_read = read(fd, buffer, sizeof(buffer));
    if (bytes_read == -1) 
    {
        write_msg("Error reading file\n");
        close(fd);
        exit(-1);
    }
    close(fd);

    // Process the buffer to remove the treasure with the given id
    char temp_file[512];
    snprintf(temp_file, sizeof(temp_file), "./%s/treasure_temp.txt", hunt_id);

    int temp_fd = open(temp_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1)
    {
        write_msg("Error opening temporary file\n");
        exit(-1);
    }

    int ok=0;
    char line[2048];
    int idx = 0;
    for (int i = 0; i < bytes_read; i++) 
    {
        char ch = buffer[i];
        
        if (ch == '\n' || idx >= sizeof(line) - 1) 
        {
            line[idx] = '\0';
            if (strlen(line) > 0) 
            {
                int t_id, t_value;
                char t_name[32], t_clue[1028];
                float t_lat, t_long;

                char *token = strtok(line, ",");
                t_id = atoi(token);

                token = strtok(NULL, ",");
                strncpy(t_name, token, sizeof(t_name));

                token = strtok(NULL, ",");
                t_lat = atof(token);

                token = strtok(NULL, ",");
                t_long = atof(token);

                token = strtok(NULL, ",");
                strncpy(t_clue, token, sizeof(t_clue));

                token = strtok(NULL, ",");
                t_value = atoi(token);

                // Only write back lines that don't match the id
                if (t_id != atoi(id))
                {
                    snprintf(buffer, sizeof(buffer),
                             "%d,%s,%.3f,%.3f,%s,%d\n",
                             t_id, t_name, t_lat, t_long, t_clue, t_value);
                    write(temp_fd, buffer, strlen(buffer));
                }
                else
                {
                    ok=1;
                }
            }
            idx = 0;
        } 
        else 
        {
            line[idx++] = ch;
        }
    }

    close(temp_fd);

    // Replace the old file with the updated one
    if (remove(file) != 0)
    {
        write_msg("Error removing old file\n");
        exit(-1);
    }

    if (rename(temp_file, file) != 0)
    {
        write_msg("Error renaming temporary file\n");
        exit(-1);
    }

    if(ok==1)
    {
        write_msg("Successfully removed treasure.\n");
    }
    else
    {
        write_msg("Id doesn't exist, so no treasure was deleted.\n");
    }
}


void remove_hunt(const char *hunt_id)
{
    char dir[256];
    snprintf(dir,sizeof(dir),"./%s", hunt_id);
    struct stat st;
    if(stat(dir, &st)==-1 || !S_ISDIR(st.st_mode))
    {
        write_msg("Hunt does not exist.\n");
        exit(-1);
    }
    char file[512];
    snprintf(file,sizeof(file), "%s/treasure.txt", dir);
    if(remove(file)==-1)
    {
        write_msg("Error removing treasure file.\n");
        exit(-1);
    }
    if(rmdir(dir)==-1)
    {
        write_msg("Error removing hunt directory.\n");
    }
    else
    {
        write_msg("Hunt removed successfully.\n");
    }
}

int main(int argc, char *argv[]) 
{
    if (argc < 3) 
    {
        write_msg("To few arguments.\n");
        return -1;
    }

    // Alegerea optiunii
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