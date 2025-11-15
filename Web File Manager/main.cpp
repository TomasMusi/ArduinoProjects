#include <WiFi.h>
#include <SPIFFS.h>
#include <WebServer.h>

// ================== CONFIGURATION ==================
const char *ssid = "YOUR_WIFI_NAME";
const char *password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

//  HTML PAGE
String htmlPage()
{
    String page =
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<title>ESP32 Souborovy Manager</title>"
        "<style>"
        "body{font-family:Arial;max-width:800px;margin:20px auto;background:#111;color:#eee;}"
        "h1{color:#4caf50;}"
        "table{border-collapse:collapse;width:100%;margin-top:10px;}"
        "th,td{border:1px solid #555;padding:6px;text-align:left;}"
        "a{color:#4caf50;text-decoration:none;}"
        "a:hover{text-decoration:underline;}"
        ".upload-box{margin-top:20px;padding:10px;border:1px solid #444;background:#222;}"
        "input[type=file]{color:#ccc;}"
        "</style></head><body>";

    page += "<h1>ESP32 File Manager</h1>";
    page += "<p>Board: Arduino Nano ESP32 &nbsp;|&nbsp; Souborovy System: SPIFFS</p>";

    // List files
    page += "<h2>Soubory</h2>";
    File root = SPIFFS.open("/");
    if (!root || !root.isDirectory())
    {
        page += "<p><b>Chyba Pri Cteni SPIFFS.</b></p>";
    }
    else
    {
        File file = root.openNextFile();
        page += "<table><tr><th>Jmeno</th><th>Velikost (bytes)</th><th>Akce</th></tr>";
        while (file)
        {
            String name = String(file.name());
            size_t size = file.size();
            page += "<tr><td>" + name + "</td><td>" + String(size) + "</td>";
            page += "<td>"
                    "<a href=\"/download?name=" +
                    name + "\">Stahnout</a> | "
                           "<a href=\"/delete?name=" +
                    name + "\" "
                           "onclick=\"return confirm('Delete " +
                    name + "?');\">Smazat</a>"
                           "</td></tr>";
            file = root.openNextFile();
        }
        page += "</table>";
    }

    // Upload form
    page +=
        "<div class='upload-box'>"
        "<h2>Nahrat Soubor</h2>"
        "<form method='POST' action='/upload' enctype='multipart/form-data'>"
        "<input type='file' name='file'>"
        "<input type='submit' value='Upload'>"
        "</form>"
        "</div>";

    page += "</body></html>";
    return page;
}

//  HANDLERS

// Main page
void handleRoot()
{
    server.send(200, "text/html", htmlPage());
}

// File download
void handleDownload()
{
    if (!server.hasArg("name"))
    {
        server.send(400, "text/plain", "Missing 'name' parameter");
        return;
    }

    String path = server.arg("name");
    if (!SPIFFS.exists(path))
    {
        server.send(404, "text/plain", "File not found");
        return;
    }

    File file = SPIFFS.open(path, "r");
    server.streamFile(file, "application/octet-stream");
    file.close();
}

// File delete
void handleDelete()
{
    if (!server.hasArg("name"))
    {
        server.send(400, "text/plain", "Missing 'name' parameter");
        return;
    }

    String path = server.arg("name");
    if (!SPIFFS.exists(path))
    {
        server.send(404, "text/plain", "File not found");
        return;
    }

    SPIFFS.remove(path);
    server.sendHeader("Location", "/");
    server.send(303); // redirect
}

// File upload
File uploadFile;

void handleFileUpload()
{
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
            filename = "/" + filename;
        uploadFile = SPIFFS.open(filename, "w");
        Serial.printf("Upload start: %s\n", filename.c_str());
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (uploadFile)
        {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (uploadFile)
        {
            uploadFile.close();
            Serial.printf("Upload finished: %u bytes\n", upload.totalSize);
        }
    }
}

// After upload finished
void handleUploadDone()
{
    server.sendHeader("Location", "/");
    server.send(303); // redirect to main page
}

// SETUP AND LOOP

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== ESP32 File Manager (SPIFFS) ===");
    // initialization of SPIFFS
    if (!SPIFFS.begin(true))
    { // true = format if fails
        Serial.println("SPIFFS Mount Failed");
    }
    else
    {
        Serial.println("SPIFFS mounted.");
    }

    // WiFi connect
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.printf("Connecting to WiFi: %s\n", ssid);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nFailed to connect to WiFi.");
    }
    else
    {
        Serial.println("\nWiFi connected.");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.println("Open this in your browser: http://" + WiFi.localIP().toString());
    }

    // Routes REST API
    server.on("/", HTTP_GET, handleRoot);
    server.on("/download", HTTP_GET, handleDownload);
    server.on("/delete", HTTP_GET, handleDelete);

    // upload: handler plus upload callback
    server.on("/upload", HTTP_POST, handleUploadDone, handleFileUpload);

    server.begin();
    Serial.println("HTTP server started.");
}

void loop()
{
    server.handleClient();
}
