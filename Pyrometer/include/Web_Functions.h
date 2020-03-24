#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>

ESP8266WebServer server(80);
bool savedConfiguration = false;
String configData[3] = {"", "", ""};

bool sendMeasurement(String MaterialName, float MaterialEmissivity, float ObjectTemperature, float AmbientTemperature){ // wysylanie http do serwera
    String temp;
    DynamicJsonDocument json(256);

    json["PyrometerId"] = configData[2].toInt();
    json["MaterialName"] = MaterialName;
    json["MaterialEmissivity"] = MaterialEmissivity;
    json["ObjectTemperature"] = ObjectTemperature;
    json["AmbientTemperature"] = AmbientTemperature;

    serializeJson(json, temp); // serializacja 

    HTTPClient http;
    http.begin("http://192.168.0.94/collectdata"); // tu wysylane sa dane (adres serwera) USTAWIONY NA ADRES STATYCZNY, W DOMYSLE TU POWINIEN BYC TAKI SERWER
    http.addHeader("Content-Type", "application/json"); // header http
    int responseCode = http.POST(temp); // jaki response code
    http.end(); // zakonczenie http

    if(responseCode == 200) return true; // jesli poprawnie wyslano pomiar
    else return false; // jesli cos sie nie udalo
}

void pageFile(){ // wczytanie strony konfiguracji
    SPIFFS.begin();
    File file = SPIFFS.open("/index.html", "r"); // czytanie pliku
    server.streamFile(file, "text/html"); // wysylanie strony 
    file.close();
}

bool loadConfiguration(){ // wczytywanie konfiguracji
    String temp = "";
    int tempIndex = 0;

    SPIFFS.begin();
    File file = SPIFFS.open("/conf.txt", "r");
    size_t fileSize = file.size(); // musi to byc bo uzywajac samego file.size() nie dziala
    char* buf = new char[fileSize];
    file.readBytes(buf, fileSize); // wczytanie danych do bufora
    file.close();

    for(unsigned int i = 0; i < fileSize; ++i){ // petla zczytujaca wiersz
        if(buf[i] == '\r'){ // jak natrafi na powrot karetki
            configData[tempIndex] = temp; // koniec wiersza
            temp = ""; // czyszczenie tempa
            ++i; // jeszcze \n
            ++tempIndex;
        } 
        else temp += buf[i]; // klejenie wiersza
    }
    delete[] buf; // bez wyciekow pamieci

    if(configData[0] == "nowifi4me") WiFi.mode(WIFI_OFF); // wylacz WiFi jesli wybrano brak polaczenia
    else{ // jesli mamy miec polaczenie 
        int i = 0;
        WiFi.begin(configData[0], configData[1]); // proba polaczenia
        while(WiFi.status() != WL_CONNECTED){ // jesli jeszcze sie nie polaczylo
            delay(1000); // sekunda przerwy
            if(++i == 10) break; // jesli dojdzie do 10 wychodzi z petli
        }
        if(i != 10) return true; // jesli udalo sie polaczyc w mniej niz 10 sekund informacja ze polaczono
    }
    return false; // jesli nie polaczono/wybrano brak polaczenia
}

void saveConfiguration(){ // zapis konfiguracji 
    if(server.args()){ 
        DynamicJsonDocument json(256);
        deserializeJson(json, server.arg(0));
        String SSID = json["SSID"];
        String Password = json["Pass"];
        String PyroID = json["PyroID"];
        
        SPIFFS.begin();
        File file = SPIFFS.open("/conf.txt", "w"); // plik otwarty do pisania
        file.println(SSID);
        file.println(Password);
        file.println(PyroID);
        file.close();
        
        server.send(200); // wyslij ze wszystko ok
        delay(1000); // aby wyslac response
        WiFi.softAPdisconnect(); // wylaczenie AP
        WiFi.mode(WIFI_STA); // tryb laczenia do istniejacego AP

        savedConfiguration = true; // informacja ze ma nastapic reset urzadzenia
    } 
    else server.send(400); // jesli cos nie tak
}

void fetchSSIDs(){ // pobranie dostepnych sieci
    String serialized = "";
    unsigned int networksCount = WiFi.scanNetworks(); // ilosc sieci
    DynamicJsonDocument arr(1024); // tu mozna zmienic jesli za malo WiFi sie wyswietla
    for(unsigned int i = 0; i < networksCount; ++i) arr.add(WiFi.SSID(i)); // klejenie tablicy dostepnych sieci
    serializeJson(arr, serialized); // serializacja
    server.send(200, "application/json", serialized); // wyslanie 
}

void startServer(){
    server.on("/", pageFile); // glowna strona konfiguracji
    server.on("/saveconfiguration", saveConfiguration); // tutaj zbiera dane konfiguracyjne
    server.on("/fetchssids", fetchSSIDs); // tutaj wysyla liste sieci
    server.begin(); // start serwera http
}