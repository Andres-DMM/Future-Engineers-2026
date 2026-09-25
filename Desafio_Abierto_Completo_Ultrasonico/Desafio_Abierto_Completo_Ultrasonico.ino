#include <Wire.h>
#include <Servo.h>
#include "HUSKYLENS.h"

// =====================================================
// HUSKYLENS
// =====================================================

HUSKYLENS huskylens;


// =====================================================
// PINES DEL MOTOR
// =====================================================

const int motorIN1 = 2;
const int motorIN2 = 3;
const int motorENA = 5;


// =====================================================
// SERVO
// =====================================================

const int SERVO_PIN = 9;

Servo steeringServo;

int anguloServo = 90;

const int ANGULO_CENTRO = 90;

// Primer movimiento
const int ANGULO_PRIMER_GIRO = 45;

// Segundo movimiento
const int ANGULO_SEGUNDO_GIRO = 25;


// =====================================================
// ULTRASÓNICOS
// =====================================================

// HC-SR04 IZQUIERDO
const int TRIG_IZQ = 6;
const int ECHO_IZQ = 7;

// HC-SR04 DERECHO
const int TRIG_DER = 10;
const int ECHO_DER = 11;


// =====================================================
// DISTANCIAS
// =====================================================

// Distancia mínima antes de considerar que está
// demasiado cerca de una pared/objeto.
//
// Ajusta este valor según la pista.

const int DISTANCIA_MINIMA = 15;


// =====================================================
// VELOCIDADES
// =====================================================

const int VEL_RECTA = 102;

const int VEL_GIRO = 65;

const int VEL_REVERSA = 70;


// =====================================================
// TIEMPOS
// =====================================================

// Después de detectar ID2 tenemos aproximadamente
// 3 segundos para encontrar ID4.

const unsigned long TIEMPO_BUSCAR_ID4 = 3000;


// Cuánto tiempo retroceder cuando no encontramos ID4.

const unsigned long TIEMPO_REVERSA = 700;


// =====================================================
// ESTADOS
// =====================================================

enum Estado {

  RECTO,

  // ID2 detectado
  GIRO_INICIAL,

  // Ya está en 45° buscando ID4
  BUSCANDO_ID4,

  // ID4 detectado
  SEGUNDO_GIRO,

  // Mantener 25°
  MANTENER_GIRO,

  // Regresar a 90°
  REGRESANDO,

  // Recuperación porque no encontró ID4
  RECUPERACION

};

Estado estado = RECTO;


// =====================================================
// TIEMPOS
// =====================================================

unsigned long tiempoEstado = 0;


// =====================================================
// ULTRASÓNICOS
// =====================================================

long medirDistancia(int trigPin, int echoPin) {

  // Limpiar trigger
  digitalWrite(trigPin, LOW);

  delayMicroseconds(2);

  // Pulso de 10 us
  digitalWrite(trigPin, HIGH);

  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);


  // Medir eco
  long duracion =
    pulseIn(echoPin, HIGH, 25000);


  // Si no recibió eco
  if (duracion == 0) {

    return 999;
  }


  // Convertir a centímetros
  long distancia =
    duracion * 0.0343 / 2;


  return distancia;
}


// =====================================================
// CORRECCIÓN POR OBSTÁCULOS
// =====================================================
//
// Si está demasiado cerca del lado izquierdo:
//
//     pared
//       |
//       |  ROBOT
//       |    →
//       |
//
// Se corrige hacia la derecha.
//
//
//
// Si está demasiado cerca del lado derecho:
//
// se corrige hacia la izquierda.
//
// =====================================================

bool corregirObstaculo() {

  long distanciaIzq =
    medirDistancia(TRIG_IZQ, ECHO_IZQ);


  // Pequeño delay para no disparar ambos ultrasónicos
  // prácticamente al mismo tiempo.

  delayMicroseconds(500);


  long distanciaDer =
    medirDistancia(TRIG_DER, ECHO_DER);


  // Mostrar ocasionalmente las distancias
  static unsigned long ultimoPrint = 0;

  if (millis() - ultimoPrint > 250) {

    ultimoPrint = millis();

    Serial.print("Izq: ");
    Serial.print(distanciaIzq);

    Serial.print(" cm | Der: ");
    Serial.print(distanciaDer);

    Serial.println(" cm");
  }


  // ===================================================
  // DEMASIADO CERCA DE LA IZQUIERDA
  // ===================================================

  if (distanciaIzq < DISTANCIA_MINIMA) {

    Serial.println("MUY CERCA IZQUIERDA -> CORRIGIENDO DERECHA");


    // Girar dirección hacia la derecha
    steeringServo.write(115);

    anguloServo = 115;


    return true;
  }


  // ===================================================
  // DEMASIADO CERCA DE LA DERECHA
  // ===================================================

  if (distanciaDer < DISTANCIA_MINIMA) {

    Serial.println("MUY CERCA DERECHA -> CORRIGIENDO IZQUIERDA");


    // Girar dirección hacia la izquierda
    steeringServo.write(65);

    anguloServo = 65;


    return true;
  }


  return false;
}


