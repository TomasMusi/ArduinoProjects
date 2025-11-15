#include <WiFi.h>
#include <ESP32Ping.h>

int networkCount = 0;
bool waitingForNetworkChoice = true;
bool waitingForPassword = false;
bool connected = false;
bool inToolsMenu = false;

String selectedSSID = "";
String password = "";
String inputBuffer = "";

void setup()
{
    // this means, start a communication between your ESP32 and your computer at the speed of 115200 bits per second.
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== ESP32 WiFi Selector ===");
    Serial.println("Type 'q' anytime to stop program.\n");

    Serial.println("Scanning networks...\n");

    // scanning for wifi connections.
    networkCount = WiFi.scanNetworks();

    if (networkCount == 0)
    {
        Serial.println("No networks found.");
    }
    else
    {
        Serial.printf("Found %d networks:\n", networkCount);
        for (int i = 0; i < networkCount; i++)
        {
            Serial.printf("%d) %s (RSSI: %d dBm)\n",
                          i + 1,
                          WiFi.SSID(i).c_str(),
                          WiFi.RSSI(i));
        }
    }

    Serial.println("\nEnter the number of the WiFi network you want to connect to:");
}

void loop()
{

    if (inToolsMenu)
    {
        handleToolsMenu();
        return;
    }

    if (connected)
    {
        showToolsMenu();
        inToolsMenu = true;
        return;
    }

    if (Serial.available())
    {
        char c = Serial.read();

        if (c == 'q')
        {
            Serial.println("\nProgram stopped.");
            while (true)
            {
            }
        }

        if (c == '\n' || c == '\r')
        {

            if (inputBuffer.length() == 0)
                return;

            if (waitingForNetworkChoice)
            {
                int choice = inputBuffer.toInt();
                if (choice < 1 || choice > networkCount)
                {
                    Serial.println("Invalid number. Try again:");
                }
                else
                {
                    selectedSSID = WiFi.SSID(choice - 1);
                    Serial.printf("Selected WiFi: %s\n", selectedSSID.c_str());
                    Serial.println("Enter WiFi password:");
                    waitingForNetworkChoice = false;
                    waitingForPassword = true;
                }
            }
            else if (waitingForPassword)
            {
                password = inputBuffer;
                Serial.println("Connecting...\n");
                connectToWiFi();
            }

            inputBuffer = "";
        }
        else
        {
            inputBuffer += c;
        }
    }
}

//   CONNECT TO WIFI

void connectToWiFi()
{
    waitingForPassword = false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(selectedSSID.c_str(), password.c_str());

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 30)
    {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\n\n=== Connected Successfully! ===");
        Serial.printf("SSID: %s\n", selectedSSID.c_str());
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());

        connected = true;
    }
    else
    {
        Serial.println("\nFailed to connect. Wrong password or weak signal.");
        Serial.println("Restart board to try again.");
    }
}

//   NETWORK TOOLS MENU

void showToolsMenu()
{
    Serial.println("\n=== Network Tools ===");
    Serial.println("1) Ping Gateway");
    Serial.println("2) Show WiFi Info");
    Serial.println("3) Scan Local Devices");
    Serial.println("4) Ping Google DNS (8.8.8.8)");
    Serial.println("5) Disconnect WiFi");
    Serial.print("Choose option: ");
}

void handleToolsMenu()
{
    if (!Serial.available())
        return;

    char c = Serial.read();

    if (c == 'q')
    {
        Serial.println("\nProgram stopped.");
        // Microcontroller can't stop if they have power, thats why we are using infity whileloop, in order to make him stop! We will use next time this: esp_deep_sleep_start();  its the closest thing to "stop".
        while (true)
        {
        }
    }

    // if the users just clicks enter, ignore it.
    if (c == '\n' || c == '\r')
        return;

    // switch if statement
    switch (c)
    {

    case '1':
        pingGateway();
        break;

    case '2':
        showWiFiInfo();
        break;

    case '3':
        scanLocalDevices();
        break;

    case '4':
        pingGoogle();
        break;

    case '5':
        disconnectWiFi();
        return;

    default:
        Serial.println("Invalid choice.");
    }

    showToolsMenu();
}

//   TOOL FUNCTIONS

// First Option
void pingGateway()
{
    IPAddress gw = WiFi.gatewayIP();
    Serial.printf("\nPinging gateway %s ...\n", gw.toString().c_str());

    if (Ping.ping(gw))
    {
        Serial.println("Gateway reachable.");
    }
    else
    {
        Serial.println("Gateway NOT reachable.");
    }
}

// Second Option
void showWiFiInfo()
{
    Serial.println("\n=== WiFi Info ===");
    Serial.printf("SSID: %s\n", selectedSSID.c_str());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
}

// Third Option
void scanLocalDevices()
{
    Serial.println("\nFast scanning local network (10-second limit)...");

    IPAddress base = WiFi.localIP();
    unsigned long startTime = millis();
    int found = 0;

    for (int i = 1; i < 255; i++)
    {

        // Stop after 10 seconds
        if (millis() - startTime > 10000)
        {
            Serial.println("\nTime limit reached (10 seconds).");
            break;
        }

        // Allow user to CANCEL scan with 'q'
        if (Serial.available())
        {
            char c = Serial.read();
            if (c == 'q')
            {
                Serial.println("\nScan cancelled by user.");
                return;
            }
        }

        IPAddress ip(base[0], base[1], base[2], i);

        // Send ONE ping (fast)
        bool alive = Ping.ping(ip, 1);

        if (alive)
        {
            Serial.printf("Device found: %s (avg %.2f ms)\n",
                          ip.toString().c_str(),
                          Ping.averageTime());
            found++;
        }
    }

    Serial.printf("\nScan complete. Devices found: %d\n", found);
}

// Fourth Option
void pingGoogle()
{
    Serial.println("\nPinging 8.8.8.8 ...");

    if (Ping.ping("8.8.8.8"))
    {
        Serial.println("Google DNS reachable.");
    }
    else
    {
        Serial.println("Google DNS NOT reachable.");
    }
}

// Last option
void disconnectWiFi()
{
    Serial.println("\nDisconnecting WiFi...");
    WiFi.disconnect();
    Serial.println("Disconnected.");
    while (true)
    {
    }
}
