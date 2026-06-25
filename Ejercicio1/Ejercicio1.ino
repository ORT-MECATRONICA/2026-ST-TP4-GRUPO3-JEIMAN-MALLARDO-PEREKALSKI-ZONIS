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

typedef enum {
  RST,
  P1,
  P2
} estados_t;

estados_t maquinaPantalla = RST;

float temperatura = 0;
float valorUmbral = 26;

bool sw1_flag = false;
bool sw2_flag = false;
unsigned long tiempo_sw1 = 0;
unsigned long tiempo_sw2 = 0;
long int millisUltimoCheck = 0;
bool ignorar_sw1 = false;

void setup() {
  Serial.begin(115200);
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);

  dht.begin();
  u8g2.begin();

  preferences.begin("st-tp4", false);
  valorUmbral = preferences.getFloat("umbral", 26.0);
}

void loop() {
  bool sw1_actual = !digitalRead(SW1);
  bool sw2_actual = !digitalRead(SW2);

  // Leer el sensor de temperatura cada 2 segundos
  if (millis() - millisUltimoCheck >= 2000) {
    float temperaturaTest = dht.readTemperature();
    if (!isnan(temperaturaTest)) {
      temperatura = temperaturaTest;
    }
    millisUltimoCheck = millis();
  }

  switch (maquinaPantalla) {
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

      if (sw1_actual && !sw1_flag) {
        tiempo_sw1 = millis();
        sw1_flag = true;
      }

      // Si se mantiene presionado
      if (sw1_actual) {
        if (millis() - tiempo_sw1 >= 5000) {
          maquinaPantalla = P2;
          tiempo_sw1 = millis();
          ignorar_sw1 = true;
        }
      } else sw1_flag = false;

      break;

    case P2:
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.setCursor(0, 15);
      u8g2.print("VU : ");
      u8g2.print(valorUmbral);
      u8g2.sendBuffer();

      if (ignorar_sw1) {
        if (!sw1_actual) {
          ignorar_sw1 = false;
          sw1_flag = false;
        }
      } else {
        if (sw1_actual) sw1_flag = true;
        // Aumentar umbral 1 grado al soltar SW1
        if (!sw1_actual && sw1_flag) {
          valorUmbral++;
          sw1_flag = false;
        }
      }

      // Lógica para SW2: Disminuir o Volver
      if (sw2_actual && !sw2_flag) {
        tiempo_sw2 = millis();
        sw2_flag = true;
      }

      if (sw2_actual) {
        if (millis() - tiempo_sw2 >= 5000) {
          // Volver a P1 y guardar en Preferences
          maquinaPantalla = P1;
          preferences.putFloat("umbral", valorUmbral);
          Serial.print("Valor umbral guardado en memoria: ");
          Serial.println(valorUmbral);

          sw2_flag = false;
          tiempo_sw2 = 0;
        }
      } else {
        sw2_flag = false;
        if (tiempo_sw2 != 0) {
          valorUmbral--;
          tiempo_sw2 = 0;
        }
      }
      break;
  }
}
