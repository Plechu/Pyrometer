#include <Arduino.h>
#include <FS.h> // biblioteka file system (SPIFFS)
#include "MLX90614.h" // biblioteka czujnika
#include "Web_Functions.h" // biblioteka funkcji
#include <TFT_eSPI.h> // biblioteka wyswietlacza
#include <SPI.h> // do wyswietlacza

class Material{
  String materials[31][2] = { // tablica materialow (oszczedniej niz ladowac do pamieci i z niej wyciagac)
    {"Zloto polerowane", "0.02"}, {"Mosiadz polerowany", "0.03"}, {"Aluminium polerowane", "0.05"}, {"Aluminium niepolerowane", "0.07"}, {"Miedz polerowana", "0.07"},
    {"Chrom polerowany", "0.1"}, {"Cynk i blacha", "0.2"}, {"Mosiadz zmatowiony", "0.22"}, {"Stal walcowana", "0.24"}, {"Stal nierdzewna", "0.59"},
    {"Aluminium anodowane", "0.78"}, {"Sklejka", "0.82"}, {"Bialy plastik", "0.84"}, {"Panele drewniane", "0.87"}, {"Czarny lakier", "0.87"},
    {"Drewno", "0.9"}, {"Papier", "0.9"}, {"P.V.C.", "0.92"}, {"Porcelana glazurowana", "0.92"}, {"Ziemia", "0.92"},
    {"Szklo", "0.92"}, {"Czerwona cegla", "0.93"}, {"Guma", "0.93"}, {"Szklo akrylowe", "0.94"}, {"Taśma izolacyjna", "0.95"},
    {"Czarny plastik", "0.95"}, {"Suchy beton", "0.95"}, {"Szklo matowe", "0.96"}, {"Lod", "0.97"}, {"Skora", "0.98"}, {"Skron", "0.95"}
  };
  int materialsIndex; // index w tablicy materialow
  String currentMaterialName; // obecna nazwa materialu wskazana przez index
  float currentMaterialEmissivity; // obecna nazwa emisyjnosci -||-

public:
  Material(){ // konstruktor ladujacy index z pamieci (ostatni ustawiony material przed wylaczeniem zasilania)
  SPIFFS.begin(); // otwarcie SPI Flash File System
  materialsIndex = 0;
  int multiplier = 1; //mnoznik potrzebny do algorytmu

  File file = SPIFFS.open("/materials_index.txt", "r"); // otwarcie pliku z pamieci flash
  size_t fileSize = file.size(); // rozmiar pliku (od 1 do 2 znakow, liczba max 2 cyfrowa)
  char* buf = new char[fileSize]; // bufor
  file.readBytes(buf, fileSize); // wpisanie zawartosci pliku do bufora
  file.close(); // zamkniecie pliku

  for(unsigned int i = fileSize; i > 0; --i){ // tworzenie indeksu
    materialsIndex += ((int)buf[i - 1]  - '0') * multiplier; // liczba raz 1 lub 10
    multiplier *= 10; // mnoznik * 10
  }
  delete[] buf; // usuniecie bufora (BEZ WYCIEKU PAMIECI)
  currentMaterialName = materials[materialsIndex][0]; // ustawienie nazwy wskazanego materialu
  currentMaterialEmissivity = String(materials[materialsIndex][1]).toFloat(); // ustawienie emisyjnosci -||-
  }
  
  void saveMaterialsIndex(){ // zapis indeksu do pliku
    SPIFFS.begin(); // montowanie
    File file = SPIFFS.open("/materials_index.txt", "w"); // otwarcie pliku w trybie zapisu
    file.print(String(materialsIndex)); // wpisanie indexu do pliku
    file.close(); // zamykanie pliku
  }

  void shiftMaterials(bool shift){ // przesuwanie sie w tablicy materialow
    if(shift){ // jesli przesuniecie w prawo
      if(materialsIndex == 30) materialsIndex = 0; // jesli nie przekroczymy zakresu to kolejny material, jesli tak to zaczynamy od poczatku
      else ++materialsIndex;
    }
    else{ // w lewo
      if(materialsIndex == 0) materialsIndex = 30; // jesli nie przekroczymy zakresu to poprzedni material, jesli tak to zaczynamy od konca
      else --materialsIndex; 

    }
    currentMaterialName = materials[materialsIndex][0]; // ustawienie nazwy wskazanego materialu
    currentMaterialEmissivity = String(materials[materialsIndex][1]).toFloat(); // ustawienie emisyjnosci -||-
  }

