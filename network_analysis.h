#ifndef NETWORK_ANALYSIS_H
#define NETWORK_ANALYSIS_H

#define MAX_COMMAND_LENGTH 256
#define EXPECTED_NETWORK_FIELD_COUNT 14
#define EXPECTED_CLIENT_FIELD_COUNT 7
#define MAX_NETWORKS 100
#define MAX_CLIENTS 10
#define NETWORK_CAPTURE_PREFIX "network_capture"
#define CLIENT_CAPTURE_PREFIX "client_capture"
#define MAX_LINE_LENGTH 1024
#define MAX_ESSID_LENGTH 128
#define BSSID_LENGTH 17
#define ESSID_INDEX 13
#define BSSID_INDEX 0
#define CHANNEL_INDEX 3
#define POWER_INDEX 8
#define STATION_INDEX 0
#define PACKETS_INDEX 4
#define TOP_NETWORKS 3

typedef struct
{
    char essid[MAX_ESSID_LENGTH];
    char bssid[BSSID_LENGTH + 1];
    int channel;
    int power;
} Network;

typedef struct
{
    char essid[MAX_ESSID_LENGTH];
    char bssid[BSSID_LENGTH + 1];
    char station[BSSID_LENGTH + 1];
    int power;
    int packets;
} Client;


int validate_network_line(char* line, char* fields[EXPECTED_NETWORK_FIELD_COUNT], const char* my_essid);
int validate_client_line(char* line, char* fields[EXPECTED_CLIENT_FIELD_COUNT]);
int parse_network_capture(Network networks[MAX_NETWORKS], int* total_networks, const char* my_essid);
int parse_client_capture(Client client, const char* essid, const char* bssid);
int compare_by_power(const void *a, const void *b);
int compare_by_packets(const void *a, const void *b);
void print_network(Network n);

#endif