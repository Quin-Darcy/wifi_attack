#ifndef NETWORK_SCAN_H
#define NETWORK_SCAN_H

#define MAX_COMMAND_LENGTH 256
#define SCAN_TIMEOUT 60
#define CAPTURE_PREFIX "scan_capture"

int initialize_interface(const char* interface_name);
int scan(const char* interface_name);

#endif