  String getCurrentMaterialName(){ return currentMaterialName; } // akcesor do pobrania nazwy materialu
  float getCurrentMaterialEmissivity(){ return currentMaterialEmissivity; } // ackesor do pobrania emisyjnosci materialu
};

Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // instancja klasy obslugi czujnika
Material materialObject; // instancja klasy materialu
TFT_eSPI tft = TFT_eSPI(); // instacja klasy obslugi wyswietlacza
String objTemp = ""; // przetrzymanie przez iteracje temperatury obiektu
String ambTemp = ""; // -||- otoczenia
String emissivity = ""; // przetrzymanie emisyjnosci
int posBegObjTemp = 0; // pozycja poczatku wyswietlania temperatury obiektu
int posEndObjTemp = 0; // pozycja konca -||- 
int latchedTime = 0; // zatrzasniety czas wejscia do przerwania (drgania stykow)
int latchedMeasurementTime = 0;
float lastMaterialEmissivity = materialObject.getCurrentMaterialEmissivity(); // ostatni ustawiony wspolczynnik emisyjnosc
bool buttonFlag = true; // informacja ze nacisnieto przycisk
bool measurementButtonFlag = false; // -||- pomiaru
bool changeEmissivityFlag = false; // flaga zmiany emisyjnosci
bool refreshStaticText = true; // informacja czy odswiezyc stale elementy wystwielacza
bool hideInfo = false; // informacja czy schowac kominikat o zmianie emisyjnosci
bool APFlag = false; // informacja czy urzadzenie ma rozglaszac swoja siec

int xCalculateCenterText(const char* string){return round(((160 - tft.textWidth(string))/2)); } // funkcja obliczajaca poczatkowa pozycje dla wysrodkowanego napisu
int xCalculateCenterTextInRefrence(const char* refrence, const char* string){ // -||- w odniesieniu do innego napisu
  int temp = tft.textWidth(refrence) - tft.textWidth(string);
  if(temp > 0) return round(temp/2); 
  else return 0;
  }

void ICACHE_RAM_ATTR handleMeasurementButton(){ // przerwanie wywolane przez nacisniecie przycisku pomiaru
  detachInterrupt(digitalPinToInterrupt(10)); // wylaczenie wszystkch przerwan na czas wysylu
  detachInterrupt(digitalPinToInterrupt(0));
  detachInterrupt(digitalPinToInterrupt(2));
  latchedMeasurementTime = millis(); // drgania 
  measurementButtonFlag = true; // info ze wysylamy
}
void ICACHE_RAM_ATTR handleLeftButton(){ // przerwanie wywolane nacisnieciem lewego przycisku
  detachInterrupt(digitalPinToInterrupt(0)); // wylaczenie przerwania z uwagi na drgania stykow
  materialObject.shiftMaterials(false); // przesuniecie sie w tablicy materialow w lewo
  if(lastMaterialEmissivity != materialObject.getCurrentMaterialEmissivity()) changeEmissivityFlag = true; // ustawienie flagi zmiany emisyjnosci
  else changeEmissivityFlag = false;
  latchedTime = millis(); // zatrzasniecie czasu przerwania 
  buttonFlag = true; // flaga informujaca o nacisnieciu przycisku
}

void ICACHE_RAM_ATTR handleRightButton(){ // to samo co dla lewego
  detachInterrupt(digitalPinToInterrupt(2));
  materialObject.shiftMaterials(true); // przesuniecie sie w tablicy materialow w prawo
  if(lastMaterialEmissivity != materialObject.getCurrentMaterialEmissivity()) changeEmissivityFlag = true;
  else changeEmissivityFlag = false;
  latchedTime = millis();
  buttonFlag = true;
}

