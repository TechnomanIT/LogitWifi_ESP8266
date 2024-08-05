#define dbg(myFixedText, variableName) \
  Serial.println(variableName);

/*
  Name:    DataLoggerV100Socket ESP-EEPROM.ino
  Created: 19/9/2023 1:09:09 AM
  Author:  Syed Rafiuddin
  Version: 1.0.0
*/

// the setup function runs once when you press reset or power the board
#include <DS1307.h>
#include <Adafruit_AHTX0.h>
#include <ESP8266WiFi.h>
#include "ESP8266HTTPClient.h"
#include "ESP8266WebServer.h"
#include <Wire.h>        // Include the Wire library for I2C communication
#include <OneWire.h>
#include <MapFloat.h>
#include <DallasTemperature.h>
#include <FS.h>
#include <LittleFS.h>
#include <TimeLib.h> // You might need to install this library

ESP8266WebServer server(80);

DS1307 rtc;  // Create an RTC_DS1307 object
// Data wire is connected to GPIO2 (D4 on some boards)
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

//const int ledPin = 16; // D0 pin on NodeMCU
Adafruit_AHTX0 aht;
const int powerCheckPin = 16;

#define FLASH_CS D8 // GPIO15, pin D8 on NodeMCU
#define SECTOR_SIZE 4096 // Sector size of W25Q64JVS (4 KB)

;
typedef enum {
  Sensor_Error,
  RTC_Error,
  SensorID_Error,
  Date_Error,
  Battery_Error
} Status_Mode;


const char* ConfigFile = "/Config.dat";
const char* DataFile = "/Data.dat";

struct login_user_settings {
  bool flag;
  char id[30];
  char password[30];
};

login_user_settings login_user;


String id_str = "admin";
String password_str = "password";


struct Soft_AP_setting {
  char Soft_SSID_Name[30];
  char Soft_Password[30];
};


Soft_AP_setting Soft_AP;

String Soft_SSID_Name_str = "Logit-WiFi";
String Soft_Password_str = "";

uint8_t max_connections = 2; //Maximum Connection Limit for AP
int current_stations = 0, new_stations = 0;

IPAddress Soft_IP(192, 168, 4, 1);
IPAddress Soft_Gateway(192, 168, 4, 1);
IPAddress Soft_Subnet(255, 255, 255, 0);


String Soft_IP_str;
String Soft_Subnet_str;
String Soft_Gateway_str;

String macAddressstr = "";
String SensorID = "";


struct Wifi_Network_Setting {
  char WiFi_Enabled[2];
  char ssidListname[30];
  char Wifi_Password[30];
};


Wifi_Network_Setting Wifi_Network;

String WiFi_Enabled_bl = "0";
String ssidList_str = "";
String Wifi_Password_str = "";
String ssidList = "";


struct Network_DHCP_Setting {
  char DHCP_Enabled[2];
  char DHCP_IP[16];
  char DHCP_Subnet[16];
  char DHCP_Gateway[16];
};

Network_DHCP_Setting Network_DHCP;

String DHCP_Enabled_bl = "0";
String DHCP_IP_str = "192.168.1.1";
String DHCP_Subnet_str = "255.255.255.0";
String DHCP_Gateway_str = "192.168.0.1";


struct server_setting {
  char Server_Address[30];
  char Server_Port[6];
};
server_setting server_setting_API;

String Server_Address_str = "http:www.technoman.biz";
String Server_Port_str = "80";

struct Device_Setting_struct
{
  char sensor_Type[2];
  float rhoffset;
  float tempoffset;
  bool isDisplay;
} Device_Setting;

String DeviceType = "01";
float RhOffset = 0.0;
float TempOffset = 0.0;
bool IsDisplay = false;

struct MemoryAddress {
  int ReadAddress;
  int WriteAddress;
  bool Overwrite;
  bool DataFlag;
} MemoryAddressObj;

int readAddress = 0 ;
int writeAddress = 0;
bool  overwrite = false;
bool  dataFlag = false;


//const int Max_Sector = 5760;
const int Max_Sector = 255;

struct DataPacket_Struct {
  float TEMP; //Temperature
  float HUM; //Humidity
  char DT[18]; //Date Time
  char BS[2]; //Power Status
  int CV; //Power Status
};


byte sensorAddress[8]; // 64-bit address buffer

String loginpage = "<!doctype html> <html lang='en'> <head> <meta charset='utf-8'> <meta name='viewport' content='width=device-width, initial-scale=1'> <title>Wifi Setup</title> <style> *, ::after, ::before { box-sizing: border-box; } body { margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; background-color: #f5f5f5; } .form-control { display: block; width: 100%; height: calc(1.5em + .75rem + 2px); border: 1px solid #ced4da; } button { cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #501b1d; border-color: #501b1d; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .3rem; width: 100% } .form-signin { width: 100%; max-width: 400px; padding: 15px; margin: auto; } h1 { text-align: center } .textstyle { border: 1px solid transparent; border-color: #007bff; background-color:#fff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .4rem; width: 100%; } </style> </head> <body> <main class='form-signin'> <form action='/login' method='post'> <h1 class=''>Login</h1><br /> <div class='form-floating'><label>User ID</label><input class=\"textstyle\" type='text' class='form-control' name='userid'> </div> <div class='form-floating'><br /><label>Password</label><input class=\"textstyle\" type='password' class='form-control' name='password'> </div> <br /> <br /> <button type='submit'>Login</button> </form> </main> </body> </html>";

wl_status_t wifiStatus;


String Status = "";
int analogInPin  = A0;    // Analog input pin
int sensorValue;
float calibration = -0.27; // Check Battery voltage using multimeter & add/subtract the value
float voltage;
int bat_percentage;
volatile bool timerFlag = false;




