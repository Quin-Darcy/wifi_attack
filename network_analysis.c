#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "network_analysis.h"
#include "utils.h"


int validate_network_line(char* line, char* fields[EXPECTED_NETWORK_FIELD_COUNT], const char* my_essid) 
{
    char* token = strtok(line, ",");
    int field_count = 0;

    while (token != NULL && field_count < EXPECTED_NETWORK_FIELD_COUNT) 
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
    if (field_count < EXPECTED_NETWORK_FIELD_COUNT) 
    {
        return 1;
    }

    return 0; // Line is valid and should be processed
}

int validate_client_line(char* line, char* fields[EXPECTED_CLIENT_FIELD_COUNT]) 
{
    static int station_mac_encountered = 0; // 0 = false, 1 = true
    char* token = strtok(line, ",");
    int field_count = 0;

    // Before parsing, check if we have encountered the "Station MAC" header
    if (station_mac_encountered == 0) 
    {
        while (token != NULL) 
        {
            char* temp_token = trim_whitespace(token);
            if (strcmp(temp_token, "Station MAC") == 0) 
            {
                station_mac_encountered = 1; // Header found, subsequent lines are of interest
                return 1; // Indicate that this line is the header and not data
            }
            token = strtok(NULL, ",");
        }
        // If this point is reached in this iteration, "Station MAC" has not been encountered yet
        return 1; // Indicate to skip this line as it's before the "Station MAC" line
    }
    else // "Station MAC" has been encountered, process the line as data
    {
        // Reset tokenization process for actual data parsing
        token = strtok(line, ",");
        while (token != NULL && field_count < EXPECTED_CLIENT_FIELD_COUNT) 
        {
            fields[field_count] = trim_whitespace(token);
            
            // Check if any of the fields are blank
            if (strcmp(fields[field_count], "") == 0)
            {
                return 1; // Indicates an invalid line due to a blank field
            }
            token = strtok(NULL, ",");
            field_count++;
        }

        if (field_count < EXPECTED_CLIENT_FIELD_COUNT) 
        {
            return 1; // Indicates an invalid line due to insufficient fields
        }
        
        return 0; // Line is valid and should be processed
    }
}

int parse_network_capture(Network networks[MAX_NETWORKS], int* total_networks, const char* my_essid)
{
    printf("[i] Parsing %s-01.csv ...\n", NETWORK_CAPTURE_PREFIX);

    // Open the file
    char filename[MAX_COMMAND_LENGTH];
    snprintf(filename, sizeof(filename), "%s-01.csv", NETWORK_CAPTURE_PREFIX);
    FILE *fp = fopen(filename, "r");
    if (!fp) 
    {
        printf("Failed to open file\n");
        return 1;
    }

    // To temporarily store the parsed fields and validate them
    char* fields[EXPECTED_NETWORK_FIELD_COUNT];

    // Iterate through each line and store the validated ones
    int network_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp) && network_count < MAX_NETWORKS) 
    {
        int line_status = validate_network_line(line, fields, my_essid);
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

int parse_client_capture(Client client, const char* essid, const char* bssid)
{
    printf("[i] Parsing %s-%s-01.csv ...\n", essid, CLIENT_CAPTURE_PREFIX);

    // Open the file
    char filename[MAX_COMMAND_LENGTH];
    snprintf(filename, sizeof(filename), "%s-%s-01.csv", essid, CLIENT_CAPTURE_PREFIX);
    FILE *fp = fopen(filename, "r");
    if (!fp) 
    {
        printf("Failed to open file\n");
        return 1;
    }

    // To temporarily store the parsed fields and validate them
    char* fields[EXPECTED_CLIENT_FIELD_COUNT];

    // To store all valid clients and to have something which can be sorted
    Client clients[MAX_CLIENTS];

    // Iterate through each line and store the validated ones
    int client_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp)) 
    {
        int line_status = validate_client_line(line, fields);
        if (line_status == 1)
        {
            // Found issue, skip and keep processing
            continue;
        }
        else 
        {
            // Add the network
            strcpy(clients[client_count].essid, essid);
            strcpy(clients[client_count].bssid, bssid);
            strcpy(clients[client_count].station, fields[STATION_INDEX]);
            clients[client_count].power = atoi(fields[POWER_INDEX - 5]);
            clients[client_count].packets = atoi(fields[PACKETS_INDEX]);
            client_count++;
        }
    }

    if (client_count == 0)
    {
        return 1;
    }

    // If there is 2 or more clients, sort by packets 
    if (client_count > 1)
    {
        qsort(clients, client_count, sizeof(Client), compare_by_packets);
    }
    
    strcpy(client.essid, essid);
    strcpy(client.bssid, bssid);
    strcpy(client.station, clients[0].station);
    client.power = clients[0].power;
    client.packets = clients[0].packets;

    fclose(fp);
    return 0;
}

int compare_by_power(const void *a, const void *b) 
{
    const Network *networkA = (const Network *)a;
    const Network *networkB = (const Network *)b;
    return (networkB->power - networkA->power); 
}

int compare_by_packets(const void *a, const void *b) 
{
    const Client *clientA = (const Client *)a;
    const Client *clientB = (const Client *)b;
    return (clientB->packets - clientA->packets); 
}

void print_network(Network n)
{
    printf("ESSID: %s\n", n.essid);
    printf("    BSSID: %s\n", n.bssid);
    printf("    CHANNEL: %d\n", n.channel);
    printf("    POWER: %d\n", n.power);
    printf("--------------------------------\n");
}