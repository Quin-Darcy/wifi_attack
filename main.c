#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 256
#define SCAN_TIMEOUT 60
#define CAPTURE_PREFIX "scan_capture"
#define MAX_ESSID_LENGTH 128
#define BSSID_LENGTH 17
#define MAX_NETWORKS 100
#define MAX_LINE_LENGTH 1024

typedef struct
{
    char essid[MAX_ESSID_LENGTH];
    char bssid[BSSID_LENGTH + 1];
    int channel;
    int power;
} Network;

int initialize_interface(const char* interface_name)
{
    printf("[i] Initializing %s ...\n", interface_name);

    int status;
    char commands[4][MAX_COMMAND_LENGTH]; // Array of strings each large enough to hold largest command

    // Initialize commands with snprintf
    snprintf(commands[0], sizeof(commands[0]), "sudo ip link set %s down", interface_name);
    snprintf(commands[1], sizeof(commands[1]), "sudo ip link set dev %s address 12:34:56:78:9a:bc", interface_name);
    snprintf(commands[2], sizeof(commands[2]), "sudo iw dev %s set type monitor", interface_name);
    snprintf(commands[3], sizeof(commands[3]), "sudo ip link set %s up", interface_name);

    // Run commands and check status after each
    for (int i = 0; i < 4; i++) {
        status = system(commands[i]);
        if (status == -1) {
            printf("Err: Failed to run %s\n", commands[i]);
            return -1; 
        }
    }

    // Running the command and checking if the interface is in monitor mode
    char status_check[MAX_COMMAND_LENGTH];
    snprintf(status_check, sizeof(status_check), "iw dev %s info | grep 'type monitor' > /dev/null 2>&1", interface_name);
    status = system(status_check);

    // Checking the command's return status
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            printf("[+] %s now in monitor mode.\n", interface_name);
        } else {
            printf("[!] %s failed to be put into monitor mode.\n", interface_name);
            return -1;
        }
    } else {
        printf("[!] Failed to execute %s.\n", status_check);
        return -1;
    }

    return 0;
}

int scan(const char* interface_name)
{
    printf("[i] Performing wireless network scan ...\n");

    // Remove all previous captures before continuing
    system("sudo rm *.csv > /dev/null 2>&1");
    system("sudo rm *.cap > /dev/null 2>&1");

    // Create the command string to perform scan and call command with system()
    char command[MAX_COMMAND_LENGTH];
    snprintf(command, sizeof(command), "sudo timeout %d airodump-ng %s -w %s --output-format csv > /dev/null 2>&1", SCAN_TIMEOUT, interface_name, CAPTURE_PREFIX);
    int status = system(command);
    if (status == -1)
    {
        printf("Err: Failed to perform scan.\n");
        return -1;
    }

    printf("[+] Scan captured in %s-01.csv\n", CAPTURE_PREFIX);

    return 0;
}

int is_valid_line(char* line) 
{
    int channel, speed;
    char privacy[10], cipher[10], authentication[10];
    sscanf(line, "%*[^,],%*[^,],%*[^,],%d,%d,%[^,],%[^,],%[^,],%*d,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,]",
           &channel, &speed, privacy, cipher, authentication);

    return !(channel == -1 || speed == -1 || strcmp(privacy, "") == 0 || strcmp(cipher, "") == 0 || strcmp(authentication, "") == 0);
}

int parse_capture(Network networks[MAX_NETWORKS])
{
    printf("[i] Parsing capture file ...\n");

    // Open the file
    char filename[MAX_COMMAND_LENGTH];
    snprintf(filename, sizeof(filename), "%s-01.csv", CAPTURE_PREFIX);
    FILE *fp = fopen(filename, "r");
    if (!fp) 
    {
        printf("Failed to open file\n");
        return 1;
    }

    int line_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp) && line_count < MAX_NETWORKS) 
    {
        // Skip header lines
        if (line_count < 2)
            line_count++;

        // Check if line is valid based on your criteria
        if (!is_valid_line(line)) continue;

        // Parse valid line
        int index = line_count - 2;
        sscanf(line, "%17[^,],%*[^,],%*[^,],%d,%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^,],%*[^,],%*[^,],%*[^,],%32[^,]",
               networks[index].bssid, &networks[index].channel, &networks[index].power, networks[index].essid);

        line_count++;
    }

    fclose(fp);

    // Example: Print parsed networks
    for (int i = 0; i < line_count - 2; i++) {
        printf("ESSID: %s, BSSID: %s, Power: %d\n", networks[i].essid, networks[i].bssid, networks[i].power);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    // Validate arguments
    if (argc != 2)
    {
        printf("Err: Invalid input\n");
        return -1;
    }

    int result;

    // Initialize wireless interface by placing it into monitor mode
    char* interface_name = argv[1];
    result = initialize_interface(interface_name);
    if (result != 0)
    {
        printf("Err: Failed to initialize interface\n");
        return -1;
    }

    // Perform scan of nearby wireless networks
    result = scan(interface_name);
    if (result != 0)
    {
        printf("Err: Failed to perform scan.\n");
        return -1;
    }

    // Parse capture file to identity valid targets
    Network networks[MAX_NETWORKS];
    result = parse_capture(networks);
    if (result != 0)
    {
        printf("Err: Failed to parse capture file.\n");
        return -1;
    }
    
    return 0;
}