void setup() {
  pinMode(D3, OUTPUT);
  pinMode(powerCheckPin, INPUT);
  Serial.begin(115200);
  while (!Serial); //wait for serial port to connect - needed for Leonardo only

  // Get reset information
  String resetInfo = String(ESP.getResetInfo());
  Serial.println("Restart cause: " + resetInfo);

  Wire.begin();
  rtc.begin();
  digitalWrite(D3, HIGH);
  delay(2000);

  // Initialize SPI
  SPI.begin();
  SPI.setFrequency(4000000); // Set SPI frequency to 4 MHz
  SPI.setDataMode(SPI_MODE0); // Set SPI mode to 0
  SPI.setBitOrder(MSBFIRST); // Set bit order to MSB first


  Serial.println("Setup Start----");



  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  else
  {
    Serial.println("LittleFS mount Pass");
  }


  if (LittleFS.exists(ConfigFile)) {
    Serial.println("Config File exists. Reading settings...");
    readDataConfig();
  } else {
    Serial.println("Config File does not exist. Setting default values...");
    saveDataConfig();
    delay(5000);
    ESP.restart();
  }
  if (LittleFS.exists(DataFile)) {
    Serial.println("Data File exists. Reading settings...");
    readDataMemory();
  } else {
    Serial.println("Data File does not exist. Setting default values...");
    saveDataMemory();
  }
  Serial.println("Flag " + String(login_user.flag));

  if (login_user.flag)    {
    Serial.println("initialize and get data from Config file");

    Serial.println("__________Login User info__________");
    id_str = String(login_user.id);
    Serial.println("Login User Name: " + id_str);
    password_str = String(login_user.password);
    Serial.println("Login Password: " + password_str);

    Serial.println("__________Soft AP info__________");
    if (String(Soft_AP.Soft_SSID_Name) != "")
    {
      Soft_SSID_Name_str = String(Soft_AP.Soft_SSID_Name);
    }
    Serial.println("AP Name: " + Soft_SSID_Name_str);
    if (String(Soft_AP.Soft_Password) != "")
    {
      Soft_Password_str =  String(Soft_AP.Soft_Password);
    }
    Serial.println("AP Password: " + Soft_Password_str);

    WiFi.mode(WIFI_AP);
    // Set Wi-Fi output power to maximum
    WiFi.setOutputPower(20.5); // Output power in dBm
    if (WiFi.softAPConfig(Soft_IP, Soft_Gateway, Soft_Subnet))
    {
      Serial.println("Access Point configuration done and Ready for connection.");
      if (WiFi.softAP(Soft_SSID_Name_str, Soft_Password_str, 1, false, max_connections) == true)
      {
        Serial.println("Access Point Ready.");
      }
      else
      {
        Serial.println("Unable to Create Access Point");
      }
    }
    else
    {
      Serial.println("Access Point Configuration has been faild.");
    }



    WiFi_Enabled_bl =  Wifi_Network.WiFi_Enabled[0];

    if (WiFi_Enabled_bl == "1")
    {
      Serial.println("__________Network SSID & Password info__________");
      Serial.println("WiFi Enabled");
      unsigned long connectionTimeout = 15000;  // Timeout for WiFi connection attempts (milliseconds)
      unsigned long startTime;

      WiFi_Enabled_bl == "1";
      WiFi.mode(WIFI_AP_STA);
      Serial.println("WIFI MODE: WIFI_AP_STA");

      Serial.println("__________Network DHCP Enable info__________");
      DHCP_Enabled_bl = Network_DHCP.DHCP_Enabled[0];
      Serial.println("DHCP Enable:" + DHCP_Enabled_bl );
      if (DHCP_Enabled_bl == "1")
      {
        Serial.println("Static IP Enable");
        DHCP_Enabled_bl = "1";
        DHCP_IP_str =  String(Network_DHCP.DHCP_IP);
        DHCP_Subnet_str =  String(Network_DHCP.DHCP_Subnet);
        DHCP_Gateway_str =  String(Network_DHCP.DHCP_Gateway);
        Serial.println("Static IP: " + DHCP_IP_str);
        Serial.println("Subnet IP: " + DHCP_Subnet_str);
        Serial.println("Gateway IP: " + DHCP_Gateway_str);


        IPAddress staticIP; // IPAddress object to store the converted IP
        IPAddress gateway; // Gateway address as four integers
        IPAddress subnet; // Subnet mask as four integers

        if (stringToIP(DHCP_IP_str, staticIP) && stringToIP(DHCP_Gateway_str, gateway) && stringToIP(DHCP_Subnet_str, subnet)) {
          WiFi.config(staticIP, gateway, subnet);
        } else {
          Serial.println("Invalid IP address format");
        }
      }
      if (DHCP_Enabled_bl == "0")
      {
        Serial.println("__________Network DHCP Disable info__________");
        DHCP_Enabled_bl = "0";
        DHCP_IP_str =  String(Network_DHCP.DHCP_IP);
        DHCP_Subnet_str =  String(Network_DHCP.DHCP_Subnet);
        DHCP_Gateway_str =  String(Network_DHCP.DHCP_Gateway);
        Serial.println("Static IP: " + DHCP_IP_str);
        Serial.println("Subnet IP: " + DHCP_Subnet_str);
        Serial.println("Gateway IP: " + DHCP_Gateway_str);
      }

      ssidList_str =  String(Wifi_Network.ssidListname);
      Serial.println("Network SSID Name: " + ssidList_str);
      Wifi_Password_str =  String(Wifi_Network.Wifi_Password);
      Serial.println("Network SSID Password: " + Wifi_Password_str);


      WiFi.begin(ssidList_str, Wifi_Password_str);

      Serial.println("Connecting to existing WiFi network...");
      startTime = millis(); // Record the start time

      while (WiFi.status() != WL_CONNECTED && millis() - startTime < connectionTimeout) {
        digitalWrite(D3, LOW);
        delay(1000);
        digitalWrite(D3, HIGH);
        delay(1000);
        digitalWrite(D3, HIGH);
        Serial.println("Connecting to WiFi...");
        Serial.println("Wifi Status: " + getStatusString(WiFi.status()));
        if (WiFi.status() == WL_CONNECTED)
        {
          Soft_IP = WiFi.localIP();
          WiFi.setAutoReconnect(true);
          WiFi.persistent(true);
          Serial.println("Network IP Address: " + Soft_IP.toString());
          Serial.println("Access Point started");
          Serial.println("IP Address:");
          Serial.println(WiFi.softAPIP());
        }
        else
        {
          Serial.println("Failed to connect to WiFi network. Starting as access point only.");
        }
      }
      wifiStatus = WiFi.status();
    }
    else
    {
      WiFi.mode(WIFI_AP_STA);
      Serial.println("WIFI MODE: WIFI_AP_STA");
      Serial.println("WiFi Not Enabled ");
      WiFi_Enabled_bl == "0";
      ssidList_str =  "";
      Serial.println(ssidList_str);
      Wifi_Password_str =  "";
      Serial.println(Wifi_Password_str);
    }

    Serial.print("Enable WiFi_Enabled_bl: ");
    Serial.println(WiFi_Enabled_bl);
    Serial.println("__________Server & Port Setting info__________");

    Server_Address_str = String(server_setting_API.Server_Address);
    Serial.println("Server Address: " + Server_Address_str);
    Server_Port_str = String(server_setting_API.Server_Port);
    Serial.println("Server Port: " + Server_Port_str);



    Serial.println("__________Device offset Setting__________");
    DeviceType = Device_Setting.sensor_Type;
    if (DeviceType == "01")
    {
      RhOffset = Device_Setting.rhoffset;
      TempOffset = Device_Setting.tempoffset;
      IsDisplay = Device_Setting.isDisplay;
      Serial.println("Device Type:" + DeviceType );
      Serial.println("Rh Offset:" + String(RhOffset));
      Serial.println("Temp Offset:" + String(TempOffset));
      Serial.println("Display:" + String(IsDisplay));

    }
    if (DeviceType == "02")
    {
      RhOffset = 0.0;
      TempOffset = Device_Setting.tempoffset;
      IsDisplay = Device_Setting.isDisplay;
      Serial.println("Device Type:" + DeviceType );
      Serial.println("Temp Offset:" + String(TempOffset));
      Serial.println("Display:" + String(IsDisplay));
    }
  }

  server.on("/", onhandelpage);
  server.on("/getTime", handleGetTime);
  server.on("/login", onhandelpageresultpage);
  server.on("/Status", HTTP_GET, handlePage1);
  server.on("/Network", HTTP_GET, handlePageGetNetwork);
  server.on("/Network", handlePageNetwork);
  server.on("/User", HTTP_GET, handlePageGetUser);
  server.on("/User", handlePagePostUser);
  server.on("/Setting", HTTP_GET, handlePageGetRTCDateTime);
  server.on("/Setting", handlePagePostRTCDateTime);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/file", HTTP_GET, handleFile);
  server.on("/Restart", HTTP_GET, handlePageGetRestart);
  server.on("/Restart", handlePagePostRestart);
  server.begin();

  RhOffset = Device_Setting.rhoffset;
  TempOffset = Device_Setting.tempoffset;
  IsDisplay = Device_Setting.isDisplay;

  digitalWrite(D3, LOW);
  delay(1000);
  digitalWrite(D3, HIGH);
  delay(1000);
  digitalWrite(D3, LOW);
  delay(1000);
  digitalWrite(D3, HIGH);
  delay(1000);
  digitalWrite(D3, LOW);



  //Get Mac Address OR Device ID
  macAddressstr = getMACAddress();
  //Get Sensor ID


  delay(1000);
  digitalWrite(D3, HIGH);

  String Sensor_ID = GetSensorID();
  if (Sensor_ID == "E01")
  {
    digitalWrite(D3, LOW);
    delay(2000);
    digitalWrite(D3, HIGH);
    delay(2000);
    digitalWrite(D3, LOW);
    SensorID = "ID Not Found";
  }
  else {
    SensorID = Sensor_ID;
  }

  delay(1000);
  digitalWrite(D3, LOW);
  delay(1000);
  digitalWrite(D3, HIGH);

  while (!aht.begin())
  {
    digitalWrite(D3, HIGH);
    Serial.println("Could not find AHT? Check wiring");
    Serial.println("API Method for Sensor not connected.");
  }
  delay(1000);
  digitalWrite(D3, LOW);

  readAddress = 0;
  writeAddress = 0;
  dataFlag = false;
  overwrite = false;

  // Check free memory
  checkFreeMemory();
}


void saveDataConfig() {


  File file = LittleFS.open(ConfigFile, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }


  //Login Information save.

  login_user.flag = true;
  strncpy(login_user.id, id_str.c_str(), sizeof(login_user.id) );
  strncpy(login_user.password, password_str.c_str(), sizeof(login_user.password) );

  //SoftAP Information save.
  strncpy(Soft_AP.Soft_SSID_Name, Soft_SSID_Name_str.c_str(), sizeof(Soft_AP.Soft_SSID_Name) );
  strncpy(Soft_AP.Soft_Password, Soft_Password_str.c_str(), sizeof(Soft_AP.Soft_Password) );


  //Wifi SSID Information save.
  strncpy(Wifi_Network.WiFi_Enabled, WiFi_Enabled_bl.c_str(), sizeof(Wifi_Network.WiFi_Enabled) );
  strncpy(Wifi_Network.ssidListname, ssidList_str.c_str(), sizeof(Wifi_Network.ssidListname) );
  strncpy(Wifi_Network.Wifi_Password, Wifi_Password_str.c_str(), sizeof(Wifi_Network.Wifi_Password) );


  //DHCP Information save.
  strncpy(Network_DHCP.DHCP_Enabled, DHCP_Enabled_bl.c_str(), sizeof(Network_DHCP.DHCP_Enabled) );
  strncpy(Network_DHCP.DHCP_IP, DHCP_IP_str.c_str(), sizeof(Network_DHCP.DHCP_IP) );
  strncpy(Network_DHCP.DHCP_Subnet, DHCP_Subnet_str.c_str(), sizeof(Network_DHCP.DHCP_Subnet) );
  strncpy(Network_DHCP.DHCP_Gateway, DHCP_Gateway_str.c_str(), sizeof(Network_DHCP.DHCP_Gateway) );

  //API Server Information save.
  strncpy(server_setting_API.Server_Address, Server_Address_str.c_str(), sizeof(server_setting_API.Server_Address) );
  strncpy(server_setting_API.Server_Port, Server_Port_str.c_str(), sizeof(server_setting_API.Server_Port) );


  //Device Setting Information save.
  strncpy(Device_Setting.sensor_Type, DeviceType.c_str(), sizeof(Device_Setting.sensor_Type) );
  Device_Setting.rhoffset = RhOffset;
  Device_Setting.tempoffset = TempOffset;
  Device_Setting.isDisplay = IsDisplay;


  file.write((const uint8_t*)&login_user, sizeof(login_user));
  file.write((const uint8_t*)&Soft_AP, sizeof(Soft_AP));
  file.write((const uint8_t*)&Wifi_Network, sizeof(Wifi_Network));
  file.write((const uint8_t*)&Network_DHCP, sizeof(Network_DHCP));
  file.write((const uint8_t*)&server_setting_API, sizeof(server_setting_API));
  file.write((const uint8_t*)&Device_Setting, sizeof(Device_Setting));

  file.close();
  Serial.println("Data saved");
}

