#include "HUSKYLENS.h"
#include "Wire.h"
#include <Servo.h>

HUSKYLENS huskylens;
Servo servoDireccion;

// --- PINES DEL DRV8833 (Actualizados) ---
const int motorIN1 = 9;   
const int motorIN2 = 10;   

// --- PIN DEL SERVO DE DIRECCIÓN ---
const int pinServo = 2; 

// --- ÁNGULOS DEL SERVO ---
const int ANGULO_CENTRO = 90;
const int ANGULO_IZQ = 60;  // Leve a la izquierda
const int ANGULO_DER = 120; // Leve a la derecha

// --- IDs DE LA HUSKYLENS ---
const int ID_ROJO = 4;
const int ID_VERDE = 3;

// --- "DISTANCIA" VISUAL (TAMAÑO DEL BLOQUE) ---
// La pantalla mide 320 pixeles de ancho. 
// 60 pixeles significa que el objeto está a una distancia media/corta.
// Si gira muy pronto, súbele a 90 o 120. Si choca antes de girar, bájale a 40.
const int TAMANO_MINIMO = 60; 

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000); 

  // Configurar pines del DRV8833
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);

  // Configurar servo
  servoDireccion.attach(pinServo);
  servoDireccion.write(ANGULO_CENTRO);

  Serial.println(F("Iniciando sistema..."));

  while (!huskylens.begin(Wire)) {
    Serial.println(F("Fallo al iniciar HuskyLens. Revisa los cables I2C."));
    delay(100);
  }
  Serial.println(F("HuskyLens lista. ¡Arrancando!"));
}

void loop() {
  // 1. AVANZAR A MÁXIMA POTENCIA SIEMPRE
  digitalWrite(motorIN1, HIGH); 
  digitalWrite(motorIN2, LOW);

  bool esquivar = false;
  int colorDetectado = 0;

  // 2. LEER LA CÁMARA
  if (huskylens.request() && huskylens.available()) {
    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      
      if (result.command == COMMAND_RETURN_BLOCK) {
        
        // AQUÍ ESTÁ LA MAGIA DE LA "DISTANCIA"
        // Solo reacciona si el ancho (width) del bloque es mayor al tamaño mínimo
        if (result.width > TAMANO_MINIMO) {
          colorDetectado = result.ID;
          esquivar = true;
          break; // Si ya vio un bloque lo suficientemente grande, sale a esquivar
        }
      }
    }
  }

  // 3. LÓGICA DE DIRECCIÓN
  if (esquivar) {
    if (colorDetectado == ID_ROJO) {
      Serial.print(F("ROJO CERCA! Ancho: ")); // Solo para que lo veas en el monitor serie
      Serial.println(TAMANO_MINIMO);
      
      servoDireccion.write(ANGULO_IZQ);
      delay(500); 
    } 
    else if (colorDetectado == ID_VERDE) {
      Serial.print(F("VERDE CERCA! Ancho: "));
      Serial.println(TAMANO_MINIMO);
      
      servoDireccion.write(ANGULO_DER);
      delay(500); 
    }
  } else {
    // Si no ve nada, o si lo que ve está muy lejos (bloque pequeño), sigue recto
    servoDireccion.write(ANGULO_CENTRO);
  }
}