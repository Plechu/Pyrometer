#include "MLX90614.h"

uint8_t crc = 0;

Adafruit_MLX90614::Adafruit_MLX90614(uint8_t i2caddr) {
  _addr = i2caddr;
}

uint8_t crc8(uint8_t data){
    uint8_t i = 8;
    crc ^= data;
    while (i--) crc = crc & 0x80 ? (crc << 1) ^ 7 : crc << 1;
    return crc;
}

boolean Adafruit_MLX90614::begin(void) {
  Wire.begin();
  return true;
}

//////////////////////////////////////////////////////

double Adafruit_MLX90614::readObjectTempC(void) {
  return readTemp(MLX90614_TOBJ1);
}

double Adafruit_MLX90614::readAmbientTempC(void) {
  return readTemp(MLX90614_TA);
}

float Adafruit_MLX90614::readTemp(uint8_t reg) {
  float temp;
  
  temp = read16(reg);
  temp *= .02;
  temp  -= 273.15;
  return temp;
}

void Adafruit_MLX90614::changeEmissivityFactor(float EF){
  uint16_t EF_hex = round(EF * 65535);
  write16(0x24, 0x0000); // czyszczenie pamieci w czujniku
  delay(500);
  write16(0x24, EF_hex); // zapis nowej emisyjnosci
  delay(500);
}

/*void Adafruit_MLX90614::showEmissivityFactor(){
    //Serial.println(read16(0x24));
    uint16_t EF_hex = read16(0x24);
    float EF = (float)EF_hex / 65535.0;
    Serial.print("Aktualny wspolczynnik: ");
    Serial.println(EF);
}*/

uint16_t Adafruit_MLX90614::read16(uint8_t a) {
  uint16_t ret;

  Wire.beginTransmission(_addr); // start transmission to device 
  Wire.write(a); // sends register address to read from
  Wire.endTransmission(false); // end transmission
  
  Wire.requestFrom(_addr, (uint8_t)3);// send data n-bytes read
  ret = Wire.read(); // receive DATA
  ret |= Wire.read() << 8; // receive DATA
  Wire.endTransmission();

  return ret;
}

void Adafruit_MLX90614::write16(uint8_t cmd, uint16_t data)
{
    uint8_t pec;
    crc = 0;
 
    crc8(_addr << 1);
    crc8(cmd);
    crc8(lowByte(data));
    pec = crc8(highByte(data));
 
    Wire.beginTransmission(_addr);
    Wire.write(cmd);
 
    Wire.write(lowByte(data));
    Wire.write(highByte(data));
 
    Wire.write(pec);
    
    Wire.endTransmission();
}