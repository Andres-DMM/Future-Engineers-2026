#include "HUSKYLENS.h"
#include <Wire.h>

HUSKYLENS huskylens;

// Definición de tus IDs asignados en la cámara
const int ID_NARANJA = 1;
const int ID_AZUL = 2;
const int ID_ROJO = 3;
const int ID_VERDE = 4;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Inicialización de HuskyLens por I2C
  while (!huskylens.begin(Wire)) {
    Serial.println(F("Error: No se detecta HuskyLens. Revisa cables e I2C en el menú."));
    delay(1000);
  }
  Serial.println(F("HuskyLens listo para reconocimiento de color."));
}

void loop() {
  // Solicita los datos de los bloques en pantalla
  if (!huskylens.request()) {
    Serial.println(F("Fallo al comunicarse con HuskyLens"));
    delay(100);
    return;
  }

  // Verifica si hay bloques detectados en la pantalla
  while (huskylens.available()) {
    HUSKYLENSResult result = huskylens.read();
    
    // Filtramos las acciones según el ID específico del color detectado
    if (result.ID == ID_NARANJA) {
      Serial.print(F("¡Color NARANJA detectado! (ID 1) -> "));
      imprimirCoordenadas(result);
      // Aquí puedes encender un LED, un motor, etc.
    } 
    else if (result.ID == ID_AZUL) {
      Serial.print(F("¡Color AZUL detectado! (ID 2) -> "));
      imprimirCoordenadas(result);
    } 
    else if (result.ID == ID_ROJO) {
      Serial.print(F("¡Color ROJO detectado! (ID 3) -> "));
      imprimirCoordenadas(result);
    }
    else if (result.ID == ID_VERDE) {
      Serial.print(F("¡Color VERDE detectado! (ID 4) -> "));
      imprimirCoordenadas(result);
    } 
    else {
      Serial.print(F("Color desconocido detectado con ID: "));
      Serial.println(result.ID);
    }
  }
  
  delay(100); // Pequeña pausa para no saturar el monitor serie
}

// Función auxiliar para mostrar la ubicación del color en la pantalla
void imprimirCoordenadas(HUSKYLENSResult result) {
  Serial.print(F("Posición Centro X: "));
  Serial.print(result.xCenter);
  Serial.print(F(" | Ancho: "));
  Serial.println(result.width);
}
