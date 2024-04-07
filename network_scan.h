#ifndef NETWORK_SCAN_H
#define NETWORK_SCAN_H

#define MAX_COMMAND_LENGTH 256
#define NETWORK_SCAN_TIMEOUT 60
#define CLIENT_SCAN_TIMEOUT 60
#define NETWORK_CAPTURE_PREFIX "network_capture"
#define CLIENT_CAPTURE_PREFIX "client_capture"

int initialize_interface(const char* interface_name);
int network_scan(const char* interface_name);
int client_scan(const char* interface_name, const char* bssid, const char* essid, const int channel);

#endif