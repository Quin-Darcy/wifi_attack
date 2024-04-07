#include <stdio.h>
#include <stdlib.h>
#include "network_scan.h"
#include "network_analysis.h"

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
    result = scan(interface_name);
    if (result != 0)
    {
        printf("Err: Failed to perform scan.\n");
        return -1;
    }

    // Parse capture file to identity valid targets
    int total_networks = 0;
    Network networks[MAX_NETWORKS];
    result = parse_capture(networks, &total_networks, my_essid);
    if (result != 0)
    {
        printf("Err: Failed to parse capture file.\n");
        return -1;
    }

    if (total_networks < TOP_NETWORKS)
    {
        printf("Insufficient networks captured. Try again ...\n");
        return -1;
    }

    // Sorting the network array by power
    qsort(networks, total_networks, sizeof(Network), compare_by_power);

    printf("Top Networks by Power:\n");
    for (int i = 0; i < TOP_NETWORKS; i++)
    {
        print_network(networks[i]);
    }

    return 0;
}