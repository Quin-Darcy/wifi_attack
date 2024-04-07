#ifndef NETWORK_ANALYSIS_H
#define NETWORK_ANALYSIS_H

#define MAX_COMMAND_LENGTH 256
#define EXPECTED_FIELD_COUNT 14
#define MAX_NETWORKS 100
#define CAPTURE_PREFIX "scan_capture"
#define MAX_LINE_LENGTH 1024
#define MAX_ESSID_LENGTH 128
#define BSSID_LENGTH 17
#define ESSID_INDEX 13
#define BSSID_INDEX 0
#define CHANNEL_INDEX 3
#define POWER_INDEX 8
#define TOP_NETWORKS 3

typedef struct
{
    char essid[MAX_ESSID_LENGTH];
    char bssid[BSSID_LENGTH + 1];
    int channel;
    int power;
} Network;

int validate_line(char* line, char* fields[EXPECTED_FIELD_COUNT], const char* my_essid);
int parse_capture(Network networks[MAX_NETWORKS], int* total_networks, const char* my_essid);
int compare_by_power(const void *a, const void *b);
void print_network(Network n);

#endif