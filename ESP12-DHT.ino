/**
 ESP-12E  (ESP8266) manejado desde IBM IoT
 en funcion de la libreria (mqtt_Mosquitto o mqtt_Bluemix)
 manda los datos a un servidor mqtt u otro
 Morrastronics -- by chuRRuscat
 v1.0 2017 initial version
 v2.0 2018 mqtt & connectivity  functions separated
 v2.1 Feb 2019 migration to ArduinoJSON version 6
*/

#undef PRINT_SI
#define PRINT_SI
#ifdef PRINT_SI
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
  #define DPRINT(...)     //linea en blanco
  #define DPRINTLN(...)   //linea en blanco
#endif
/*************************************************
 ** -------- Valores Personalizados ----------- **
 * ***********************************************/
//#include "Pruebas.h"
#include "salon.h"
//#include "jardin.h"
//#include "terraza.h"
#include "mqtt_mosquitto.h"

#define AJUSTA_T 10000 // para ajustar las esperas que hay en algunos sitios
/*************************************************
 ** ----- Fin de Valores Personalizados ------- **
 * ***********************************************/
#include <Pin_NodeMCU.h> 
#define DHTPIN  D5      // PIN 5 (GPIO 14) Pin al que conecto el sensor DHT.
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
const int sensorPin = A0;  // PIN analogico de Sensor Suelo
#define CONTROL_HUMEDAD D2  // base del transistor que enciende y apaga el sensor de humedad
#include <DHT.h>
//#include <DHT_U.h>
#include <FS.h>

/* si no econtrara valores iniciales en SPIFFS, 
 *  estos son los que pondre
 */
#define FICHEROVALORES "/metadata"
#define JSONBUFFSIZE 250
#define DATOSJSONSIZE 250

// ********* Variables del sensor que expondremos ********** 
float temperatura,humedadAire,sensacion=20;
int  intervaloConex,humedadMin,humedadMax,humedadSuelo,humedadCrudo;
int humedadCrudo1,humedadCrudo2;

// otras variables

int umbralRiego, tRiego;
char datosJson[DATOSJSONSIZE];
DHT dht(DHTPIN, DHTTYPE);

void setup() {
boolean status;
  
 Serial.begin(115200);
 DPRINTLN("arranco");  
 pinMode(CONTROL_HUMEDAD,OUTPUT);
 
 dht.begin();
 SPIFFS.begin();
 cargaValores();  // carga los valores que estan en SPIFFS
 digitalWrite(CONTROL_HUMEDAD, HIGH);
 espera(1000);
 humedadCrudo1 = analogRead(sensorPin);
 espera(1000);
 humedadCrudo2 = analogRead(sensorPin);
 digitalWrite(CONTROL_HUMEDAD, LOW);
 wifiConnect();
 mqttConnect();
 delay(50);
 initManagedDevice(); 
 publicaDatos();
}

uint32_t ultima=0;

void loop() {
DPRINT("*");     
 if (!loopMQTT()) {  // leo si hay mensajes de IoT y si tengo conexion      
     DPRINTLN("He perdido la conexion,reconecto");     
     sinConectividad();        
     mqttConnect();
     initManagedDevice();  // Hay que suscribirse cada vez
 } 
 if ((millis()-ultima)>intervaloConex) {   
    DPRINT("intervalo:");DPRINT(intervaloConex);
    DPRINT("\tmillis :");DPRINT(millis());
    DPRINT("\tultima :");DPRINTLN(ultima);   
   publicaDatos();
   ultima=millis();
 }
 espera(1000);
}