void readDataConfig() {
  File file = LittleFS.open(ConfigFile, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  file.read((uint8_t*)&login_user, sizeof(login_user));
  file.read((uint8_t*)&Soft_AP, sizeof(Soft_AP));
  file.read((uint8_t*)&Wifi_Network, sizeof(Wifi_Network));
  file.read((uint8_t*)&Network_DHCP, sizeof(Network_DHCP));
  file.read((uint8_t*)&server_setting_API, sizeof(server_setting_API));
  file.read((uint8_t*)&Device_Setting, sizeof(Device_Setting));
  file.close();
  // Print the read data
  Serial.println("Saved Data:");
  Serial.print("Login Flag: ");
  Serial.println(login_user.flag);
  Serial.print("Login User ID: ");
  Serial.println(login_user.id);
  Serial.print("Login User Password: ");
  Serial.println(login_user.password);

  Serial.print("Soft AP SSID: ");
  Serial.println(Soft_AP.Soft_SSID_Name);
  Serial.print("Soft AP Password: ");
  Serial.println(Soft_AP.Soft_Password);

  Serial.print("WiFi Enabled: ");
  Serial.println(Wifi_Network.WiFi_Enabled);
  Serial.print("WiFi SSID List: ");
  Serial.println(Wifi_Network.ssidListname);
  Serial.print("WiFi Password: ");
  Serial.println(Wifi_Network.Wifi_Password);

  Serial.print("Network DHCP Enabled: ");
  Serial.println(Network_DHCP.DHCP_Enabled);
  Serial.print("Network DHCP IP: ");
  Serial.println(Network_DHCP.DHCP_IP);
  Serial.print("Network DHCP Subnet: ");
  Serial.println(Network_DHCP.DHCP_Subnet);
  Serial.print("Network DHCP Gateway: ");
  Serial.println(Network_DHCP.DHCP_Gateway);

  Serial.print("Server Address: ");
  Serial.println(server_setting_API.Server_Address);
  Serial.print("Server Port: ");
  Serial.println(server_setting_API.Server_Port);

  Serial.print("Device Sensor Type: ");
  Serial.println(Device_Setting.sensor_Type);
  Serial.print("Device Rhoffset: ");
  Serial.println(Device_Setting.rhoffset);
  Serial.print("Device Tempoffset: ");
  Serial.println(Device_Setting.tempoffset);
  Serial.print("Device Is Display: ");
  Serial.println(Device_Setting.isDisplay);
}

void saveDataMemory() {
  File file = LittleFS.open(DataFile, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }


  MemoryAddressObj.ReadAddress = readAddress;
  MemoryAddressObj.WriteAddress = writeAddress;
  MemoryAddressObj.Overwrite = overwrite;
  MemoryAddressObj.DataFlag = dataFlag;
  file.write((const uint8_t*)&MemoryAddressObj, sizeof(MemoryAddressObj));


  file.close();
  Serial.println("Data saved");
}

void readDataMemory() {
  File file = LittleFS.open(DataFile, "r");
  if (!file) {
    Serial.println("Failed to open file for Read");
    return;
  }


  file.read((uint8_t*)&MemoryAddressObj, sizeof(MemoryAddressObj));

  readAddress = MemoryAddressObj.ReadAddress ;
  writeAddress = MemoryAddressObj.WriteAddress;
  overwrite = MemoryAddressObj.Overwrite;
  dataFlag = MemoryAddressObj.DataFlag ;

  Serial.println("Read Data:");
  Serial.print("ReadAddress: ");
  Serial.println(String(readAddress));
  Serial.print("WriteAddress: ");
  Serial.println(writeAddress);
  Serial.print("Overwrite: ");
  Serial.println(overwrite);
  Serial.print("DataFlag: ");
  Serial.println(dataFlag);

  file.close();
  Serial.println("Data Read");
}

String getStatusString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "Idle";
    case WL_NO_SSID_AVAIL:
      return "No SSID Available";
    case WL_SCAN_COMPLETED:
      return "Scan Completed";
    case WL_CONNECTED:
      return "Connected";
    case WL_CONNECT_FAILED:
      return "Connection Failed";
    case WL_CONNECTION_LOST:
      return "Connection Lost";
    case WL_DISCONNECTED:
      return "Disconnected";
    default:
      return "Unknown Status";
  }
}

void onhandelpage()  {
  server.send(200, "text/html", loginpage);
}

void onhandelpageresultpage()  {


  if (server.method() == HTTP_POST) {
    String userid = server.arg("userid");
    String password = server.arg("password");
    Serial.print("User ID: ");
    Serial.println(String(login_user.id));
    Serial.print("Password: ");
    Serial.println(String(login_user.password));
    if (id_str == userid && password_str == password)
    {
      Serial.println("Login Success");

      Status = "<!DOCTYPE html> <html> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <style> body { margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; background-color: #f5f5f5; } .topnav { overflow: hidden; background-color: #333; } .topnav a { float: left; color: #f2f2f2; text-align: center; padding: 14px 16px; text-decoration: none; font-size: 17px; } .topnav a:hover { background-color: #64485c; color: black; } .topnav a.active { background-color: #501b1d; color: white; } .form-signin { width: 100%; max-width: 650px; padding: 15px; margin: auto; } button { cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #007bff; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .3rem; } .textstyle { border: 1px solid transparent; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .4rem; width: 90%; } </style> </head> <main class='form-signin'> <div class=\"topnav\"> <a class=\"active\" href='/Status'>Status</a> <a href='/Network'>Network</a> <a href='/User'>User</a>  <a href='/Setting'>Setting</a> </div> <br> <br> <form class=\"form-signin\" action='/Status' method='POST'> <label class='form-floating' for=\"macAddress\">MAC Address:</label><br> <input class='textstyle' type=\"text\" id=\"macAddress\" name=\"macAddress\" disabled value=\"" + macAddressstr + "\"><br /> <label for=\"macAddress\">Station IP Address:</label><br> <input class='textstyle' disabled id=\"STIPAD\" name=\"STIPAD\" value=\"" + Soft_IP.toString() + "\"><br> <label for=\"macAddress\">WiFi Status:</label><br> <input class='textstyle' disabled type=\"text\" id=\"wifistatus\" name=\"wifistatus\" value=\"" + getStatusString(wifiStatus) + "\"><br> <label for=\"macAddress\">Sensor ID:</label><br> <input type=\"text\" class='textstyle' disabled id=\"SFIPAD\" name=\"SFIPAD\" value=\"" + SensorID + "\"> <label for=\"macAddress\">Date Time:</label><br> <input type=\"text\" disabled class='textstyle'  id=\"DateTimeES\" name=\"DateTimeES\" value=\"\"> <br /> <br /></form> </main> <script> function updateTime() { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.getElementById(\"DateTimeES\").value = this.responseText; } }; xhttp.open(\"GET\", \"/getTime\", true); xhttp.send(); }  setInterval(updateTime, 800);</script> </body> </html>";


      server.sendHeader("Location", "/Status");
      server.send(302, "text/plain", Status);

      //writeFile("/Config.txt",  GetDateTime() + " User Login: " + userid);
      //server.send(200, "text/html", html);
      Serial.println("Write Config");

    } else
    {
      //      writeFile("/Config.txt", "Date Time: " + GetDateTime() + " User Login failed: " + userid +  '\r' + '\n');
      server.send(400, "text/plain", "Invalid user and password");
    }
  }
}

void handleGetTime() {
  String dateTime = GetDateTime();
  server.send(200, "text/plain", dateTime);
}

void handlePage1()  {
  Serial.println("handlePage1");
  server.send(200, "text/html", Status);
}

void handlePageGetRTCDateTime()  {
  uint8_t _sec, _min, _hour, _day, _month;
  uint16_t _year;

  //get time from RTC
  rtc.get(&_sec, &_min, &_hour, &_day, &_month, &_year);

  char buffer[8]; // Make sure the buffer is large enough to hold the formatted string
  sprintf(buffer, "%04d-%02d-%02d", _year, _month, _day);
  buffer[4];
  String date = String(buffer);
  Serial.println("Date Time: " + date);
  sprintf(buffer, "%02d:%02d", _hour, _min);
  String Time1 = String(buffer);
  Serial.println("Date Time: " + Time1);
  String html = "<!DOCTYPE html> <html> <head> <meta name=\" viewport\" content=\"width=device-width, initial-scale=1\"> <style> body { margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; background-color: #f5f5f5; } .topnav { overflow: hidden; background-color: #333; } .topnav a { float: left; color: #f2f2f2; text-align: center; padding: 14px 16px; text-decoration: none; font-size: 17px; } .topnav a:hover { background-color: #64485c; color: black; } .topnav a.active { background-color: #501b1d; color: white; } .form-signin { width: 100%; max-width: 650px; padding: 15px; margin: auto; } button { cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #007bff; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .3rem; } .textstyle { border: 1px solid transparent; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .4rem; width: 90%; } h2 { color: #501b1d; } .label-lg { font-size: 18px; line-height: 1.33333; } .input-lg { border: 1px solid transparent; border-color: #007bff; height: 46px; width: 95%; font-size: 18px; line-height: 1.33333; border-radius: 6px; padding: 10px 16px; } </style> </head> <main class='form-signin'> <div class=\"topnav\"> <a href='/Status'>Status</a> <a href='/Network'>Network</a> <a href='/User'>User</a> <a class=\"active\" href='/Setting'>Setting</a> </div> <br> <form class=\"form-signin\" action='/Setting' method='POST'> <h2 class=''>Date Time</h2><br /> <div class='form-floating'><label>Date Time:</label> <input type=\"date\" id=\"date\" name=\"date\"> <input type=\"time\" id=\"time\" name=\"time\"> <div> <h2>Sensor Type</h2> </div> <div id='Sensor_group' align='right'> <label class='label-lg'> <input name='snrtype' id='S1' value=01 type='radio' />Humidity</label> <input name='snrtype' id='S2' value=02 type='radio' />Temperature </label> </div> <div id=\"snrtype\"> <label for=\"network\">Humidity Offset:</label> <br> <div id=\"Rhofset\"><input type=\"text\" class='textstyle' id=\"Rhoffset\" name=\"Humidityoffset\" value=\"" + String(RhOffset) + "\"> </div> <label for=\"network\">Temperature offset</label> <br> <div id=\"Tempofset\"> <input type=\"text\" class='textstyle' id=\"Tempoffset\" name=\"Temperatureoffset\" value=\"" + String(TempOffset) + "\"> </div> <br> <div> <h2>Logging</h2> </div> </div> <div id='Display_group' align='left'> <label class='label-lg'> <input name='displayEnb' id='DEnable' value=1 type='radio' />Enable</label> <input name='displayEnb' id='DDisable' value=0 type='radio' />Disable </label> </div> <di> <h4>Passcode</h4> <input type=\"text\" class='textstyle' id=\"passcode\" name=\"Temperatureoffset\"> <span id=\"datetime\"></span> </di> </div> </div> <br /> <br /> <button type='submit'>Save</button> <script> const now = new Date(); const currentDateTime = now.toLocaleString(); document.querySelector('#datetime').textContent = currentDateTime; const dateTimeStr = currentDateTime; const dateObj = new Date(dateTimeStr); const unixTimestamp = Math.floor(dateObj.getTime() / 1000); console.log(`The Unix timestamp for ${dateTimeStr} is: ${unixTimestamp}`); document.addEventListener('DOMContentLoaded', function () { var offsetradioValueToSet = \"" + DeviceType + "\"; var displayradiovalueToSet = \"" + String(IsDisplay) + "\"; document.getElementById('date').value = \"" + date + "\";document.getElementById('time').value =\"" + Time1 + "\";var S1radio = document.getElementById('S1'); var S2radio = document.getElementById('S2'); if (S1radio.value === offsetradioValueToSet) { S1radio.checked = true; var div = document.getElementById('Rhofset'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; var div = document.getElementById('Tempofset'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; } if (S2radio.value === offsetradioValueToSet) { S1radio.checked = false; S2radio.checked = true; var div = document.getElementById('Rhofset'); div.style.pointerEvents = 'none'; div.style.opacity = '0.5'; var div = document.getElementById('Tempofset'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; }; var DEradio = document.getElementById('DEnable'); var DDradio = document.getElementById('DDisable'); if (DEradio.value === displayradiovalueToSet) { DEradio.checked = true; } if (DDradio.value === displayradiovalueToSet) { DDradio.checked = true; } }); var S1Radio = document.getElementById('S1'); var S2Radio = document.getElementById('S2'); var Rhgroup = document.getElementById('Rhofset'); var Tempgroup = document.getElementById('Tempofset'); S1Radio.addEventListener('change', function () { if (S1Radio.checked) { var div = document.getElementById('Rhofset'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; var div = document.getElementById('Tempofset'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; } }); S2Radio.addEventListener('change', function () { if (S2Radio.checked) { var div = document.getElementById('Rhofset'); div.style.pointerEvents = 'none'; div.style.opacity = '0.5'; var div = document.getElementById('Tempofset'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; } }); document.querySelector('form').addEventListener('submit', function (e) { var pass = document.getElementById(\"passcode\").value; console.log(unixTimestamp); console.log(pass); if (unixTimestamp != pass) { alert(\"Passcode is not valid!\"); e.preventDefault(); } console.log(\"forward\"); }); </script> </form> </main> </body> </html>";


  server.send(200, "text/html", html);
}

