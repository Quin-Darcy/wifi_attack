#include "network_scan.h"
#include <stdlib.h>
#include <stdio.h>

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
    for (int i = 0; i < 4; i++) 
    {
        status = system(commands[i]);
        if (status == -1) 
        {
            printf("Err: Failed to run %s\n", commands[i]);
            return -1; 
        }
    }

    // Running the command and checking if the interface is in monitor mode
    char status_check[MAX_COMMAND_LENGTH];
    snprintf(status_check, sizeof(status_check), "iw dev %s info | grep 'type monitor' > /dev/null 2>&1", interface_name);
    status = system(status_check);

    // Checking the command's return status
    if (WIFEXITED(status)) 
    {
        if (WEXITSTATUS(status) == 0) 
        {
            printf("[+] %s now in monitor mode.\n", interface_name);
        } 
        else 
        {
            printf("[!] %s failed to be put into monitor mode.\n", interface_name);
            return -1;
        }
    } 
    else 
    {
        printf("[!] Failed to execute %s.\n", status_check);
        return -1;
    }
    return 0;
}

int network_scan(const char* interface_name)
{
    printf("[i] Performing wireless network scan ...\n");

    // Remove all previous captures before continuing
    system("sudo rm *.csv > /dev/null 2>&1");
    system("sudo rm *.cap > /dev/null 2>&1");
    system("sudo rm *.netxml > /dev/null 2>&1");

    // Create the command string to perform scan and call command with system()
    char command[MAX_COMMAND_LENGTH];
    snprintf(command, sizeof(command), "sudo timeout %d airodump-ng %s -w %s --output-format csv > /dev/null 2>&1", NETWORK_SCAN_TIMEOUT, interface_name, NETWORK_CAPTURE_PREFIX);
    int status = system(command);
    if (status == -1)
    {
        printf("Err: Failed to perform scan.\n");
        return -1;
    }

    printf("[+] Scan captured in %s-01.csv\n", NETWORK_CAPTURE_PREFIX);

    return 0;
}

int client_scan(const char* interface_name, const char* bssid, const char* essid, const int channel)
{
    printf("[i] Performing targeted scan against %s ...\n", essid);

    // Create the command string to perform scan and call command with system()
    char command[MAX_COMMAND_LENGTH];
    snprintf(command, sizeof(command), "sudo timeout %d airodump-ng %s -d %s --channel %d -w %s-%s --output-format csv > /dev/null 2>&1", CLIENT_SCAN_TIMEOUT, interface_name, bssid, channel, essid, CLIENT_CAPTURE_PREFIX);
    int status = system(command);
    if (status == -1)
    {
        printf("Err: Failed to perform scan.\n");
        return -1;
    }

    printf("[+] Scan captured in %s-%s-01.csv\n", essid, NETWORK_CAPTURE_PREFIX);

    return 0;
}