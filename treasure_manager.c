#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

// Structu pentru date
typedef struct 
{
    int id;
    char name[32];
    float latitude; 
    float longitude;
    char clue[1028];
    int value;
} Treasure;

// Citire de la tastatura farra scanf
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
        write_msg("Eroare la crearea directorului\n");
        exit(-1);
    }

    snprintf(file_path, sizeof(file_path), "%s/treasure.txt", dir_path);
    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) 
    {
        write_msg("Eroare la deschiderea fișierului\n");
        exit(-1);
    }

    Treasure t;

    write_msg("ID: ");
    read_input(input, sizeof(input));
    t.id = atoi(input);

    write_msg("Nume comoară: ");
    read_input(t.name, sizeof(t.name));

    write_msg("Latitudine: ");
    read_input(input, sizeof(input));
    t.latitude = atof(input);

    write_msg("Longitudine: ");
    read_input(input, sizeof(input));
    t.longitude = atof(input);

    write_msg("Indiciu: ");
    read_input(t.clue, sizeof(t.clue));

    write_msg("Valoare: ");
    read_input(input, sizeof(input));
    t.value = atoi(input);

    char buffer[2048];
    int len = snprintf(buffer, sizeof(buffer), "%d,%s,%.3f,%.3f,%s,%d,\n",
                       t.id, t.name, t.latitude, t.longitude, t.clue, t.value);

    write(fd, buffer, len);
    close(fd);
    write_msg("Comoară adăugată cu succes.\n");
}

// Functia de listare a hunt-ului (id, size, ultima modificare si treasure)
void list(const char *hunt_id)
{
    char file[512];
    snprintf(file, sizeof(file), "./%s/treasure.txt", hunt_id);

    struct stat st;
    if (stat(file, &st) == -1) 
    {
        write_msg("Id gresit\n");
        return;
    }

    // Afisare nume hunt
    write_msg("Hunt: ");
    write_msg(hunt_id);
    write_msg("\n");

    // Afisare size fisier
    char buf[256];
    snprintf(buf, sizeof(buf), "Dimensiune fișier: %ld bytes\n", st.st_size);
    write_msg(buf);

    // Afisare ultima modificare
    char timebuf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "Ultima modificare: %Y-%m-%d %H:%M:%S\n", tm_info);
    write_msg(timebuf);

    // Citire si afisare treasures
    int fd = open(file, O_RDONLY);
    if (fd == -1) 
    {
        write_msg("Eroare la deschiderea fișierului\n");
        return;
    }

    write_msg("Treasures: \n");
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
                         "\nID: %d\nNume: %s\nLatitudine: %.3f, Longitudine: %.3f\nIndiciu: %s\nValoare: %d\n",
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

int main(int argc, char *argv[]) 
{
    if (argc < 3) 
    {
        write_msg("Prea putine argumente.\n");
        return 1;
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
    else
    {
        write_msg("Comanda gresita.\n");
    }

    return 0;
}