void handlePageGetRestart() {
  String html = "<!DOCTYPE html> <html> <head> <meta name=\" viewport\" content=\"width=device-width, initial-scale=1\"> <style> body { margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; background-color: #f5f5f5; } .topnav { overflow: hidden; background-color: #333; } .topnav a { float: left; color: #f2f2f2; text-align: center; padding: 14px 16px; text-decoration: none; font-size: 17px; } .topnav a:hover { background-color: #64485c; color: black; } .topnav a.active { background-color: #501b1d; color: white; } .form-signin { width: 100%; max-width: 650px; padding: 15px; margin: auto; } button { cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #007bff; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .3rem; } .textstyle { border: 1px solid transparent; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .4rem; width: 90%; } h2 { color: #501b1d; } .label-lg { font-size: 18px; line-height: 1.33333; } .input-lg { border: 1px solid transparent; border-color: #007bff; height: 46px; width: 95%; font-size: 18px; line-height: 1.33333; border-radius: 6px; padding: 10px 16px; } </style> </head> <main class='form-signin' > <br> <form class=\"form-signin\" action='/Restart' method='POST'> <h1 class=''>Restart</h1><br /> <h4>The settings will be effected after the module is restarted.</h4> <br /> <br /> <button type='submit'>Restart</button> </form> </main> </body> </html>";
  server.send(200, "text/html", html);
}

void handlePagePostRestart() {
  if (server.method() == HTTP_POST)
  {
    server.send(200, "text/html", loginpage);
    // writeFile("/Config.txt", "Date Time: " + GetDateTime() + "Restart Device" + '\r' + '\n');
    delay(5000);
    ESP.restart();
  }
}

void handlePagePostRTCDateTime()  {
  if (server.method() == HTTP_POST)
  {
    String dateString = server.arg("date");
    Serial.print("Date: ");
    Serial.println(dateString);
    String timeString = server.arg("time");
    Serial.print("Time: ");
    Serial.println(timeString);
    SetDateTime(dateString, timeString);

    String SensorType = server.arg("snrtype");
    Serial.println("Sensor Type:" + SensorType );
    DeviceType = SensorType;

    if (SensorType == "01" ) {
      String humoffset = server.arg("Humidityoffset");
      RhOffset = humoffset.toFloat();
      String temoffset = server.arg("Temperatureoffset");
      TempOffset = temoffset.toFloat();

      readAddress = 0;
      writeAddress = 0;
      dataFlag = false;
      overwrite = false;
      saveDataMemory();
    }
    if (SensorType == "02") {
      RhOffset = 0.0;
      String temoffset = server.arg("Temperatureoffset");
      TempOffset = temoffset .toFloat();

      readAddress = 0;
      writeAddress = 0;
      dataFlag = false;
      overwrite = false;
      saveDataMemory();

    }
    String isDispaly = server.arg("displayEnb");
    if (isDispaly == "0") {
      IsDisplay = false;

    }
    else
    {
      IsDisplay = true;
    }

    strncpy(Device_Setting.sensor_Type, DeviceType.c_str(), sizeof(Device_Setting.sensor_Type) );
    Device_Setting.rhoffset = RhOffset;
    Device_Setting.tempoffset = TempOffset;
    Device_Setting.isDisplay = IsDisplay;

    Serial.println("LittleFS mount pass");
    saveDataConfig();

    handlePageGetRestart();
  }
}

void handlePageGetUser()  {
  String html = "<!doctype html> <html lang='en'> <head> <meta charset='utf-8'> <meta name='viewport' content='width=device-width, initial-scale=1'> <title>Wifi Setup</title> <style> body { margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; background-color: #f5f5f5; } .topnav { overflow: hidden; background-color: #333; } .topnav a { float: left; color: #f2f2f2; text-align: center; padding: 14px 16px; text-decoration: none; font-size: 17px; } .topnav a:hover { background-color: #64485c; color: black; } .topnav a.active { background-color: #501b1d; color: white; } .form-signin { width: 100%; max-width: 650px; padding: 15px; margin: auto; } button { cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #007bff; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .3rem; } .textstyle { border: 1px solid transparent; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .4rem; width: 90%; } </style> </head> <body> <main class='form-signin'> <div class=\"topnav\"> <a href='/Status'>Status</a> <a href='/Network'>Network</a> <a class=\"active\" href='/User'>User</a>  <a href='/Setting'>Setting</a></div> <br> <br> <form class=\"active\" action='/User' method='post'> <h1 class=''>User Setting</h1><br /> <div class='form-floating'><label>User ID</label><input disabled class=\"textstyle\" type='text' value=\"admin\" class='form-control' name='userid'> </div> <div class='form-floating'><br /><label>Password</label><input class=\"textstyle\" id=\"paw\" type='password' class='form-control' name='password'> </div> <div class='form-floating'><br /><label>Confirm Password</label><input class=\"textstyle\" id=\"cpaw\" type='password' class='form-control' name='confim-password'> </div> <br /> <br /> <button type='submit'>Save</button> </form> </main> <script>document.querySelector('form').addEventListener('submit', function (e) { var userName = document.getElementById(\"paw\").value; var userEmail = document.getElementById(\"cpaw\").value; console.log(userEmail); console.log(userName); if (userName != userEmail) { alert(\"Passsword dit not match!\"); e.preventDefault(); } console.log(\"forward\"); });</script> </body> </html>";
  server.send(200, "text/html", html);
}

void handlePagePostUser()  {
  if (server.method() == HTTP_POST)
  {
    String userid_temp = "admin";
    Serial.print("userid_temp: ");
    Serial.println(userid_temp);
    String Password_temp = server.arg("password");
    Serial.print("Password_temp: ");
    Serial.println(Password_temp);
    String confim_password_temp = server.arg("confim-password");
    Serial.print("confim_password_temp: ");
    Serial.println(confim_password_temp);

    if (id_str != userid_temp || password_str != Password_temp)
    {
      id_str = userid_temp;
      password_str = Password_temp;
      strncpy(login_user.id, id_str.c_str(), sizeof(login_user.id) );
      strncpy(login_user.password, password_str.c_str(), sizeof(login_user.password) );

      saveDataConfig();
      Serial.println("LittleFS mount pass");

      handlePageGetRestart();
    }
  }

}

void handlePageGetNetwork()  {

  // Start the Wi-Fi scan
  ssidList = GetWifiList();

  String html = "<!DOCTYPE html> <html> <head> <meta name=\" viewport\" content=\"width=device-width, initial-scale=1\"> <style> body { margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; background-color: #f5f5f5; } .topnav { overflow: hidden; background-color: #333; } .topnav a { float: left; color: #f2f2f2; text-align: center; padding: 14px 16px; text-decoration: none; font-size: 17px; } .topnav a:hover { background-color: #64485c; color: black; } .topnav a.active { background-color: #501b1d; color: white; } .form-signin { width: 100%; max-width: 650px; padding: 15px; margin: auto; } button { cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #007bff; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .3rem; } .textstyle { border: 1px solid transparent; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .4rem; width: 90%; } h2{ color: #501b1d; } .label-lg { font-size: 18px; line-height: 1.33333; } .input-lg { border: 1px solid transparent; border-color: #007bff; height: 46px; width: 95%; font-size: 18px; line-height: 1.33333; border-radius: 6px; padding: 10px 16px; }</style> </head> <main class='form-signin'> <div class=\"topnav\"> <a href='/Status'>Status</a> <a class=\"active\"  href='/Network'>Network</a> <a href='/User'>User</a>  <a href='/Setting'>Setting</a></div> <br> <form class=\"form-signin active\" action='/Network' method='POST'> <div> <h2>SOFT AP</h2> </div> <label class='form-floating' for=\"network\">AP Name:</label> <br><input class='textstyle' type=\"text\" id=\"network\" name=\"Soft_SSID_Name\" value=\"" + Soft_SSID_Name_str + "\"><br /> <label for=\"network\">AP Password:</label> <br><input type='password' class='textstyle' id=\"STIPAD\" name=\"Soft_Password\" value=\"" + Soft_Password_str + "\"><br> <label for=\"network\">Soft IP:</label> <br><input class='textstyle' id=\"STIPAD\" name=\"Soft_IP\" value=\"" + WiFi.softAPIP().toString() + "\"><br> <label for=\"network\">Soft Subnet:</label> <br><input class='textstyle' id=\"STIPAD\" name=\"Soft_Subnet\" value=\"" + Soft_Subnet.toString() + "\"><br>  <label for=\"network\">Soft Gateway:</label> <br><input class='textstyle' id=\"STIPAD\" name=\"Soft_Gateway\" value=\"" + Soft_Gateway.toString() + "\"><br> <div> <h2>WiFi Router Setting</h2> </div> <div id='staEgroup' align='right'> <label class='label-lg'> <input name='staE' id='staE1' value=1 type='radio' />Enable <input name='staE' id='staE0' value=0 type='radio' />Disable </label> </div> <div id=\"networkgroup\"><label for=\"network\">SSD:</label> <br> <input type=\"text\" class='textstyle' id=\"SSD_Name\" name=\"SSD_Name\" value=\"" + ssidList_str + "\"> <br> <label for=\"network\">SSID List:</label> <br> <div> <select name='sta_list' class='form-control input-lg' id='sta_list'>" +  ssidList  + "</select> </div> <label for=\"network\">Password:</label> <br> <input type=\"password\" class='textstyle' id=\"SFIPAD\" name=\"Wifi_Password\" value=\"" + Wifi_Password_str + "\"> </div> <div> <h2>DHCP Setting</h2> </div><div id='DHCP_group' align='right'> <label class='label-lg'> <input name='dhcpg' id='dhcpE1' value=1 type='radio' />Enable <input name='dhcpg' id='dhcpE0' value=0 type='radio' />Disable </label> </div><div id=\"dhcpgroup\"> <label for=\"network\">DHCP IP:</label> <br> <input class='textstyle' type=\"text\" id=\"wifistatus\" name=\"DHCP_IP\" value=\"" + DHCP_IP_str + "\"> <br> <label for=\"network\">DHCP Subnet:</label> <br> <input type=\"text\" class='textstyle' id=\"SFIPAD\" name=\"DHCP_Subnet\" value=\"" + DHCP_Subnet_str + "\"> <label for=\"network\">DHCP Gateway:</label> <br> <input type=\"text\" class='textstyle' id=\"SFIPAD\" name=\"DHCP_Gateway\" value=\"" + DHCP_Gateway_str + "\"> </div><div> <h2>Server Setting</h2> </div> <label for=\"network\">Server Address:</label> <br> <input class='textstyle' type=\"text\" id=\"wifistatus\" name=\"Server_Address\" value=\"" + Server_Address_str + "\"> <br> <label for=\"network\">Server Port:</label> <br> <input type=\"text\" class='textstyle' id=\"SFIPAD\" name=\"Server_Port\" value=\"" + Server_Port_str + "\"> <br /> <br /> <button type='submit'>Save</button> <br /> <br /> </form> </main> <script> document.addEventListener('DOMContentLoaded', function () { var radioValueToSet = \"" + WiFi_Enabled_bl + "\"; var enableradioButton = document.getElementById('staE1'); var disableradioButton = document.getElementById('staE0'); if (enableradioButton.value === radioValueToSet) { enableradioButton.checked = true; var div = document.getElementById('networkgroup'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; }; if (disableradioButton.value === radioValueToSet) { disableradioButton.checked = true; var div = document.getElementById('networkgroup'); div.style.pointerEvents = 'none'; div.style.opacity = '0.5'; }; var dhcpradioValueToSet = \"" + DHCP_Enabled_bl + "\"; var enableradioButton = document.getElementById('dhcpE1'); var disableradioButton = document.getElementById('dhcpE0'); if (enableradioButton.value === dhcpradioValueToSet) { enableradioButton.checked = true; var div = document.getElementById('dhcpgroup'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; }; if (disableradioButton.value === dhcpradioValueToSet) { disableradioButton.checked = true; var div = document.getElementById('dhcpgroup'); div.style.pointerEvents = 'none'; div.style.opacity = '0.5'; }; });const selectElement = document.getElementById('sta_list'); selectElement.addEventListener('change', (event) => { const result = document.getElementById('SSD_Name'); result.value = event.target.value; }); var enableRadio = document.getElementById('staE1'); var disableRadio = document.getElementById('staE0'); var networkGroup = document.getElementById('networkgroup'); enableRadio.addEventListener('change', function () { if (enableRadio.checked) { var div = document.getElementById('networkgroup'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; } }); disableRadio.addEventListener('change', function () { if (disableRadio.checked) { var div = document.getElementById('networkgroup'); div.style.pointerEvents = 'none'; div.style.opacity = '0.5'; } }); var dhcpenableRadio = document.getElementById('dhcpE1'); var dhcpdisableRadio = document.getElementById('dhcpE0'); var dhcpdhcpgroup = document.getElementById('dhcpgroup'); dhcpenableRadio.addEventListener('change', function () { if (dhcpenableRadio.checked) { var div = document.getElementById('dhcpgroup'); div.style.pointerEvents = 'auto'; div.style.opacity = '1'; } }); dhcpdisableRadio.addEventListener('change', function () { if (dhcpdisableRadio.checked) { var div = document.getElementById('dhcpgroup'); div.style.pointerEvents = 'none'; div.style.opacity = '0.5'; } }); </script></body> </html>";

  server.send(200, "text/html", html);
}