void setup() {
  pinMode(0, INPUT_PULLUP); // pullup dla przycisku lewego
  pinMode(2, INPUT_PULLUP); // -||- prawego
  pinMode(10, INPUT_PULLUP); // -||- wysylania danych
  tft.init(); // inicjalizacja wyswietlacza
  tft.fillScreen(TFT_BLACK); // ustawienie tla na czarne
  tft.setRotation(1); // rotacja pozioma
  tft.setTextSize(1);
  mlx.begin(); // start I2C

  SPIFFS.begin();
  if(!SPIFFS.exists("/conf.txt")){ // jesli plik konfiguracyjny nie istnieje to nalezy skonfigurowac urzadzenie
    // Obsluga wyswietlacza
    tft.setTextFont(2);
    tft.setCursor(xCalculateCenterText("Skonfiguruj urzadzenie"), 10);
    tft.print(F("Skonfiguruj urzadzenie"));
    tft.setTextFont(1);
    tft.setCursor(xCalculateCenterText("SSID: Pyrometer"), 50);
    tft.print(F("SSID: Pyrometer"));
    tft.setCursor(xCalculateCenterText("Haslo: telemedycyna2020"), 60);
    tft.print(F("Haslo: telemedycyna2020"));
    tft.setCursor(xCalculateCenterText("Adres IP: 192.168.1.1"), 80);
    tft.print(F("Adres IP: 192.168.1.1"));

    WiFi.mode(WIFI_AP); // WiFi w trybie Access Point
    WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0)); // parametry sieci rozglaszanej przez uC
    WiFi.softAP("Pyrometer", "telemedycyna2020", 1, 0, 1); // ssid odkryte, jeden kanal, jeden klient
    startServer(); // wlaczenie serwera http
    APFlag = true; // informacja ze AP jest wlaczone 
  } 
  else{ // jesli plik konfiguracyjny jest wlaczony
    tft.setTextFont(2);
    tft.setCursor(xCalculateCenterText("Sprawdzanie ustawien..."), 30);
    tft.print(F("Sprawdzanie ustawien..."));

    if(loadConfiguration()){ // jesli jest polaczony z siecia wpisana w konfiguracji, jesli false to znaczy ze albo nie polaczony albo wybrano brak sieci
      tft.fillRect(0, 25, 160, 27, TFT_BLACK);
      tft.setTextFont(2);
      tft.setCursor(xCalculateCenterText("Polaczono z siecia"), 10);
      tft.print(F("Polaczono z siecia"));
      tft.setTextFont(1);
      tft.setCursor(xCalculateCenterText(configData[0].c_str()), 50);
      tft.print(configData[0]); // informacja o zapisie czujnika
      tft.setCursor(xCalculateCenterText(WiFi.localIP().toString().c_str()), 70);
      tft.print(WiFi.localIP().toString());
      tft.setCursor(xCalculateCenterText(("Numer pirometru: " + configData[2]).c_str()), 90);
      tft.print("Numer pirometru: " + configData[2]);
      delay(5000);
      tft.fillScreen(TFT_BLACK);
      attachInterrupt(digitalPinToInterrupt(10), handleMeasurementButton, FALLING);// jesli sie polaczyl to znaczy ze mozna probowac wysylac pomiary do serwera
    }
    else{ // jesli nie polaczy sie z siecia podana w konfiguracji
      tft.fillRect(0, 25, 160, 27, TFT_BLACK);
      tft.setTextFont(2);
      if(configData[0] == "nowifi4me"){ // jesli wybrano brak polaczenia
      tft.setCursor(xCalculateCenterText("Nie wybrano sieci"), 0);
      tft.print(F("Nie wybrano sieci")); // informacja o zapisie czujnika
      hideInfo = true;
      }
      else{
      tft.setCursor(xCalculateCenterText("Brak polaczenia"), 10);
      tft.print(F("Brak polaczenia"));
      tft.setTextFont(1);
      tft.setCursor(xCalculateCenterText("Nieprawidlowe haslo"), 40);
      tft.print(F("Nieprawidlowe haslo")); 
      tft.setCursor(xCalculateCenterText("lub"), 60);
      tft.print("lub");
      tft.setCursor(xCalculateCenterText("Nie znaleziono sieci"), 80);
      tft.print(F("Nie znaleziono sieci"));
      tft.setCursor(xCalculateCenterText(configData[0].c_str()), 100);
      tft.print(configData[0]);
      delay(5000);
      tft.fillScreen(TFT_BLACK);
      }
    } 
  } 
}