// =====================================================
// MOTOR
// =====================================================

void avanzar(int velocidad) {

  digitalWrite(motorIN1, HIGH);

  digitalWrite(motorIN2, LOW);

  analogWrite(motorENA, velocidad);
}


// =====================================================
// REVERSA
// =====================================================

void reversa(int velocidad) {

  digitalWrite(motorIN1, LOW);

  digitalWrite(motorIN2, HIGH);

  analogWrite(motorENA, velocidad);
}


// =====================================================
// PARAR MOTOR
// =====================================================

void pararMotor() {

  analogWrite(motorENA, 0);
}


// =====================================================
// SERVO SUAVE
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

    delay(5);
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

    HUSKYLENSResult resultado =
      huskylens.read();


    // ID2 = AZUL

    if (resultado.ID == 2) {

      encontroID2 = true;
    }


    // ID4 = NARANJA

    if (resultado.ID == 4) {

      encontroID4 = true;
    }
  }


  // ===================================================
  // ID2
  // ===================================================

  if (estado == RECTO && encontroID2) {

    Serial.println();
    Serial.println("==============================");
    Serial.println("ID2 AZUL DETECTADO");
    Serial.println("==============================");


    estado = GIRO_INICIAL;
  }


  // ===================================================
  // ID4
  // ===================================================

  else if (estado == BUSCANDO_ID4 && encontroID4) {

    Serial.println();
    Serial.println("==============================");
    Serial.println("ID4 NARANJA DETECTADO");
    Serial.println("==============================");


    estado = SEGUNDO_GIRO;
  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);


  // ===================================================
  // MOTOR
  // ===================================================

  pinMode(motorIN1, OUTPUT);

  pinMode(motorIN2, OUTPUT);

  pinMode(motorENA, OUTPUT);

  pararMotor();


  // ===================================================
  // SERVO
  // ===================================================

  steeringServo.attach(SERVO_PIN);

  anguloServo = ANGULO_CENTRO;

  steeringServo.write(ANGULO_CENTRO);


  // ===================================================
  // ULTRASÓNICOS
  // ===================================================

  pinMode(TRIG_IZQ, OUTPUT);

  pinMode(ECHO_IZQ, INPUT);


  pinMode(TRIG_DER, OUTPUT);

  pinMode(ECHO_DER, INPUT);


  digitalWrite(TRIG_IZQ, LOW);

  digitalWrite(TRIG_DER, LOW);


  // ===================================================
  // HUSKYLENS
  // ===================================================

  Wire.begin();


  while (!huskylens.begin(Wire)) {

    Serial.println("Error conectando HuskyLens");

    delay(1000);
  }


  Serial.println();
  Serial.println("==============================");
  Serial.println("HuskyLens conectado");
  Serial.println("2 HC-SR04 activos");
  Serial.println("==============================");

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

    // -------------------------------------------------
    // Primero revisar obstáculos laterales
    // -------------------------------------------------

    bool huboCorreccion =
      corregirObstaculo();


    // -------------------------------------------------
    // Si no hubo obstáculo, mantener dirección recta
    // -------------------------------------------------

    if (!huboCorreccion) {

      steeringServo.write(ANGULO_CENTRO);

      anguloServo = ANGULO_CENTRO;
    }


    // -------------------------------------------------
    // Avanzar
    // -------------------------------------------------

    avanzar(VEL_RECTA);


    // -------------------------------------------------
    // Buscar ID2
    // -------------------------------------------------

    leerHuskyLens();
  }


  // ===================================================
  // ID2 DETECTADO
  // ===================================================
  //
  // 1. Parar
  // 2. Servo 90 → 45
  // 3. Avanzar
  // 4. Empezar temporizador de 3 segundos
  //
  // ===================================================

  else if (estado == GIRO_INICIAL) {

    Serial.println("ID2 -> iniciar giro");


    pararMotor();

    delay(100);


    Serial.println("Servo 90 -> 45");

    moverServoSuave(ANGULO_PRIMER_GIRO);


    // Empezar el temporizador
    tiempoEstado = millis();


    Serial.println("Buscando ID4 durante 3 segundos");


    avanzar(VEL_GIRO);


    estado = BUSCANDO_ID4;
  }


  // ===================================================
  // BUSCAR ID4
  // ===================================================
  //
  // El robot continúa avanzando con el servo a 45°.
  //
  // Si encuentra ID4:
  //
  //     SEGUNDO_GIRO
  //
  // Si pasan 3 segundos:
  //
  //     RECUPERACION
  //
  // ===================================================

  else if (estado == BUSCANDO_ID4) {

    avanzar(VEL_GIRO);


    // Buscar ID4
    leerHuskyLens();


    // -------------------------------------------------
    // Comprobar timeout
    // -------------------------------------------------

    if (estado == BUSCANDO_ID4) {

      if (millis() - tiempoEstado >=
          TIEMPO_BUSCAR_ID4) {

        Serial.println();
        Serial.println("==============================");
        Serial.println("NO SE ENCONTRO ID4");
        Serial.println("INICIANDO RECUPERACION");
        Serial.println("==============================");


        estado = RECUPERACION;
      }
    }
  }


  // ===================================================
  // ID4 DETECTADO
  // ===================================================
  //
  // 45° → 25°
  //
  // ===================================================

  else if (estado == SEGUNDO_GIRO) {

    Serial.println("ID4 -> segundo giro");


    pararMotor();

    delay(100);


    Serial.println("Servo 45 -> 25");

    moverServoSuave(ANGULO_SEGUNDO_GIRO);


    tiempoEstado = millis();


    estado = MANTENER_GIRO;
  }


  // ===================================================
  // MANTENER 25°
  // ===================================================

  else if (estado == MANTENER_GIRO) {

    avanzar(VEL_GIRO);


    if (millis() - tiempoEstado >= 300) {

      Serial.println("Segundo giro terminado");


      pararMotor();


      estado = REGRESANDO;
    }
  }


  // ===================================================
  // REGRESAR AL CENTRO
  // ===================================================

  else if (estado == REGRESANDO) {

    Serial.println("Regresando servo a 90");


    moverServoSuave(ANGULO_CENTRO);


    Serial.println("Continuando recto");


    estado = RECTO;


    avanzar(VEL_RECTA);
  }


  // ===================================================
  // RECUPERACIÓN
  // ===================================================
  //
  // No encontró ID4 después de ~3 segundos.
  //
  // Queremos:
  //
  //       PARAR
  //          ↓
  //       REVERSA
  //          ↓
  //  dirección hacia la derecha
  //          ↓
  //       retroceder
  //          ↓
  //       centrar
  //          ↓
  //       avanzar
  //
  // ===================================================

  else if (estado == RECUPERACION) {

    Serial.println();
    Serial.println("RECUPERACION");
    Serial.println("Retrocediendo hacia la derecha");


    // -------------------------------------------------
    // Parar
    // -------------------------------------------------

    pararMotor();

    delay(100);


    // -------------------------------------------------
    // Dirección hacia la DERECHA
    // -------------------------------------------------

    moverServoSuave(115);


    // -------------------------------------------------
    // Retroceder
    // -------------------------------------------------

    reversa(VEL_REVERSA);


    unsigned long inicioReversa =
      millis();


    // -------------------------------------------------
    // Retroceder durante el tiempo definido
    // -------------------------------------------------

    while (millis() - inicioReversa <
           TIEMPO_REVERSA) {

      // Mantener dirección hacia derecha

      steeringServo.write(115);

      delay(5);
    }


    // -------------------------------------------------
    // Parar
    // -------------------------------------------------

    pararMotor();

    delay(100);


    // -------------------------------------------------
    // Centrar dirección
    // -------------------------------------------------

    Serial.println("Centrando servo");

    moverServoSuave(ANGULO_CENTRO);


    // -------------------------------------------------
    // Volver a avanzar
    // -------------------------------------------------

    Serial.println("Recuperacion terminada");

    estado = RECTO;

    avanzar(VEL_RECTA);
  }


  delay(5);
}