void handlePageNetwork()  {

  if (server.method() == HTTP_POST)
  {
    //EEPROM.begin(EEPROM_size);
    //-------------------SOFT AP Setting--------------
    String Soft_SSID_Name_temp = server.arg("Soft_SSID_Name");
    Serial.print("Soft_SSID_Name_temp: ");
    Serial.println(Soft_SSID_Name_temp);
    String Soft_Password_temp = server.arg("Soft_Password");
    Serial.print("Soft_Password_temp: ");
    Serial.println(Soft_Password_temp);


    if ( Soft_SSID_Name_str != Soft_SSID_Name_temp || Soft_Password_str != Soft_Password_temp)
    {
      Soft_SSID_Name_str = Soft_SSID_Name_temp;
      Soft_Password_str = Soft_Password_temp;


      strncpy(Soft_AP.Soft_SSID_Name, server.arg("Soft_SSID_Name").c_str(), sizeof(Soft_AP.Soft_SSID_Name) );
      strncpy(Soft_AP.Soft_Password, server.arg("Soft_Password").c_str(), sizeof(Soft_AP.Soft_Password) );


      // Save WiFi Network settings to EEPROM


      Serial.print("Print Char Soft_SSID_Name: ");
      Serial.println(Soft_AP.Soft_SSID_Name);

      Serial.print("Print Char Soft_Password: ");
      Serial.println(Soft_AP.Soft_Password);
    }

    String Soft_IP_temp = server.arg("Soft_IP");
    Serial.print("Soft_IP_temp: ");
    Serial.println(Soft_IP_temp);
    String Soft_Subnet_temp = server.arg("Soft_Subnet");
    Serial.print("Soft_Subnet_temp: ");
    Serial.println(Soft_Subnet_temp);
    String Soft_Gateway_temp = server.arg("Soft_Gateway");
    Serial.print("Soft_Gateway_temp: ");
    Serial.println(Soft_Gateway_temp);
    if (Soft_IP_str != Soft_IP_temp || Soft_Subnet_str != Soft_Subnet_temp || Soft_Gateway_str != Soft_Gateway_temp)
    {
      Soft_IP_str = Soft_IP_temp ;
      Soft_Subnet_str = Soft_Subnet_temp ;
      Soft_Gateway_str = Soft_Gateway_temp;
    }

    //-------------------WiFi Network Setting--------------
    String optionbuttion = server.arg("staE");
    Serial.print("optionbuttion: ");
    Serial.println(optionbuttion);

    String ssidList_temp = server.arg("sta_list");
    Serial.print("ssidList_temp: ");
    Serial.println(ssidList_temp);
    String Wifi_Password_temp = server.arg("Wifi_Password");
    Serial.print("Wifi_Password_temp: ");
    Serial.println(Wifi_Password_temp);


    if (optionbuttion == "1")
    {
      if (WiFi_Enabled_bl != optionbuttion || ssidList_temp != ssidList_str || Wifi_Password_temp != Wifi_Password_str)
      {
        Serial.println("Enabled Network");
        ssidList_str = ssidList_temp;
        Wifi_Password_str = Wifi_Password_temp;
        WiFi_Enabled_bl = optionbuttion;
        strncpy(Wifi_Network.WiFi_Enabled, server.arg("staE").c_str(), sizeof(Wifi_Network.WiFi_Enabled) );
        strncpy(Wifi_Network.ssidListname, server.arg("SSD_Name").c_str(), sizeof(Wifi_Network.ssidListname) );
        strncpy(Wifi_Network.Wifi_Password, server.arg("Wifi_Password").c_str(), sizeof(Wifi_Network.Wifi_Password) );



        // Save WiFi Network settings to EEPROM


        Serial.print("Print Char Wifi_Network.ssidListname: ");
        Serial.println(Wifi_Network.ssidListname);

        Serial.print("Print Char Wifi_Network.Wifi_Password: ");
        Serial.println(Wifi_Network.Wifi_Password);
      }
    }
    if (optionbuttion == "0")
    {
      Serial.println("Disabled Network");
      WiFi_Enabled_bl = optionbuttion;
      strncpy(Wifi_Network.WiFi_Enabled, "0", sizeof(Wifi_Network.WiFi_Enabled) );
      strncpy(Wifi_Network.ssidListname, "", sizeof(Wifi_Network.ssidListname) );
      strncpy(Wifi_Network.Wifi_Password, "", sizeof(Wifi_Network.Wifi_Password) );


      // Save WiFi Network settings to EEPROM


      Serial.print("Wifi_Network.ssidListname: ");
      Serial.println(Wifi_Network.ssidListname);

      Serial.print("Wifi_Network.Wifi_Password: ");
      Serial.println(Wifi_Network.Wifi_Password);
    }


    //-------------------DHCP Setting--------------
    String DHCP_Enabled_bl_temp = server.arg("dhcpg");
    String DHCP_IP_temp = server.arg("DHCP_IP");
    String DHCP_Subnet_temp = server.arg("DHCP_Subnet");
    String DHCP_Gateway_temp = server.arg("DHCP_Gateway");

    Serial.println("DHCP Enable: " + DHCP_Enabled_bl_temp);
    Serial.println("Static IP: " + DHCP_IP_temp);
    Serial.println("Static Subnet: " + DHCP_Subnet_temp);
    Serial.println("Static Gateway: " + DHCP_Gateway_temp);



    if (DHCP_Enabled_bl_temp == "1")
    {
      DHCP_Enabled_bl = "1";

      Serial.print("DHCP_Enabled_bl : ");
      Serial.println(DHCP_Enabled_bl );


      Serial.println("Static IP: " + DHCP_IP_str);
      Serial.println("Static Subnet: " + DHCP_Subnet_str);
      Serial.println("Static Gateway: " + DHCP_Gateway_str);

      if (DHCP_IP_str != DHCP_IP_temp || DHCP_Subnet_str != DHCP_Subnet_temp || DHCP_Gateway_str != DHCP_Gateway_temp )
      {

        DHCP_IP_str = DHCP_IP_temp ;
        DHCP_Subnet_str = DHCP_Subnet_temp ;
        DHCP_Gateway_str = DHCP_Gateway_temp;


        Serial.println("Static IP: " + DHCP_IP_str);
        Serial.println("Static Subnet: " + DHCP_Subnet_str);
        Serial.println("Static Gateway: " + DHCP_Gateway_str);


        strncpy(Network_DHCP.DHCP_Enabled, DHCP_Enabled_bl.c_str(), sizeof(Network_DHCP.DHCP_Enabled));
        strncpy(Network_DHCP.DHCP_IP, DHCP_IP_str.c_str(), sizeof(Network_DHCP.DHCP_IP));
        strncpy(Network_DHCP.DHCP_Subnet, DHCP_Subnet_str.c_str(), sizeof(Network_DHCP.DHCP_Subnet) );
        strncpy(Network_DHCP.DHCP_Gateway, DHCP_Gateway_str.c_str(), sizeof(Network_DHCP.DHCP_Gateway) );



      }
    }
    if (DHCP_Enabled_bl_temp == "0")
    {
      DHCP_Enabled_bl = "0";
      Serial.print("DHCP_Enabled_bl_temp 4: ");
      Serial.println(DHCP_Enabled_bl_temp);

      Serial.print("DHCP_Enabled_bl : ");
      Serial.println(DHCP_Enabled_bl );


      strncpy(Network_DHCP.DHCP_Enabled, server.arg("dhcpg").c_str(), sizeof(Network_DHCP.DHCP_Enabled));

    }


    //-------------------Web API Server--------------
    String Server_Address_temp = server.arg("Server_Address");
    Serial.print("Server_Address_temp: ");
    Serial.println(Server_Address_temp);

    String Server_Port_temp = server.arg("Server_Port");
    Serial.print("Server_Port_temp: ");
    Serial.println(Server_Port_temp);

    if ( Server_Address_temp != server_setting_API.Server_Address || Server_Port_temp != server_setting_API.Server_Port)
    {
      Server_Address_str = Server_Address_temp;
      Server_Port_str = Server_Port_temp;

      // Update WiFi settings in memory

      strncpy(server_setting_API.Server_Address, server.arg("Server_Address").c_str(), sizeof(server_setting_API.Server_Address) );
      strncpy(server_setting_API.Server_Port, server.arg("Server_Port").c_str(), sizeof(server_setting_API.Server_Port) );

      Serial.print("Print Char Soft_AP.Server_Address: ");
      Serial.println(server_setting_API.Server_Address);

      Serial.print("Print Char Soft_AP.Server_Port: ");
      Serial.println(server_setting_API.Server_Port);
      Serial.print("EEPROM Address: ");

      Serial.print("EEPROM Address: ");
    }
    saveDataConfig();
    Serial.println("Network HTTP POST");

    handlePageGetRestart();
  }
}