boolean tomaDatos (){
  // Obtengo la temperatura y humedad
  float bufTemp,bufTemp1,bufHumedad,bufHumedad1;
  boolean escorrecto=true; 

  DPRINTLN("Leo de DHT");
  bufHumedad = dht.readHumidity();
  bufTemp = dht.readTemperature();
  DPRINT("\tTemperatura\t");DPRINTLN(bufTemp);
  DPRINT("\tHumedad aire\t");DPRINTLN(bufHumedad);
  digitalWrite(CONTROL_HUMEDAD, HIGH);
  espera(5000);
  DPRINTLN("Leo de Analog");
  humedadCrudo = analogRead(sensorPin);
  DPRINT("Humedad crudo 1:\t");DPRINTLN(humedadCrudo);
  humedadCrudo=constrain(humedadCrudo,humedadMin,humedadMax);
  DPRINT("Humedad crudo R:\t");DPRINTLN(humedadCrudo);
  digitalWrite(CONTROL_HUMEDAD, LOW);
  // hago la media móvil de humedad suelo de los últimos tres valores 
  humedadCrudo=(humedadCrudo1+humedadCrudo2+humedadCrudo)/3;
  DPRINT("Humedad crudo M:t");DPRINTLN(humedadCrudo);
  humedadCrudo2=humedadCrudo1;
  humedadCrudo1=humedadCrudo;
  // vuelvo a tomar humedad y temperatura
  bufHumedad1 = dht.readHumidity();
  bufTemp1 = dht.readTemperature();  
  DPRINTLN("tomo datos");
  DPRINT("\thumedadCrudo  ");DPRINTLN(humedadCrudo);
  DPRINT("\t humedad Min  ");DPRINTLN(humedadMin);
  DPRINT("\t humedad Max  ");DPRINTLN(humedadMax); 
  if (humedadMin==humedadMax) humedadMax+=1; 
  humedadSuelo = map(humedadCrudo,humedadMin,humedadMax,0,100);
  if (isnan(bufHumedad) || isnan(bufTemp) || isnan(bufHumedad1) || isnan(bufTemp1) ) {         
     DPRINTLN("no he podido leer del sensor DHT!");         
     escorrecto=false;
  } else {
    temperatura=(bufTemp+bufTemp1)/2;
    humedadAire=(bufHumedad+bufHumedad1)/2;
    if (temperatura>60) escorrecto=false;
    if ((humedadAire>101)||(humedadAire==0)) escorrecto=false;
    // Sensacion termica en Celsius (isFahreheit = false)
    //sensacion = dht.computeHeatIndex(temperatura, humedadAire, false);    
    DPRINT("\tTemperatura  ");DPRINT(temperatura);
    DPRINT("\tHumedad aire  ");DPRINT(humedadAire);
    DPRINT("\tSensacion   ");DPRINT(sensacion);
    DPRINT("\tHumedad  ");DPRINTLN(humedadSuelo);
    DPRINT("\tHumedad crudo ");DPRINTLN(humedadCrudo);    
  } 
  return escorrecto;
}

void publicaDatos() {
  int k=0;
  char signo;
  boolean pubresult=true;  

 DPRINTLN("Llamo a Tomadatos ");
  while(!tomaDatos()) {   
     espera(1000);
     if(k++>10) {     
        sprintf(datosJson,"{\"d\":{\"temp\":\"err\",\"hAire\":\"err\",\"hSuelo\":%d}}",
          (int) humedadSuelo);
      return; 
     }
  }
  // Preparo los datos en modo JSON.
  if (temperatura<0) {
    signo='-';
    temperatura*=-1;
  }  else signo=' ';    
  sprintf(datosJson,"[{\"temp\":%c%d.%1d,\"hAire\":%d,\"hSuelo\":%d,\"hCrudo\":%d},{\"deviceId\":\"%s\"}]",
        signo,(int)temperatura, (int)(temperatura * 10.0) % 10,\
        (int)humedadAire, (int) humedadSuelo,(int)humedadCrudo,DEVICE_ID);
  // Publica los datos.
  pubresult = enviaDatos(publishTopic,datosJson);
    DPRINT("Envio ");
    DPRINT(datosJson);
    DPRINT("a ");
    DPRINT(publishTopic);
    if (pubresult) 
      DPRINTLN("... OK envio conseguido");      
    else
      DPRINTLN(".....KO envio fallado");      
}

