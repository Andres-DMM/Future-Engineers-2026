#include <Wire.h>
#include <Servo.h>
#include "HUSKYLENS.h"

// =====================================================
// HUSKYLENS
// =====================================================

HUSKYLENS huskylens;

// =====================================================
// PINES
// =====================================================

// L293D
const int motorIN1 = 2;
const int motorIN2 = 3;
const int motorENA = 5;

// Servo
const int SERVO_PIN = 9;


// =====================================================
// SERVO
// =====================================================

Servo steeringServo;

int anguloServo = 90;

const int ANGULO_CENTRO = 90;

// Primer movimiento
const int ANGULO_PRIMER_GIRO = 45;

// Segundo movimiento:
// 20° más hacia la izquierda
const int ANGULO_SEGUNDO_GIRO = 25;


// =====================================================
// VELOCIDADES
// =====================================================

const int VEL_RECTA = 102;

const int VEL_GIRO = 65;


// =====================================================
// TIEMPOS
// =====================================================

// Después de llegar a 25°,
// mantener el giro durante 300 ms
const unsigned long TIEMPO_20_GRADOS = 300;


// =====================================================
// ESTADOS
// =====================================================

enum Estado {

  RECTO,

  // ID2 detectado
  GIRO_INICIAL,

  // Ya giró a 45° y sigue avanzando
  BUSCANDO_ID4,

  // ID4 detectado
  SEGUNDO_GIRO,

  // Manteniendo los 25°
  MANTENER_GIRO,

  // Regresando a 90°
  REGRESANDO

};

Estado estado = RECTO;

unsigned long tiempoEstado = 0;


// =====================================================
// MOTOR
// =====================================================

void avanzar(int velocidad) {

  digitalWrite(motorIN1, HIGH);
  digitalWrite(motorIN2, LOW);

  analogWrite(motorENA, velocidad);
}


void pararMotor() {

  analogWrite(motorENA, 0);

}


// =====================================================
// MOVIMIENTO SUAVE DEL SERVO
// =====================================================
//
// El servo se mueve de grado en grado.
//
// 90 → 89 → 88 → ... → 45
//
// Después:
//
// 45 → 44 → 43 → ... → 25
//
// NUNCA hace:
// 45 → 90 → 25
//
// =====================================================

void moverServoSuave(int objetivo) {

  while (anguloServo != objetivo) {

    if (anguloServo > objetivo) {

      anguloServo--;

    }
    else if (anguloServo < objetivo) {

      anguloServo++;

    }

    steeringServo.write(anguloServo);

    // Movimiento gradual pero relativamente rápido
    delay(10);
  }
}


// =====================================================
// LEER HUSKYLENS
// =====================================================

void leerHuskyLens() {

  bool encontroID2 = false;
  bool encontroID4 = false;

  if (!huskylens.request()) {
    return;
  }

  while (huskylens.available()) {

    HUSKYLENSResult resultado = huskylens.read();

    if (resultado.ID == 2) {
      encontroID2 = true;
    }

    if (resultado.ID == 4) {
      encontroID4 = true;
    }
  }


  // Guardamos las detecciones dependiendo
  // del estado actual.

  if (estado == RECTO && encontroID2) {

    Serial.println("ID2 DETECTADO");

    estado = GIRO_INICIAL;

  }


  else if (estado == BUSCANDO_ID4 && encontroID4) {

    Serial.println("ID4 DETECTADO");

    estado = SEGUNDO_GIRO;

  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);


  // -----------------------------
  // Motor
  // -----------------------------

  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  pinMode(motorENA, OUTPUT);

  pararMotor();


  // -----------------------------
  // Servo
  // -----------------------------

  steeringServo.attach(SERVO_PIN);

  anguloServo = ANGULO_CENTRO;

  steeringServo.write(ANGULO_CENTRO);


  // -----------------------------
  // HuskyLens
  // -----------------------------

  Wire.begin();

  while (!huskylens.begin(Wire)) {

    Serial.println("Error conectando HuskyLens");

    delay(1000);
  }

  Serial.println("HuskyLens conectado");

  delay(500);
}


// =====================================================
// LOOP
// =====================================================

void loop() {


  // ===================================================
  // RECTO
  // ===================================================

  if (estado == RECTO) {

    avanzar(VEL_RECTA);

    leerHuskyLens();

  }


  // ===================================================
  // ID2 DETECTADO
  // ===================================================
  //
  // 1. PARAR
  // 2. GIRAR LLANTAS A 45°
  // 3. VOLVER A AVANZAR
  //
  // ===================================================

  else if (estado == GIRO_INICIAL) {

    Serial.println("Parando para iniciar giro");

    pararMotor();

    delay(100);


    Serial.println("Girando servo hasta 45°");

    moverServoSuave(ANGULO_PRIMER_GIRO);


    Serial.println("Servo en 45° -> avanzando");

    avanzar(VEL_GIRO);

    estado = BUSCANDO_ID4;
  }


  // ===================================================
  // BUSCAR ID4
  // ===================================================
  //
  // Seguimos avanzando con las ruedas a 45°.
  //
  // NO regresamos a 90°.
  //
  // ===================================================

  else if (estado == BUSCANDO_ID4) {

    avanzar(VEL_GIRO);

    leerHuskyLens();

  }


  // ===================================================
  // ID4 DETECTADO
  // ===================================================
  //
  // 1. PARAR
  // 2. GIRAR 20° MÁS
  //
  // 45° → 25°
  //
  // ===================================================

  else if (estado == SEGUNDO_GIRO) {

    Serial.println("ID4 -> parar");

    pararMotor();

    delay(100);


    Serial.println("Girando 20° adicionales");

    moverServoSuave(ANGULO_SEGUNDO_GIRO);


    Serial.println("Servo en 25°");

    tiempoEstado = millis();

    estado = MANTENER_GIRO;
  }


  // ===================================================
  // MANTENER 25°
  // ===================================================
  //
  // Avanza durante 300 ms con las llantas a 25°.
  //
  // ===================================================

  else if (estado == MANTENER_GIRO) {

    avanzar(VEL_GIRO);

    if (millis() - tiempoEstado >= TIEMPO_20_GRADOS) {

      Serial.println("300 ms terminados");

      pararMotor();

      estado = REGRESANDO;
    }
  }


  // ===================================================
  // REGRESAR AL CENTRO
  // ===================================================
  //
  // Ahora sí:
  //
  // 25° → 26° → 27° → ... → 90°
  //
  // ===================================================

  else if (estado == REGRESANDO) {

    Serial.println("Regresando suavemente a 90°");

    moverServoSuave(ANGULO_CENTRO);


    Serial.println("90° -> recto");

    estado = RECTO;

    avanzar(VEL_RECTA);
  }


  delay(5);
}