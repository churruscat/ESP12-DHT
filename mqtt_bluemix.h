#include <ESP8266WiFi.h>
#define MQTT_MAX_PACKET_SIZE 455 //cambialo antes de incluir docpatth\Arduino\libraries\pubsubclient-master\src\pubsubclient.h
#define MQTT_KEEP_ALIVE 60
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7
#include <Pin_NodeMCU.h>

/*************************************************
 ** -------- Valores Personalizados ----------- **
 * ***********************************************/
#define PRINT_SI
//#undef PRINT_SI
#define DEVICE_TYPE "ESP12E-Riego"
#define ORG "02m09d"
char* ssid;
char* password;
char ssid1[] = "M0VISTAR_CASA";
char password1[] = "Vivan1los2Morras";
char ssid2[] ="MOVISTAR_E1F2";
char password2[] = "Vivan1los2Morras";
/*************************************************
 ** ----- Fin de Valores Personalizados ------- **
 * ***********************************************/

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;
void funcallback(char* topic, byte* payload, unsigned int payloadLength);
WiFiClient wifiClient;
PubSubClient clienteMQTT(server, 1883, funcallback, wifiClient);

char publishTopic[] =  "iot-2/evt/status/fmt/json";
//const char mandatoTopic[] =  "iot-2/cmd/+/fmt/json";
char mandatoTopic[] =  "iot-2/cmd/mandato/fmt/json";
char responseTopic[]=  "iotdm-1/response";
char manage1Topic[] =  "iotdm-1/mgmt/#";
char updateTopic[] =   "iotdm-1/device/update";
char rebootTopic[] =   "iotdm-1/mgmt/initiate/device/reboot";
char manageTopic[] =   "iotdevice-1/mgmt/manage";