void handleDownload() {

  File file = LittleFS.open(ConfigFile, "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(file, "application/octet-stream");
  file.close();
}

void handleFile() {
  String fileContent;
  File file = LittleFS.open(ConfigFile, "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  size_t fileSize = file.size();;
  Serial.printf("Size of /Config.txt: %d bytes\n", fileSize);

  char buffer[64];
  while (file.available()) {
    size_t bytesRead = file.readBytes(buffer, 64);
    fileContent += String(buffer).substring(0, bytesRead);
    // Print free heap after reading each chunk
    //Serial.printf("Free heap after reading chunk: %d bytes\n", ESP.getFreeHeap());
  }
  file.close();
  //    // Send the file content as the response
  server.send(200, "text/plain", fileContent);
  //  } else {
  //    // If the file doesn't exist, send a 404 response
  //    server.send(404, "text/plain", "File not found");
  //  }
}


const unsigned long softAPCheckInterval = 1000;   // Check SoftAP every 1 second
const unsigned long serverRequestInterval = 1000; // Send server request every 10 seconds. 10sec = 10 * 1000;
//const unsigned long SaveDataInterval = 59999; // Send server request every 1 Minute seconds. 10sec = 10 * 1000;
const unsigned long SaveDataInterval = 1000; // Send server request every 1 Minute seconds. 10sec = 10 * 1000;

static unsigned long lastSoftAPCheckTime = 0;
static unsigned long lastServerRequestTime = 0;
static unsigned long lastSaveDataRequestTime = 0;
byte Recorder = 0;
// the loop function runs over and over again until power down or reset
WiFiClient client;
HTTPClient http;
String content;
unsigned long currentTime;

void loop() {

  server.handleClient();
  new_stations = WiFi.softAPgetStationNum();

  currentTime = millis();

  // Set the CS pin as an output
  pinMode(FLASH_CS, OUTPUT);
  digitalWrite(FLASH_CS, HIGH);


  if (current_stations < new_stations) //Device is Connected
  {
    current_stations = new_stations;
    Serial.print("New Device Connected to SoftAP... Total Connections: ");
    Serial.println(current_stations);
  }

  if (current_stations > new_stations) //Device is Disconnected
  {
    current_stations = new_stations;
    Serial.print("Device disconnected from SoftAP... Total Connections: ");
    Serial.println(current_stations);
  }

  if (currentTime - lastServerRequestTime >= serverRequestInterval  && IsDisplay) {
    Serial.println("===START====");

    sensorValue = analogRead(analogInPin);
    String date_Time = GetDateTime();
    String power_status = GetPowerStatus();
    if (DeviceType == "01")
    {
      while (!aht.begin())
      {
        digitalWrite(D3, !(digitalRead(D3))); //Toggle LED Pin                                                                                                                                                                                                    .println("Could not find AHT? Check wiring");
        Serial.println("API Method for Sensor not connected.");
        delay(1000);
      }

      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data


      if (WiFi.status() == WL_CONNECTED )
      {
        Serial.println("Wifi Connected.");
        int serverPort = String(server_setting_API.Server_Port).toInt();
        if (!client.connected())
        {
          if (client.connect(String(server_setting_API.Server_Address), serverPort))
          {
            Serial.println("Connected to server");
          }
        }
        Serial.println("Client connected.");
      }
      else
      {
        Serial.println("Wifi not Connect.");
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
      }
      Serial.println("Data Flag: " + String(dataFlag));

      if (WiFi.status() == WL_CONNECTED && client.connected())
      {
        if (dataFlag == 1)
        {
          if (ReadAllData() != 0)
          {
            Serial.println("Read Data from Flash");

            DataPacket_Struct data1;
            data1.TEMP = temp.temperature;
            data1.HUM = humidity.relative_humidity;
            strcpy(data1.DT, date_Time.c_str());
            strcpy(data1.BS , power_status.c_str());
            data1.CV = sensorValue;
            SaveDataMemory(data1);
            Serial.println("Save Data in Flash");
            return;
          }
        }
        if (dataFlag == 0)
        {
          String DataPacket = GetRhDataPacketTCP(humidity.relative_humidity, temp.temperature, sensorValue, date_Time, power_status);
          int status_code = Post_TCPSocket(DataPacket);
          Serial.println("Status Code:" + String(status_code));

          if (status_code == 200)
          {
            Serial.println("===Receivied OK====");
          }
          else {
            client.stop();
            DataPacket_Struct data1;
            data1.TEMP = temp.temperature;
            data1.HUM = humidity.relative_humidity;
            strcpy(data1.DT, date_Time.c_str());
            strcpy(data1.BS , power_status.c_str());
            data1.CV = sensorValue;
            SaveDataMemory(data1);
            Serial.println("Save Data in Flash");
          }
        }
      }
      else
      {
        DataPacket_Struct data1;
        data1.TEMP = temp.temperature;
        data1.HUM = humidity.relative_humidity;
        strcpy(data1.DT, date_Time.c_str());
        strcpy(data1.BS , power_status.c_str());
        data1.CV = sensorValue;
        SaveDataMemory(data1);
        Serial.println("Save Data in Flash");
      }
    }
    if (DeviceType == "02")
    {

      DS18B20.requestTemperatures();
      float temperature_C = DS18B20.getTempCByIndex(0);
      Serial.print("Temperature: ");
      Serial.print(temperature_C);    // print the temperature in °C
      Serial.println("°C");

      if (WiFi.status() == WL_CONNECTED )
      {
        Serial.println("Wifi Connected.");
        int serverPort = String(server_setting_API.Server_Port).toInt();
        if (!client.connected())
        {
          if (client.connect(String(server_setting_API.Server_Address), serverPort))
          {
            Serial.println("Connected to server");
          }
        }
        Serial.println("Client connected.");
      }
      else
      {
        Serial.println("Wifi not Connect.");
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
      }

      Serial.println("Data Flag: " + String(dataFlag));
      if (WiFi.status() == WL_CONNECTED && client.connected())
      {
        if (dataFlag == 1)
        {
          if (ReadAllData() != 0)
          {
            Serial.println("Read Data from Flash");

            DataPacket_Struct data1;
            data1.TEMP = temperature_C;
            data1.HUM = 0;
            strcpy(data1.DT, date_Time.c_str());
            strcpy(data1.BS , power_status.c_str());
            data1.CV = sensorValue;
            SaveDataMemory(data1);
            Serial.println("Save Data in Flash");
            return;
          }
        }
        if (dataFlag == 0)
        {
          String DataPacket = GetTempDataPacketTCP(temperature_C, sensorValue, date_Time, power_status);
          int status_code = Post_TCPSocket(DataPacket);
          Serial.println("Status Code:" + String(status_code));

          if (status_code == 200)
          {
            Serial.println("===Receivied OK====");
          }
          else
          {
            DataPacket_Struct data1;
            data1.TEMP = temperature_C;
            data1.HUM = 0;
            strcpy(data1.DT, date_Time.c_str());
            strcpy(data1.BS , power_status.c_str());
            data1.CV = sensorValue;
            SaveDataMemory(data1);
            Serial.println("Save Data in Flash");

          }
        }
      }
      else
      {
        DataPacket_Struct data1;
        data1.TEMP = temperature_C;
        data1.HUM = 0;
        strcpy(data1.DT, date_Time.c_str());
        strcpy(data1.BS , power_status.c_str());
        data1.CV = sensorValue;
        SaveDataMemory(data1);
        Serial.println("Save Data in Flash");

      }
    }
    Serial.println("===END====");
    lastServerRequestTime = currentTime;
  }
}

void SetDateTime(String dateString, String timeString) {


  Serial.println(dateString);
  Serial.println(timeString);
  // Parse the date string to extract year, month, day, hour, minute, and second
  uint16_t _year = dateString.substring(0, 4).toInt();
  uint8_t _month = dateString.substring(5, 7).toInt();
  uint8_t _day = dateString.substring(8, 10).toInt();
  uint8_t _hour = timeString.substring(0, 2).toInt();
  Serial.println( timeString.substring(3, 5));
  uint8_t _minute = timeString.substring(3, 5).toInt();
  uint8_t _second = 0;

  Serial.println("DateTime");
  Serial.println(_year);
  Serial.println(_month);
  Serial.println(_day);
  Serial.println(_hour);
  Serial.println(_minute);

  Serial.println("DateTime END");

  // Create a DateTime object
  //  DateTime dt(year, month, day, hour, minute, seconda);
  //    rtc.begin();
  //    rtc.adjust(dt); // Set the RTC to the specified date and time

  rtc.begin();
  //only set the date+time one time
  rtc.set(_second, _minute, _hour, _day, _month, _year); //08:00:00 24.12.2014 //sec, min, hour, day, month, year
  // rtc.set(0, 2, 3, 13, 01, 2024); //08:00:00 24.12.2014 //sec, min, hour, day, month, year

  //start RTC
  rtc.start();
  Serial.println("Date and time set successfully!");
}

String GetDateTime() {
  String dateTime = "";
  uint8_t _sec, _min, _hour, _day, _month;
  uint16_t _year;

  //get time from RTC
  rtc.get(&_sec, &_min, &_hour, &_day, &_month, &_year);
  _year = _year - 2000;
  char buffer[18]; // Make sure the buffer is large enough to hold the formatted string
  sprintf(buffer, "%02d-%02d-%02d %02d:%02d:%02d", _day, _month, _year, _hour, _min, _sec);

  dateTime = String(buffer);
  return dateTime;
}

void LittleFSDeleteManual() {

  if (LittleFS.exists(ConfigFile)) {
    String message = "";
    if (LittleFS.remove(ConfigFile)) {
      message = "File deleted successfully!" + '\n';
    } else {
      message = message + "Error deleting file!" + '\r' + '\n';
    }
    server.send(200, "text/plain", message);
  }

}

String getMACAddress() {
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  // Convert MAC address to a string

  for (int i = 0; i < 6; ++i) {
    macAddressstr += String(mac[i], HEX);
  }
  Serial.println(macAddressstr);
  macAddressstr.toUpperCase();
  return macAddressstr;
}

String GetSensorID() {
  String _sensorID  = "";

  DeviceAddress tempDeviceAddress;
  if (DS18B20.getAddress(tempDeviceAddress, 0))
  {
    Serial.print("18B20 device Found");
    for (uint8_t i = 0; i < 8; i++) {
      if (tempDeviceAddress[i] < 16) {
        _sensorID += String(0, HEX);
      }
      _sensorID += String(tempDeviceAddress[i], HEX);
      _sensorID.toUpperCase();
    }
    Serial.println(_sensorID);
  }
  else {
    Serial.print("18B20 not Found");
    _sensorID  = "E01";
  }
  return _sensorID;
}

String GetWifiList() {
  String _ssidList = "";
  // Start the Wi-Fi scan
  int numNetworks = WiFi.scanNetworks();
  Serial.print("Found ");
  Serial.print(numNetworks);
  Serial.println(" networks");


  for (int i = 0; i < numNetworks; ++i) {
    String ssid = WiFi.SSID(i);
    Serial.println(ssid);
    _ssidList += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
    Serial.print("Signal strength (dBm): ");
    Serial.println(WiFi.RSSI(i));
  }
  _ssidList += "]";

  return _ssidList;

}

String GetDataPacket() {

  sensorValue = analogRead(analogInPin) - 7;
  voltage = (((sensorValue * 3.3) / 1024) * 2 + calibration); //multiply by two as voltage divider network is 100K & 100K Resistor
  bat_percentage = mapFloat(voltage, 2.8, 4.2, 0, 100); //2.8V as Battery Cut off Voltage & 4.2V as Maximum Voltage
  //bat_percentage = 70;
  if (bat_percentage >= 100)
  {
    bat_percentage = 100;
  }
  if (bat_percentage <= 0)
  {
    bat_percentage = 1;
  }

  Serial.print("Analog Value = ");
  Serial.println(sensorValue);
  Serial.print("Output Voltage = ");
  Serial.println(voltage);
  Serial.print("Battery Percentage = ");
  Serial.println(bat_percentage);
  Serial.println();
  Serial.println("****************************");
  Serial.println();


  if (!aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    Serial.println("API Method for Sensor not connected.");
    while (1) delay(10);
  }

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data

  String _Temperature = String(temp.temperature);
  String _Humidity = String(humidity.relative_humidity);
  String power_status = GetPowerStatus();
  //String httpRequestData = "\""+ macAddressstr+"#"+ SensorID + "#"  + GetDateTime() + "#01#" +_Temperature + "#" +_Humidity + "#" + bat_percentage + "?\"";
  String httpRequestData =  macAddressstr + "#" + SensorID + "#"  + GetDateTime() + "#01#" + _Temperature + "#" + _Humidity + "#" + power_status + "#" + bat_percentage + "?";
  return httpRequestData;
}

String GetRhDataPacketTCP(float humidity, float temp, int sensorValue, String Date_Time, String power_status) {

  humidity = humidity + RhOffset;
  temp = temp + TempOffset;


  Serial.print("Analoge Value = ");
  Serial.println(sensorValue);

  voltage = (((sensorValue * 2.2) / 682) * 2 + calibration); //multiply by two as voltage divider network is 100K & 100K Resistor
  Serial.print("Output Voltage = ");
  Serial.println(voltage);
  int tempInteger = (int)temp ;  // Integer part
  int tempFractional = (int)((temp - tempInteger) * 10); // Fractional part, multiplied by 100 for two decimal places

  int humidityInteger = (int)humidity;  // Integer part
  int humidityFractional = (int)((humidity - humidityInteger) * 10); // Fractional part, multiplied by 100 for two decimal places


  Serial.print("Temperature: ");
  Serial.println(temp);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Date Time: ");
  Serial.println(Date_Time);




  tempInteger = tempInteger * 2;

  String hexString = DeviceType + SensorID + String(humidityInteger, HEX) + String(tempInteger, HEX) ;


  // Convert the hex string to a byte array
  int len = hexString.length() / 2;

  uint8_t data[len];
  for (int i = 0; i < len; i++) {
    sscanf(hexString.substring(2 * i, 2 * i + 2).c_str(), "%4hhx", &data[i]);
  }



  uint16_t crc = calculateCRC16(data, len);

  Serial.print("CRC Value:");

  Serial.println(String(crc));

  Serial.print("CRC HEX Value:");

  Serial.println(String(crc, HEX));

  String crchex = String(crc, HEX);

  char hexCRC[4]; // 8 characters for 4-byte hex + null terminator

  // Convert the integer to a 4-byte hexadecimal string with leading zeros
  sprintf(hexCRC, "%04X", crc);




  hexString = hexString + hexCRC + "F1";
  String vol = String(voltage).substring(0, 3);
  // Print 4-byte hexadecimal value
  String Datapacket = "#" + macAddressstr + "#" + hexString + "#" + Date_Time + "#" + power_status + "#" + vol + "#" + String(tempFractional) + "#" + String(humidityFractional) + "??";
  Serial.print("Data:");



  Serial.println(Datapacket);
  return Datapacket;
}

String GetTempDataPacketTCP(float temp, int sensorValue, String Date_Time, String power_status) {

  temp = temp + TempOffset;
  int temponly = temp * 16;
  Serial.println("Temperature only = " + String(temp));

  Serial.print("Analoge Value = ");
  Serial.println(sensorValue);

  voltage = (((sensorValue * 2.2) / 682) * 2 + calibration); //multiply by two as voltage divider network is 100K & 100K Resistor
  Serial.print("Output Voltage = ");
  Serial.println(voltage);
  Serial.print("Date Time: ");
  Serial.println(Date_Time);


  // Create a buffer to hold the 4-byte hexadecimal string
  char hexTemp[4]; // 8 characters for 4-byte hex + null terminator

  // Convert the integer to a 4-byte hexadecimal string with leading zeros
  sprintf(hexTemp, "%04X", temponly);
  Serial.println("Temp Value:" + String(temponly));
  Serial.println("Hex Temp:" + String(hexTemp));
  String hexString = "02" + SensorID + hexTemp;

  int len = hexString.length() / 2;

  uint8_t data[len];
  for (int i = 0; i < len; i++) {
    sscanf(hexString.substring(2 * i, 2 * i + 2).c_str(), "%4hhx", &data[i]);
  }

  //
  uint16_t crc = calculateCRC16(data, len);

  Serial.print("CRC Value:");

  Serial.println(String(crc));

  Serial.print("CRC HEX Value:");

  Serial.println(String(crc, HEX));

  String crchex = String(crc, HEX);



  char hexCRC[4]; // 8 characters for 4-byte hex + null terminator

  // Convert the integer to a 4-byte hexadecimal string with leading zeros
  sprintf(hexCRC, "%04X", crc);

  hexString = hexString + hexCRC + "F1";
  String vol = String(voltage).substring(0, 3);
  // Print 4-byte hexadecimal value
  String Datapacket = "#" + macAddressstr + "#" + hexString + "#" + Date_Time + "#" + power_status + "#" + vol + "#0#0??";
  Serial.print("Data:");
  Serial.println(Datapacket);
  return Datapacket;
}

int ReadAllData() {
  Serial.println("Call Read Method");

  int pointer = 0;
  int temp_pointer = 0;
  if (overwrite == false)
  {
    pointer = (writeAddress) -  readAddress;
  }
  if (overwrite == true)
  {
    pointer =  (writeAddress + Max_Sector) - readAddress ;
  }

  temp_pointer = pointer;

  if (pointer > 128)
  {
    pointer = 128;
  }

  Serial.println("Write Pointer: " + String(writeAddress));
  Serial.println("Read Pointer: " + String( readAddress));
  Serial.println("Pointer Pointer: " + String(pointer));
  digitalWrite(D3, !(digitalRead(D3))); //Toggle LED Pin

  for (size_t i = 0; i < pointer ; i++)
  {
    DataPacket_Struct records;
    Serial.println("Read Record No.:" + String(readAddress));
    Serial.println("Read (i) value Record No.:" + String(i));
    readStructFromFlash( readAddress * sizeof(DataPacket_Struct), records);
    String DataPacket = "";
    int status_code = 0;
    if (DeviceType == "01") {
      String DataPacket = GetRhDataPacketTCP(records.HUM, records.TEMP, records.CV, records.DT, records.BS);
      status_code = Post_TCPSocket(DataPacket);
    }
    if (DeviceType == "02") {
      String DataPacket = GetTempDataPacketTCP(records.TEMP, records.CV, records.DT, records.BS);

      status_code = Post_TCPSocket(DataPacket);
    }
    Serial.println("Status Code:" + String(status_code));
    if (status_code == 200)
    {
      readAddress++;
      temp_pointer--;
      Serial.println("temp_pointer No.:" + String(temp_pointer));

    }
    if (status_code == 13)
    {
      client.stop();
      break;
    }
  }
  Serial.println("------------------Read Data Loop End.------------------");


  Serial.println("temp_pointer:" + String(temp_pointer));
  if (temp_pointer == 0)
  {
    dataFlag = 0;
    writeAddress = 0;
    readAddress = 0;
    Serial.println("------------------Data upload completed.------------------");
  }

  Serial.println("Read Record No.:" + String(readAddress));
  Serial.println("Max Record No.:" + String(Max_Sector));
  if (readAddress >= Max_Sector)
  {
    readAddress  = 0;
    overwrite = false;
    Serial.println("------------------Read Data Start from 0 location.------------------");
  }

  saveDataMemory();
  return temp_pointer;
}

void SaveDataMemory(const DataPacket_Struct& data) {

  //unsigned long currentTime1 = millis();
  Serial.println("Write Pointer: " + String(writeAddress));
  Serial.println("Read Pointer: " + String(readAddress));

  digitalWrite(D3, HIGH);
  if (currentTime - lastSaveDataRequestTime >= SaveDataInterval)
  {
    int mode_result = (writeAddress * 32) % 4096;
    Serial.println("Save Date Time: " + GetDateTime());
    Serial.println("Mode: " + String(mode_result));

    if (overwrite == false)
    {
      Serial.println("if");
      if (mode_result == 0)
      {
        sectorErase(writeAddress * 32);
        Serial.println("Sector Erase");
      }
      writeStruct(writeAddress * sizeof(DataPacket_Struct), data);
      Serial.println("Data Write in memory");

      Serial.println("Write Record No.:" + String(writeAddress));
      Serial.println("Max Record No.:" + String(Max_Sector));

      writeAddress++;

      if (writeAddress == Max_Sector)
      {
        writeAddress = 0;
        overwrite = true;
        Serial.println("Over Write Flag: " + String(overwrite));
      }
      Serial.println("No. Records: " + String(writeAddress));

    }
    else
    {
      Serial.println("else");

      if (mode_result == 0)
      {
        int x = writeAddress + 128;
        if (readAddress >= writeAddress && readAddress <= x)
        {
          readAddress = x;
          Serial.println("Read address Jump on this location:" + String(readAddress));
          if (readAddress >= Max_Sector)
          {
            readAddress = 0;
            overwrite = false;
            Serial.println("Read address Jump on this 0 location:" + String(readAddress));
          }
        }
        sectorErase(writeAddress * 32);
        Serial.println("Sector Erase");
      }
      writeStruct(writeAddress * sizeof(DataPacket_Struct), data);
      Serial.println("Overwrite flag set:" + String(overwrite));

      Serial.println("Data Write in memory");
      writeAddress++;
    }

    Serial.println("Saving data");
    Serial.println("Saving Read address:" + String(readAddress));
    Serial.println("Saving Write address:" + String(writeAddress));

    dataFlag  = 1;
    saveDataMemory();
    lastSaveDataRequestTime = currentTime;
  }
}

int counterResthttpcode400 = 0;

String GetPowerStatus() {

  int powerStatus = digitalRead(powerCheckPin);
  Serial.println("Power Status" + String(powerStatus));
  if (powerStatus == HIGH) {
    return "A";
  } else {
    return "B";
  }
}

int Post_DataWeb(String httpRequestData)  {
  Serial.println("Post Method Call");
  ESP.wdtFeed();
  httpRequestData = "\"" + httpRequestData + "\"";
  ESP.wdtFeed();
  String serverName = "http://" + String(server_setting_API.Server_Address) + ":"  + String(server_setting_API.Server_Port) + "/api/GetLiveData";
  Serial.println("Server Address: " + serverName);
  //String serverName = "http://192.168.2.42:45457/api/GetLiveData";
  String contentlenght = String(httpRequestData.length());

  if (httpRequestData.length() > 0)
  {

    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      //        if (client.connect(String(server_setting_API.Server_Address), 45457)) {
      //          Serial.println("Server is online");
      //          //client.stop(); // Close the connection
      //        } else {
      //          Serial.println("Server is offline");
      //          return -12; // or any other code to indicate server offline
      //        }

      http.begin(client, serverName.c_str());
      // Your Domain name with URL path or IP address with path
      //http.begin(client, serverName.c_str());
      //http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Content-Length", contentlenght);

      Serial.println(httpRequestData);
      digitalWrite(D3, HIGH);
      int httpResponseCode = http.POST(httpRequestData);
      int counter = 0;
      Serial.println("Request sent " + String(httpResponseCode));

      // Wait for the server's response without using delay
      unsigned long startTime = millis();
      unsigned long timeout = 30000;  // Adjust the timeout as needed

      while (http.connected() && millis() - startTime < timeout && httpResponseCode != 200) {
        Serial.println("Time1 Count: " + String(millis()));
        //delay(1000); // You can adjust the delay if needed
        Serial.println("Post Count: " + String(counter));
        Serial.println("Time2 Count: " + String(millis()));
        counter++;
        if (httpResponseCode == 200)
        {
          http.end();
          client.stop();
          Serial.println("Request sent " + String(httpResponseCode));
          digitalWrite(D3, LOW);
          break;
        }
        if (httpResponseCode == 404)
        {
          //Check data length.
          Serial.println("Data Length:" + contentlenght);
          if (httpRequestData.length() == 0)
          {
            content = "";
          }
          break;
        }
      }
      digitalWrite(D3, LOW);
      // Check for a completed request
      if (http.connected() && httpResponseCode != 200) {
        Serial.println("Server response timeout");
        http.end();
        client.stop();
        return -13; // or any other code to indicate timeout
      }

      http.end();
      client.stop();
      switch (httpResponseCode)
      {
        case 200:
          return httpResponseCode;
          break;
        case 400:
          if ( counterResthttpcode400 == 20)
          {
            counterResthttpcode400 = 0;
            ESP.restart();
          }
          counterResthttpcode400++;
          return httpResponseCode;
          break;
      }
      return httpResponseCode;
    }
    else {
      Serial.println("WiFi Disconnected");
      WiFi.reconnect();
      return -10;
    }
  }
  return -11;
}

unsigned char Post_TCPSocket(String httpRequestData) {
  unsigned char Return_Status = 10;
  if (httpRequestData.length() > 0)
  {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      int serverPort = String(server_setting_API.Server_Port).toInt();
      if (!client.connected())
      {
        Serial.println("server not connected");

        if (client.connect(String(server_setting_API.Server_Address), serverPort))
        {
          Serial.println("Connected to server");

          //client.stop(); // Close the connection
        }
        else {
          Serial.println("Server is offline");
          return Return_Status = 12; // or any other code to indicate server offline
        }
      }
      else
      {
        Serial.println("Server is Connected");
      }
      client.println(httpRequestData);

      digitalWrite(D3, HIGH);
      unsigned long timeout1 = millis();
      while (client.available() == 0) {
        if (millis() - timeout1 > 5000) {
          Serial.println(">>> Client Timeout !");
          return 13;
        }
      }
      // Wait for the server's response without using delay
      unsigned long startTime = millis();
      unsigned long timeout = 30000;  // Adjust the timeout as needed
      //while (client.available() && millis() - startTime < timeout)
      while (millis() - startTime < timeout)
      {
        Serial.println("Client Staus: " + String(client.available()));
        Serial.println("Respose Wait Time: " + GetDateTime());
        String response = client.readStringUntil('?');
        Serial.print("Server response: ");
        Serial.println(response);
        digitalWrite(D3, LOW);
        if (response.indexOf("OK") > 0)
        {
          Return_Status = 200;
          break;
        }
      }
      // Check for a completed request
      return Return_Status;
    }
    else {
      Serial.println("WiFi Disconnected");
      WiFi.reconnect();
      return Return_Status = 11;
    }
  }
  return Return_Status;
}

