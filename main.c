#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMAND_LENGTH 256
#define SCAN_TIMEOUT 60
#define CAPTURE_PREFIX "scan_capture"
#define MAX_ESSID_LENGTH 128
#define BSSID_LENGTH 17
#define MAX_NETWORKS 100
#define MAX_LINE_LENGTH 1024
#define EXPECTED_FIELD_COUNT 14
#define MAX_FIELD_LENGTH 128
#define ESSID_INDEX 13
#define BSSID_INDEX 0
#define CHANNEL_INDEX 3
#define POWER_INDEX 8

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

// Helper function to trim whitespace from a string in-place
char* trim_whitespace(char* str) {
    char* end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    *(end+1) = 0;

    return str;
}

int validate_line(char* line, char* fields[EXPECTED_FIELD_COUNT]) 
{
    char* token = strtok(line, ",");
    int field_count = 0;

    while (token != NULL && field_count < EXPECTED_FIELD_COUNT) 
    {
        fields[field_count] = trim_whitespace(token);
        if (strcmp(fields[field_count], "Station MAC") == 0) 
        {
            return -1; // Signals the beginning of the second section
        }

        // Denotes header line
        if (strcmp(fields[field_count], "ESSID") == 0)
        {
            return 1;
        }

        // Check if any of the fields are blank
        if (strcmp(fields[field_count], "") == 0)
        {
            return 1;
        }
        token = strtok(NULL, ",");
        field_count++;
    }

    // Checking field count
    if (field_count < EXPECTED_FIELD_COUNT) 
    {
        return 1;
    }

    return 0; // Line is valid and should be processed
}


int parse_capture(Network networks[MAX_NETWORKS], int* total_networks)
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

    // To temporarily store the parsed fields and validate them
    char* fields[EXPECTED_FIELD_COUNT];

    // Iterate through each line and store the validated ones
    int network_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp) && network_count < MAX_NETWORKS) 
    {
        int line_status = validate_line(line, fields);
        if (line_status == 1)
        {
            // Found issue, skip and keep processing
            continue;
        }
        else if (line_status == -1)
        {
            // Found start of second section - No further processing
            break;
        }
        else 
        {
            // Add the network
            strcpy(networks[network_count].essid, fields[ESSID_INDEX]);
            strcpy(networks[network_count].bssid, fields[BSSID_INDEX]);
            networks[network_count].channel = atoi(fields[CHANNEL_INDEX]);
            networks[network_count].power = atoi(fields[POWER_INDEX]);
            network_count++;
        }
    }

    *total_networks = network_count;

    fclose(fp);
    return 0;
}

void print_network(Network n)
{
    printf("ESSID: %s\n", n.essid);
    printf("    BSSID: %s\n", n.bssid);
    printf("    CHANNEL: %d\n", n.channel);
    printf("    POWER: %d\n", n.power);
    printf("--------------------------------\n");
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
    int total_networks = 0;
    Network networks[MAX_NETWORKS];
    result = parse_capture(networks, &total_networks);
    if (result != 0)
    {
        printf("Err: Failed to parse capture file.\n");
        return -1;
    }

    for (int i = 0; i < total_networks; i++)
    {
        print_network(networks[i]);
    }

    return 0;
}