void cargaValores() {
  FSInfo fsInfo; 
  boolean resultado;
   StaticJsonBuffer<JSONBUFFSIZE> jsonBuffer;      
  
  /* si no habia nada, formateo, creo el fichero y pongo 
   *  los valores por defecto en el fichero "metadata"
   */
  if (!SPIFFS.info(fsInfo)) {    
    DPRINTLN(" formateo, y cargo valores por defecto");      
    SPIFFS.format();
    valoresPorDefecto();    
  } else { 
    if (SPIFFS.exists(FICHEROVALORES)) { // Si el fichero existe, cargo los valores
      leeValores(FICHEROVALORES,datosJson);
      DPRINT(" Parametros: "); DPRINTLN(datosJson);    
    } else {       
      DPRINTLN("NO EXISTE fichero de parametros");  // si no existe, grabo los valores por defecto
      valoresPorDefecto();
    }    
  }
  JsonObject& root = jsonBuffer.parseObject(datosJson);;
  if (!root.success()) {  
     DPRINTLN("CargaValores: FALLA PARSEADO DE LOS DATOS");
     return;
  }
  JsonObject& metadata = root["metadata"];
  intervaloConex=metadata.get<unsigned long>("intervaloConex");
  umbralRiego=metadata.get<int>("umbralRiego");
  tRiego=metadata.get<int>("tRiego");
  humedadMin=metadata.get<int>("humedadMin");
  humedadMax=metadata.get<int>("humedadMax");  
  DPRINTLN("***** Valores leidos de SPIFFS:");  
  DPRINT("intervaloConex:"); DPRINTLN(intervaloConex+AJUSTA_T);
  DPRINT("tRiego:"); DPRINTLN(tRiego);
  DPRINT("umbralRiego:"); DPRINTLN(umbralRiego);
  DPRINT("humedadMin:"); DPRINTLN(humedadMin);
  DPRINT("humedadMax:"); DPRINTLN(humedadMax);  
}

boolean leeValores(char * fichero, char * buff) {
  File ficheroValores;
  int tamanyo;
  
  ficheroValores= SPIFFS.open(fichero, "r+");
  if (ficheroValores) {   
    tamanyo=ficheroValores.size();
    ficheroValores.readBytes(buff,tamanyo);
    ficheroValores.close();
    buff[tamanyo]=0;
  }
  else {
    DPRINTLN("error al abrir el fichero de metadata para leer");
    return false;
  }
   return true;
}

boolean escribeValores(char * fichero, char buff[]) {
  File ficheroValores;

  ficheroValores= SPIFFS.open(fichero, "w");
  if (ficheroValores) {   
    ficheroValores.println(buff);
  }
  else {
    return false;
  }
  ficheroValores.close();
  DPRINTLN("Escribe valores en SPIFFS:");  
  DPRINT("intervaloConex:"); DPRINTLN(intervaloConex+AJUSTA_T);
  DPRINT("tRiego:"); DPRINTLN(tRiego);
  DPRINT("umbralRiego:"); DPRINTLN(umbralRiego);
  DPRINT("humedadMin:"); DPRINTLN(humedadMin);
  DPRINT("humedadMax:"); DPRINTLN(humedadMax);
  return true;
}

void valoresPorDefecto() {
  File ficheroValores;

  tRiego=TRIEGO;
  umbralRiego=UMBRALRIEGO;
  intervaloConex=INTERVALO_CONEX;
  humedadMin=HUMEDAD_MIN;  
  humedadMax=HUMEDAD_MAX;
  sprintf(datosJson,"{\"metadata\":{\"tRiego\":%d,\"intervaloConex\":%d,\"umbralRiego\":%d,\"humedadMin\":%d,\"humedadMax\":%d},\
           \"supports\": {\"deviceActions\": true,\"firmwareActions\": true} }",
         tRiego,intervaloConex,umbralRiego,humedadMin,humedadMax);
  escribeValores(FICHEROVALORES,datosJson); 
  DPRINT("Guardo por Defecto :");DPRINTLN(datosJson);
  DPRINT("valores por defecto write ");DPRINTLN(ficheroValores.println(datosJson));
}
