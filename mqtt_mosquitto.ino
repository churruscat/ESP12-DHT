// hay que inluir mqtt_mosquitto.h en el programa principal
void funcallback(char* topic, byte* payload, unsigned int payloadLength);
WiFiClient wifiClient;
PubSubClient clienteMQTT(server, 1883, funcallback, wifiClient);

boolean wifiConnect() {
int i=0,j=0;  
  ssid=ssid1;
  password=password1;
 
  DPRINT("Conectando a WiFi  "); DPRINTLN(ssid);  
  WiFi.mode(WIFI_STA);  //Estacion, no AP ni mixto
  WiFi.disconnect();
  WiFi.begin(ssid,password);
 
  while ((WiFi.status() != WL_CONNECTED )) {
    espera(500);
    DPRINT(i++);
    DPRINT(".");   
    if (i>120) { 
      if (ssid==ssid1){
        ssid=ssid2;
        password=password2;
      } else {
        ssid=ssid1;
        password=password1;
      }
      i=0;
      j++;
      if (j>4) { return false;} /* no funciona ninguna */
     
      DPRINTLN();
      DPRINT("cambio de red num ");DPRINTLN(j);      
      DPRINT("Me conecto a "); DPRINTLN(ssid);
      WiFi.disconnect();
      espera(1000);
      WiFi.begin(ssid,password);      
    }
  }
 
 DPRINTLN(ssid);  DPRINT("*******Conectado; ADDR= ");
 DPRINTLN(WiFi.localIP());
 
 return true;
}

void sinConectividad(){
int j=0;

  clienteMQTT.disconnect(); 
  espera(500);
  while(!wifiConnect()) {   
  DPRINT("Sin conectividad, espero secs  ");DPRINTLN(int(intervaloConex/2000));
  espera(ESPERA_NOCONEX);
  }
}

void mqttConnect() {
 int j=0;
 
  if ((WiFi.status() == WL_CONNECTED )) {
   while (!clienteMQTT.connect(clientId, authMethod, token)) {      
     DPRINT(j);DPRINTLN("  Reintento conexion del MQTT client  ");
     j++;
     espera(2000);
     if (j>20) {
       sinConectividad();  
       j=0;
      }
     }
   } else {
    sinConectividad();   
  } 
}

boolean loopMQTT() {
return clienteMQTT.loop();
}

void initManagedDevice() {
 int rReboot,rUpdate,rResponse; 

 rReboot=  clienteMQTT.subscribe(rebootTopic,1);
 rUpdate=  clienteMQTT.subscribe(updateTopic,1);
 rResponse=clienteMQTT.subscribe(responseTopic,1);
 DPRINTLN("Suscripcion. Response= ");
 DPRINT("\tReboot= ");DPRINT(rReboot);
 DPRINT("\tUpdate= ");DPRINTLN(rUpdate); 
}

void funcallback(char* topic, byte* payload, unsigned int payloadLength) {
 DPRINT("funcallback invocado para el topic: "); DPRINTLN(topic);
 if (strcmp (updateTopic, topic) == 0) {
   handleUpdate(payload);  
 }
 else if (strcmp (responseTopic, topic) == 0) { 
   DPRINTLN("handleResponse payload: ");
   DPRINTLN((char *)payload); 
 } 
 else if (strcmp (rebootTopic, topic) == 0) {
   DPRINTLN("Rearrancando...");    
   ESP.restart(); // da problemas, no siempre rearranca
   //ESP.reset();
 }
}

void handleUpdate(byte* payload) {
 StaticJsonBuffer<JSONBUFFSIZE> jsonBuffer; 
 JsonObject& root = jsonBuffer.parseObject((char*)payload);
 boolean cambia=false,pubresult;
 char sensor[20],elpayload[150];

 DPRINTLN("handleUpdate payload, el payload:");
 DPRINTLN((char *)payload);
 DPRINTLN(" payload, antes de actualizar:");
 DPRINT("intervaloConex:"); DPRINTLN(intervaloConex);
 DPRINT("tRiego:"); DPRINTLN(tRiego);
 DPRINT("umbralRiego:"); DPRINTLN(umbralRiego);
 DPRINT("humedadMin:"); DPRINTLN(humedadMin);
 DPRINT("humedadMax:"); DPRINTLN(humedadMax);
 if (!root.success()) {
   return;
 }
 if (root.containsKey("sensor")) {
   if (strcmp (root["sensor"],DEVICE_ID) !=0) {
    strcpy(sensor,root["sensor"]);
    DPRINT("no va conmigo, es para ");
    DPRINTLN(sensor);
    return;
   }
 }
 if (root.containsKey("intervaloConex")) {
   intervaloConex=root["intervaloConex"];
   DPRINTLN("recibido intervalo de conexion");
   intervaloConex=intervaloConex-AJUSTA_T; 
   cambia=true;
 }
 if (root.containsKey("humedadMax")) {
   humedadMax=root["humedadMax"];
   DPRINTLN("recibido humedadMax");
   cambia=true;
 }
 if (root.containsKey("humedadMin")) {
   humedadMin=root["humedadMin"];
   DPRINTLN("Recibido humedadMin");
   cambia=true;
 }
 if (cambia==false) { // solo nos han enviado deviceId, hay que contestar con los valores actuales
     DPRINTLN("envio mis metadatos");
     sprintf(datosJson,"{\"metadata\":{\"tRiego\":%d,\"intervaloConex\":%d,\"umbralRiego\":%d,\"humedadMin\":%d,\"humedadMax\":%d} }",\
                 tRiego,intervaloConex,umbralRiego,humedadMin,humedadMax);
 } else {
     root.prettyPrintTo(elpayload);
     DPRINTLN("handleUpdate payload:"); DPRINTLN(elpayload);
 
     sprintf(datosJson,"{\"metadata\":{\"tRiego\":%d,\"intervaloConex\":%d,\"umbralRiego\":%d,\"humedadMin\":%d,\"humedadMax\":%d} }",
             tRiego,intervaloConex,umbralRiego,humedadMin,humedadMax);
     escribeValores(FICHEROVALORES,datosJson);   
     DPRINTLN("********A ver que hay en el fichero metadata:");
     cargaValores();
 }
 pubresult = enviaDatos(metadataTopic,datosJson);
 DPRINTLN("metadatos enviados");
 return;
}

boolean enviaDatos(char * topic, char * datosJSON) {
  int k=0;
  char signo;
  boolean pubresult=false;  
  
 while (!clienteMQTT.loop() & k<20 ) {
    DPRINTLN("Estaba desconectado ");   
    mqttConnect();
    initManagedDevice();
    k++; 
  } 
  pubresult = clienteMQTT.publish(topic,datosJSON);
  DPRINT("Envio ");DPRINT(datosJson);
  DPRINT("a ");DPRINTLN(publishTopic);
  if (pubresult) 
    DPRINTLN("... OK envio conseguido");      
  else
    DPRINTLN(".....KO envio fallado");
  return pubresult;    
}

void espera(unsigned long tEspera) {
  uint32_t principio = millis();
  
  while ((millis()-principio)<tEspera) {
    yield();
    delay(500);
  }
}