void sectorErase(uint32_t addr) {
  // Enable writing
  writeEnable();

  // Send Sector Erase command
  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(0x20); // Sector Erase (4KB) command
  SPI.transfer((addr >> 16) & 0xFF); // Address MSB
  SPI.transfer((addr >> 8) & 0xFF);  // Address Mid
  SPI.transfer(addr & 0xFF);         // Address LSB
  digitalWrite(FLASH_CS, HIGH);

  // Wait for the erase to complete
  delay(100); // Adjust delay based on flash memory datasheet specifications
}

bool stringToIP(const String &str, IPAddress &ip) {
  int parts[4] = {0, 0, 0, 0};
  int partIndex = 0;
  String part = "";

  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str[i];
    if (c == '.') {
      if (partIndex >= 3) {
        return false; // Too many parts
      }
      parts[partIndex++] = part.toInt();
      part = "";
    } else {
      part += c;
    }
  }
  parts[partIndex] = part.toInt();
  ip = IPAddress(parts[0], parts[1], parts[2], parts[3]);
  return partIndex == 3; // True if we got four parts (valid IP)
}

uint16_t calculateCRC16(uint8_t *data, int len) {
  uint16_t crc = 0xFFFF; // Initial value

  for (int i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i]; // XOR with data byte

    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001; // XOR with polynomial if LSB is 1
      } else {
        crc >>= 1; // Shift right if LSB is 0
      }
    }
  }

  return crc;
}

