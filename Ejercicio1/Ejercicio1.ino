//Grupo 3: Kenai Jeiman, Marco Mallardo, Ramiro Perekalski, Martín Zonis
#include <Arduino.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <Preferences.h>

#define DHTPIN 23
#define DHTTYPE DHT11
#define LEDPIN 25
#define SW1 35
#define SW2 34

Preferences preferences;


DHT dht(DHTPIN, DHTTYPE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

typedef enum
{
  RST,
  P1,
  P2
} estados_t;

estados_t maquinaPantalla = RST;

float temperatura = 0;
float valorUmbral = 26;

unsigned long tiempo_sw1 = 0;
unsigned long tiempo_sw2 = 0;
bool sw1_anterior = false;
bool sw2_anterior = false;
long int millisUltimoCheck = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  
  dht.begin();
  u8g2.begin();

  preferences.begin("mi-app", false);
  valorUmbral = preferences.getFloat("umbral", 26.0);
}

void loop()
{

  // Leer el sensor de temperatura cada 2 segundos
  if (millis() - millisUltimoCheck >= 2000)
  { 
    float temperaturaTest = dht.readTemperature();
    if (!isnan(temperaturaTest))
    {
      temperatura = temperaturaTest;
    }
    millisUltimoCheck = millis();
  }

  // Estado actual de los pulsadores (lógica negada)
  bool sw1 = !digitalRead(SW1); 
  bool sw2 = !digitalRead(SW2);

  // Variables de flanco (comparando el estado actual con el anterior)
  bool sw1_flanco_subida = (sw1 && !sw1_anterior);
  bool sw1_flanco_bajada = (!sw1 && sw1_anterior);
  
  bool sw2_flanco_subida = (sw2 && !sw2_anterior);
  bool sw2_flanco_bajada = (!sw2 && sw2_anterior);

  switch (maquinaPantalla)
  {
  case RST:
    maquinaPantalla = P1;
    break;

  case P1:
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0, 15);
    u8g2.print("VA : ");
    u8g2.print(temperatura);
    u8g2.setCursor(0, 35);
    u8g2.print("VU : ");
    u8g2.print(valorUmbral);
    u8g2.sendBuffer();

    if (sw1_flanco_subida) {
      tiempo_sw1 = millis();
    }
    
    // Si se mantiene presionado
    if (sw1) {
      if (millis() - tiempo_sw1 >= 5000) {
        maquinaPantalla = P2;
        tiempo_sw1 = millis(); // Resetear para evitar transiciones erráticas
      }
    }
    break;

  case P2:
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0, 15);
    u8g2.print("VU : ");
    u8g2.print(valorUmbral);
    u8g2.sendBuffer();

    // Aumentar umbral 1 grado al presionar SW1
    if (sw1_flanco_subida) {
      valorUmbral++;
    }

    // Lógica para SW2: Disminuir o Volver
    if (sw2_flanco_subida) {
      tiempo_sw2 = millis();
    }
    
    if (sw2) {
      if (millis() - tiempo_sw2 >= 5000) {
        // Volver a P1 y guardar en Preferences
        maquinaPantalla = P1;
        preferences.putFloat("umbral", valorUmbral);
        Serial.print("Valor umbral guardado en memoria: ");
        Serial.println(valorUmbral);
        
        // Anular el timer para no restar al soltar
        tiempo_sw2 = 0; 
      }
    } else if (sw2_flanco_bajada) {
      // Se soltó SW2 antes de los 5 segundos, entonces disminuimos 1 grado
      if (tiempo_sw2 != 0 && (millis() - tiempo_sw2 < 5000)) {
        valorUmbral--;
      }
    }

    break;
  }

  // Guardar el estado actual como anterior para la próxima iteración
  sw1_anterior = sw1;
  sw2_anterior = sw2;
}
