

#include "DHT.h"
#include <Phant.h>
#include <ESP8266WiFi.h>

////////////////
// Phant Keys //
////////////////
const char PhantHost[] = "data.sparkfun.com";
const char PublicKey[] = "SECRET";
const char PrivateKey[] = "SECRET";

/////////////////
// Post Timing //
/////////////////
const unsigned long postRate = 60000;
unsigned long lastPost = 0;
////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED


//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "SECRET";
const char WiFiPSK[] = "SECRET";

#define DHTPIN 2 // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321 [Alt.: 11 & 21]


DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  initHardware(); // Setup input/output I/O pins
  connectWiFi(); // Connect to WiFi
  digitalWrite(LED_PIN, LOW); // LED on to indicate connect success
}


void loop()
{
  // This conditional will execute every lastPost milliseconds
  // (assuming the Phant post succeeded).
  if ((lastPost + postRate <= millis()) || lastPost == 0)
  {
    Serial.println("Posting to Phant!");
    if (postToPhant())
    {
      lastPost = millis();
      Serial.println("Post Suceeded!");
    }
    else // If the Phant post failed
    {
      delay(500); // Short delay, then try again
      Serial.println("Post failed, will try again.");
    }
  }
}

void connectWiFi()
{
  byte ledStatus = LOW;
  Serial.println();
  Serial.println("Connecting to: " + String(WiFiSSID));
  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void initHardware()
{
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT); // Set LED as output
  digitalWrite(LED_PIN, HIGH); // LED off
  Serial.println("Starting dht");

  // Start dth
  dht.begin();

}

int postToPhant()
{
  // LED turns on when we enter, it'll go off when we
  // successfully post.
  digitalWrite(LED_PIN, LOW);

  // Read sensor data
  // Humidity
  float h = dht.readHumidity();
  // Temperature (Celsius; call with true for Fahrenheit)
  float t = dht.readTemperature();

  // Check if we could read values
  if (isnan(h) || isnan(t)) {
    Serial.print("Could not read data from DHT");
    digitalWrite(LED_PIN, HIGH);
    return 0;
  }

  // Declare an object from the Phant library - phant
  Phant phant(PhantHost, PublicKey, PrivateKey);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String postedID = "ThingDev-" + macID;

  // Add the four field/value pairs defined by our stream:
  phant.add("id", postedID);
  phant.add("analog", t);
  phant.add("digital", h);
  phant.add("time", millis());

  // Now connect to data.sparkfun.com, and post our data:
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(PhantHost, httpPort))
  {
    // If we fail to connect, return 0.
    return 0;
  }
  // If we successfully connected, print our Phant post:
  client.print(phant.post());

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line); // Trying to avoid using serial
  }

  // Before we exit, turn the LED off.
  digitalWrite(LED_PIN, HIGH);

  return 1; // Return success
}