void writeStruct(uint32_t addr, const DataPacket_Struct& data) {
  Serial.println("Memory location Write:" + String(addr));
  // Enable writing
  writeEnable();

  // Convert struct to bytes
  uint8_t* ptr = (uint8_t*)&data;
  size_t size = sizeof(data);
  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(0x02); // Page Program command
  SPI.transfer((addr >> 16) & 0xFF); // Address MSB
  SPI.transfer((addr >> 8) & 0xFF);  // Address Mid
  SPI.transfer(addr & 0xFF);         // Address LSB
  for (size_t i = 0; i < size; i++) {
    SPI.transfer(ptr[i]); // Write each byte
    //Serial.println("Each byte:" + String(ptr[i]));
  }
  digitalWrite(FLASH_CS, HIGH);

  // Wait for the write to complete
  delay(10); // Adjust delay based on flash memory datasheet specifications
}

void readStructFromFlash(uint32_t addr, DataPacket_Struct& data) {
  // Read bytes into struct
  uint8_t* ptr = (uint8_t*)&data;
  size_t size = sizeof(data);
  for (size_t i = 0; i < size; i++) {
    ptr[i] = readByte(addr + i);
  }
}

uint8_t readByte(uint32_t addr) {
  uint8_t result;

  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(0x03); // Read Data command
  SPI.transfer((addr >> 16) & 0xFF); // Address MSB
  SPI.transfer((addr >> 8) & 0xFF);  // Address Mid
  SPI.transfer(addr & 0xFF);         // Address LSB
  result = SPI.transfer(0);          // Read the data byte
  digitalWrite(FLASH_CS, HIGH);

  return result;
}

void writeEnable() {
  digitalWrite(FLASH_CS, LOW);
  SPI.transfer(0x06); // Write Enable command
  digitalWrite(FLASH_CS, HIGH);
}

void checkFreeMemory() {
  size_t freeHeap = ESP.getFreeHeap();
  Serial.print("Free heap memory: ");
  Serial.print(freeHeap);
  Serial.println(" bytes");
}
