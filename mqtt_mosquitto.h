#include <ESP8266WiFi.h>
#define MQTT_MAX_PACKET_SIZE 455 //cambialo antes de incluir docpatth\Arduino\libraries\pubsubclient-master\src\pubsubclient.h
#define MQTT_KEEP_ALIVE 60
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v6.0.7
#include <Pin_NodeMCU.h>

/*************************************************
 ** -------- Valores Personalizados ----------- **
 * ***********************************************/
#define DEVICE_TYPE "ESP12E-Riego"
#define ORG "canMorras"
char* ssid;
char* password;
/*********** personal.h should include SSID and passwords  ***********
 *  something like: 
char ssid1[] = "ssid1";
char password1[] = "Password_ssid1";
char ssid2[] = "ssid2";
char password2[] = "Password_ssid2";
 */
#include "personal.h"  
/*************************************************
 ** ----- Fin de Valores Personalizados ------- **
 * ***********************************************/
#define ESPERA_NOCONEX 70000  // cuando no hay conexion, descanso 70 segundos
char server[] = "192.168.1.11";
char * authMethod = NULL;
char * token = NULL;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

char publishTopic[] = "meteo/envia";  // el dispositivo envia datos a Mosquitto
char metadataTopic[]= "meteo/envia/metadata"; //el dispositivo envia sus metadatos a Mosquitto











































































































char updateTopic[]  = "meteo/update";    // Mosquitto o node-red me actualiza los metadatos
char responseTopic[]= "meteo/response";
char rebootTopic[]  = "meteo/reboot";



