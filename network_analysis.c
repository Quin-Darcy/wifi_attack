#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "network_analysis.h"
#include "utils.h"


int validate_line(char* line, char* fields[EXPECTED_FIELD_COUNT], const char* my_essid) 
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

        // Avoid capturing your own network and guest network if same ESSID prefix is present
        if (strstr(fields[field_count], my_essid) != NULL)
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


int parse_capture(Network networks[MAX_NETWORKS], int* total_networks, const char* my_essid)
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
        int line_status = validate_line(line, fields, my_essid);
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

int compare_by_power(const void *a, const void *b) 
{
    const Network *networkA = (const Network *)a;
    const Network *networkB = (const Network *)b;
    return (networkB->power - networkA->power); 
}

void print_network(Network n)
{
    printf("ESSID: %s\n", n.essid);
    printf("    BSSID: %s\n", n.bssid);
    printf("    CHANNEL: %d\n", n.channel);
    printf("    POWER: %d\n", n.power);
    printf("--------------------------------\n");
}