void loop() {
  if(APFlag){ // jesli nie skonfigurowana siec
    server.handleClient(); // zajmowanie sie clientem
    if(savedConfiguration){ // jesli flaga zapisu konfiguracji 
      tft.fillScreen(TFT_BLACK);
      tft.setTextFont(2);
      tft.setCursor(xCalculateCenterText("Zapisano ustawienia"), 30);
      tft.print(F("Zapisano ustawienia"));
      tft.setTextFont(1);
      tft.setCursor(xCalculateCenterText("Restart urzadzenia..."), 60);
      tft.print(F("Restart urzadzenia..."));
      delay(3000);
      ESP.restart();
    }
  }
  else{ // jesli jest skonfigurowana siec
    if(millis() - latchedTime > 5000 && buttonFlag && digitalRead(0) == LOW && digitalRead(2) == LOW){ // jesli przycisnieto 2 przyciski na 5 sekund
      tft.fillScreen(TFT_BLACK);
      tft.setTextFont(2);
      tft.setCursor(xCalculateCenterText("Usunieto ustawienia"), 30);
      tft.print(F("Usunieto ustawienia"));
      tft.setTextFont(1);
      tft.setCursor(xCalculateCenterText("Restart urzadzenia..."), 60);
      tft.print(F("Restart urzadzenia..."));
      delay(3000);
      
      SPIFFS.begin();
      SPIFFS.remove("/conf.txt"); // usuniecie pliku konfiguracyjnego
      ESP.restart();
    }

    if(millis() - latchedMeasurementTime > 250 && measurementButtonFlag && digitalRead(10) == HIGH && !changeEmissivityFlag && !refreshStaticText){ // jesli po drganiach, wcisnieto przycisk, puszczono przycisk, nie zmieniono emisyjnosci i nie trwa kalibracja
    // w warunku !refreshStaticText poniewaz to jest true tylko jesli program startuje + kiedy jest kalibracja
      measurementButtonFlag = false; // zerowanie flagi przycisniecia przycisku
      tft.setTextFont(2);
      if(sendMeasurement(materialObject.getCurrentMaterialName(), materialObject.getCurrentMaterialEmissivity(), objTemp.toFloat(), ambTemp.toFloat())){ // wyslanie pomiaru do serwera
      tft.setCursor(xCalculateCenterText("Wyslano pomiar"), 0);
      tft.print(F("Wyslano pomiar")); // wyslano oraz otrzymano potwierdzenie z serwera
      }
      else{
      tft.setCursor(xCalculateCenterText("Blad przesylu"), 0);
      tft.print(F("Blad przesylu")); // wyslano oraz nie otrzymano potwierdzenie z serwera
      }
      hideInfo = true;
      attachInterrupt(digitalPinToInterrupt(0), handleLeftButton, FALLING); // wlaczenie przerwania od opadajacego zbocza (nacisniecie przycisku)
      attachInterrupt(digitalPinToInterrupt(2), handleRightButton, FALLING); // -||-
      attachInterrupt(digitalPinToInterrupt(10), handleMeasurementButton, FALLING); // wlaczenie przerwania od opadajacego zbocza (nacisniecie przycisku)
    }

    if(millis() - latchedTime > 250 && buttonFlag && !(digitalRead(0) == LOW || digitalRead(2) == LOW)){ // jesli po drganiach i byl przycisniety przycisk i puszczone oba przyciski
      buttonFlag = false; // zerowanie flagi przycisniecia przycisku
      attachInterrupt(digitalPinToInterrupt(0), handleLeftButton, FALLING); // wlaczenie przerwania od opadajacego zbocza (nacisniecie przycisku)
      attachInterrupt(digitalPinToInterrupt(2), handleRightButton, FALLING); // -||-
      // Drukowanie nazwy materialu
      tft.fillRect(0, 100, 160, 28, TFT_BLACK); // czysci pole nazwy materialu
      tft.setTextFont(2);
      tft.setCursor(xCalculateCenterText(materialObject.getCurrentMaterialName().c_str()), 110); // ustawienie kursora do zapisu nazwy materialu na srodku
      tft.print(materialObject.getCurrentMaterialName()); // wyswietlenie nazwy materialu

      tft.fillRect(80, 84, 80, 10, TFT_BLACK); // czyszczenie wartosci emisyjnosci
      tft.setTextFont(1);
      tft.setCursor(160 - tft.textWidth("Emisyjnosc") + xCalculateCenterTextInRefrence("Emisyjnosc", String(materialObject.getCurrentMaterialEmissivity()).c_str()), 85); // ustawienie kursora
      tft.print(String(materialObject.getCurrentMaterialEmissivity())); // wyswietlenie emisyjnosci
    }
  
    if(millis() - latchedTime > 3000 && changeEmissivityFlag){ // jesli od klikniecia przycisku minely 3s i zmieniono emisyjnosc
      detachInterrupt(digitalPinToInterrupt(0)); // wylaczenie przerwan aby nie przerywac zapisu emisyjnosci do czujnika
      detachInterrupt(digitalPinToInterrupt(2));
      changeEmissivityFlag = false; // zerowanie flagi zmiany emisyjnosci
      lastMaterialEmissivity = materialObject.getCurrentMaterialEmissivity();
      materialObject.saveMaterialsIndex(); // zapis do pliku aktualnej pozycji indexu z tabeli materialow
      mlx.changeEmissivityFactor(lastMaterialEmissivity); // zmiana emisyjnosci wyslana do czujnika
      tft.setTextFont(2);
      tft.setCursor(xCalculateCenterText("Zapisano emisyjnosc"), 0);
      tft.print(F("Zapisano emisyjnosc")); // informacja o zapisie czujnika
      hideInfo = true;
      attachInterrupt(digitalPinToInterrupt(0), handleLeftButton, FALLING); // wlaczenie przerwan
      attachInterrupt(digitalPinToInterrupt(2), handleRightButton, FALLING);
    }
    if(mlx.readAmbientTempC() > 50){ // jesli bug czujnika (nieznany powod)
      tft.fillScreen(TFT_BLACK); // czyszczenie wyswietlacza
      tft.setTextFont(2);
      if(mlx.readAmbientTempC() > 1000){ // to jesli czujnik nie ma polaczenia z uC
        tft.setCursor(xCalculateCenterText("Brak danych czujnika"), 30);
        tft.print(F("Brak danych czujnika"));
      }else{
        tft.setCursor(xCalculateCenterText("Trwa kalibracja"), 30);
        tft.print(F("Trwa kalibracja")); // wyswietlanie informacji o kalibracji czujnika
        mlx.changeEmissivityFactor(materialObject.getCurrentMaterialEmissivity()); // force zapis emisyjnosci do czujnika (jedyny znany sposob na naprawienie buga)
      }
      refreshStaticText = true; // ustawienie flagi o wyswietleniu stalych elementow
    }else{
      tft.fillRect(0, 84, 80, 10, TFT_BLACK); // czysci pole temperatury otoczenia
      tft.fillRect(0, 25, 160, 27, TFT_BLACK); // czysci pole temperatury obiektu (napis kalibracji tez)
      // Drukowanie temperatury obiektu
      if(materialObject.getCurrentMaterialName() == "Skron") objTemp = 0.13 * (mlx.readObjectTempC() - mlx.readAmbientTempC()) + mlx.readObjectTempC(); // dla skroni
      else objTemp = String(mlx.readObjectTempC()); // zapis temperatury obiektu
      tft.setTextFont(4);
      posBegObjTemp = xCalculateCenterText(objTemp.c_str());
      tft.setCursor(posBegObjTemp, 30);
      tft.print(objTemp);
      posEndObjTemp = posBegObjTemp + tft.textWidth(objTemp);
      tft.drawCircle(posEndObjTemp + 5, 30, 2, TFT_WHITE); // znak stopni
      tft.setCursor(posEndObjTemp + 10, 25);
      tft.setTextFont(2);
      tft.print("C");
      // Drukowanie otoczenia
      ambTemp = String(mlx.readAmbientTempC()) + " C"; // zapis temperatury otoczenia
      tft.setTextFont(1);
      tft.setCursor(xCalculateCenterTextInRefrence("Otoczenie", ambTemp.c_str()), 85); 
      tft.print(ambTemp);
      tft.drawCircle(xCalculateCenterTextInRefrence("Otoczenie", ambTemp.c_str()) + tft.textWidth(ambTemp) - tft.textWidth("*C") + 3, 86, 1, TFT_WHITE); // znak stopni

      if(refreshStaticText){ // jesli trzeba odswiezyc stale elementy
      refreshStaticText = false;
      tft.setCursor(0, 75);
      tft.print(F("Otoczenie"));
      tft.setCursor(160 - tft.textWidth("Emisyjnosc"), 75);
      tft.print(F("Emisyjnosc"));
      tft.setCursor(160 - tft.textWidth("Emisyjnosc") + xCalculateCenterTextInRefrence("Emisyjnosc", String(materialObject.getCurrentMaterialEmissivity()).c_str()), 85);
      tft.print(String(materialObject.getCurrentMaterialEmissivity()));
      tft.setTextFont(2);
      tft.setCursor(xCalculateCenterText(materialObject.getCurrentMaterialName().c_str()), 110);
      tft.print(materialObject.getCurrentMaterialName());
      }
    }
    delay(1000); // delay dla porzadku (niezbyt elegancke, przepraszam)

    if(hideInfo){ 
      tft.fillRect(0, 0, 160, 20, TFT_BLACK); // czyszczenie kominikatu o zmianie emisyjnosci
      hideInfo = false;
    }
  }
}
