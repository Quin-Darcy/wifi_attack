#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 256

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
        }
    } else {
        printf("[!] Failed to execute %s.\n", status_check);
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

    // Store name of interface which will be conducting attack
    char* interface_name = argv[1];

    int result = initialize_interface(interface_name);
    if (result != 0)
    {
        printf("Err: Failed to initialize interface\n");
        return -1;
    }

    return 0;
}