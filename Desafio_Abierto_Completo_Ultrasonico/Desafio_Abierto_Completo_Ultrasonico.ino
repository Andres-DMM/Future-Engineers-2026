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

const int ANGULO_CENTRO = 89;

// Azul / ID2
const int ANGULO_PRIMER_GIRO = 43;

// Naranja / ID4
const int ANGULO_SEGUNDO_GIRO = 23;


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
// DISTANCIA MÍNIMA
// =====================================================

const int DISTANCIA_MINIMA = 15;


// =====================================================
// CORRECCIONES ULTRASÓNICAS
// =====================================================

// Si hay algo a la izquierda,
// giramos rápidamente hacia la derecha.
const int CORRECCION_DERECHA = 113;

// Si hay algo a la derecha,
// giramos rápidamente hacia la izquierda.
const int CORRECCION_IZQUIERDA = 67;


// =====================================================
// VELOCIDADES
// =====================================================

const int VEL_RECTA = 102;

const int VEL_GIRO = 65;

const int VEL_REVERSA = 70;


// =====================================================
// TIEMPOS
// =====================================================

// Tiempo máximo buscando naranja después del azul.
const unsigned long TIEMPO_BUSCAR_ID4 = 3000;

// Tiempo de recuperación.
const unsigned long TIEMPO_REVERSA = 1800;


// =====================================================
// ULTRASÓNICOS MÁS RÁPIDOS
// =====================================================

const unsigned long INTERVALO_ULTRASONICOS = 30;

unsigned long ultimoUltrasonico = 0;


// =====================================================
// ESTADOS
// =====================================================

enum Estado {

  RECTO,

  // ID2 azul detectado
  GIRO_INICIAL,

  // Servo a 45°, buscando ID4
  BUSCANDO_ID4,

  // ID4 naranja detectado
  SEGUNDO_GIRO,

  // Mantener 25°
  MANTENER_GIRO,

  // Regresar a 90°
  REGRESANDO,

  // No encontró ID4
  RECUPERACION
};

Estado estado = RECTO;


// =====================================================
// TIEMPO DEL ESTADO
// =====================================================

unsigned long tiempoEstado = 0;


// =====================================================
// MEDIR DISTANCIA
// =====================================================

long medirDistancia(int trigPin, int echoPin) {

  digitalWrite(trigPin, LOW);

  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);

  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duracion =
    pulseIn(echoPin, HIGH, 12000);

  // Sin eco
  if (duracion == 0) {
    return 999;
  }

  long distancia =
    duracion * 0.0343 / 2;

  return distancia;
}


// =====================================================
// CONTROL DE ULTRASÓNICOS
// =====================================================

void controlarUltrasonicos() {

  long distanciaIzq =
    medirDistancia(TRIG_IZQ, ECHO_IZQ);

  delayMicroseconds(300);

  long distanciaDer =
    medirDistancia(TRIG_DER, ECHO_DER);


  // Mostrar distancias cada 250 ms
  static unsigned long ultimoPrint = 0;

  if (millis() - ultimoPrint >= 250) {

    ultimoPrint = millis();

    Serial.print("IZQ: ");
    Serial.print(distanciaIzq);

    Serial.print(" cm | DER: ");
    Serial.print(distanciaDer);

    Serial.println(" cm");
  }


  // ===================================================
  // OBSTÁCULO A LA IZQUIERDA
  // ===================================================

  if (distanciaIzq < DISTANCIA_MINIMA &&
      distanciaDer >= DISTANCIA_MINIMA) {

    Serial.println(
      "OBSTACULO IZQ -> GIRANDO DERECHA"
    );

    steeringServo.write(CORRECCION_DERECHA);

    anguloServo = CORRECCION_DERECHA;

    return;
  }


  // ===================================================
  // OBSTÁCULO A LA DERECHA
  // ===================================================

  if (distanciaDer < DISTANCIA_MINIMA &&
      distanciaIzq >= DISTANCIA_MINIMA) {

    Serial.println(
      "OBSTACULO DER -> GIRANDO IZQUIERDA"
    );

    steeringServo.write(CORRECCION_IZQUIERDA);

    anguloServo = CORRECCION_IZQUIERDA;

    return;
  }


  // ===================================================
  // OBSTÁCULOS A LOS DOS LADOS
  // ===================================================

  if (distanciaIzq < DISTANCIA_MINIMA &&
      distanciaDer < DISTANCIA_MINIMA) {

    Serial.println(
      "OBSTACULO EN AMBOS LADOS"
    );

    steeringServo.write(90);

    anguloServo = 90;

    return;
  }


  // ===================================================
  // NO HAY OBSTÁCULOS -> CENTRAR SERVO
  // ===================================================

  if (anguloServo != ANGULO_CENTRO) {

    steeringServo.write(ANGULO_CENTRO);

    anguloServo = ANGULO_CENTRO;

    Serial.println("CAMINO LIBRE -> CENTRANDO SERVO");
  }
}


// =====================================================
// MOTOR - AVANZAR
// =====================================================

void avanzar(int velocidad) {

  digitalWrite(motorIN1, HIGH);

  digitalWrite(motorIN2, LOW);

  analogWrite(motorENA, velocidad);
}


// =====================================================
// MOTOR - REVERSA
// =====================================================

void reversa(int velocidad) {

  digitalWrite(motorIN1, LOW);

  digitalWrite(motorIN2, HIGH);

  analogWrite(motorENA, velocidad);
}


// =====================================================
// MOTOR - PARAR
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

    delay(4);
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


    // =================================================
    // ID2 = AZUL
    // =================================================

    if (resultado.ID == 2) {

      encontroID2 = true;
    }


    // =================================================
    // ID4 = NARANJA
    // =================================================

    if (resultado.ID == 4) {

      encontroID4 = true;
    }
  }


  // ===================================================
  // AZUL / ID2
  // ===================================================

  if (estado == RECTO && encontroID2) {

    Serial.println();
    Serial.println("==============================");
    Serial.println("ID2 AZUL DETECTADO");
    Serial.println("==============================");

    estado = GIRO_INICIAL;
  }


  // ===================================================
  // NARANJA / ID4
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

    Serial.println(
      "Error conectando HuskyLens"
    );

    delay(1000);
  }


  Serial.println();
  Serial.println("==============================");
  Serial.println("HUSKYLENS CONECTADO");
  Serial.println("2 HC-SR04 ACTIVOS");
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
    // ULTRASÓNICOS
    // -------------------------------------------------

    if (millis() - ultimoUltrasonico >=
        INTERVALO_ULTRASONICOS) {

      ultimoUltrasonico = millis();

      controlarUltrasonicos();
    }


    // -------------------------------------------------
    // AVANZAR
    // -------------------------------------------------

    avanzar(VEL_RECTA);


    // -------------------------------------------------
    // BUSCAR AZUL
    // -------------------------------------------------

    leerHuskyLens();
  }


  // ===================================================
  // AZUL / ID2
  // ===================================================

  else if (estado == GIRO_INICIAL) {

    Serial.println(
      "ID2 -> INICIANDO GIRO"
    );


    // Parar
    pararMotor();

    delay(100);


    // Girar servo
    Serial.println(
      "SERVO 90 -> 45"
    );

    moverServoSuave(
      ANGULO_PRIMER_GIRO
    );


    // Empezar contador
    tiempoEstado = millis();


    Serial.println(
      "BUSCANDO ID4 DURANTE 3 SEGUNDOS"
    );


    // Avanzar con servo a 45°
    avanzar(VEL_GIRO);


    estado = BUSCANDO_ID4;
  }


  // ===================================================
  // BUSCANDO NARANJA / ID4
  // ===================================================

  else if (estado == BUSCANDO_ID4) {


    // Mantener 45°
    steeringServo.write(
      ANGULO_PRIMER_GIRO
    );

    anguloServo =
      ANGULO_PRIMER_GIRO;


    // Avanzar
    avanzar(VEL_GIRO);


    // Buscar ID4
    leerHuskyLens();


    // -------------------------------------------------
    // Si todavía no encontró ID4,
    // revisar los 3 segundos.
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
  // NARANJA / ID4
  // ===================================================

  else if (estado == SEGUNDO_GIRO) {

    Serial.println(
      "ID4 -> SEGUNDO GIRO"
    );


    pararMotor();

    delay(100);


    Serial.println(
      "SERVO 45 -> 25"
    );

    moverServoSuave(
      ANGULO_SEGUNDO_GIRO
    );


    tiempoEstado = millis();


    estado = MANTENER_GIRO;
  }


  // ===================================================
  // MANTENER SEGUNDO GIRO
  // ===================================================

  else if (estado == MANTENER_GIRO) {


    // Mantener exactamente 25°
    steeringServo.write(
      ANGULO_SEGUNDO_GIRO
    );

    anguloServo =
      ANGULO_SEGUNDO_GIRO;


    avanzar(VEL_GIRO);


    // Mantener 300 ms
    if (millis() - tiempoEstado >= 300) {

      Serial.println(
        "SEGUNDO GIRO TERMINADO"
      );


      pararMotor();


      estado = REGRESANDO;
    }
  }


  // ===================================================
  // REGRESAR A CENTRO
  // ===================================================

  else if (estado == REGRESANDO) {

    Serial.println(
      "REGRESANDO SERVO A 90"
    );


    moverServoSuave(
      ANGULO_CENTRO
    );


    Serial.println(
      "CONTINUANDO RECTO"
    );


    estado = RECTO;


    avanzar(VEL_RECTA);
  }


  // ===================================================
  // RECUPERACIÓN
  // ===================================================

  else if (estado == RECUPERACION) {

    Serial.println();
    Serial.println("==============================");
    Serial.println("RECUPERACION");
    Serial.println("RETROCEDIENDO HACIA LA DERECHA");
    Serial.println("==============================");


    // -------------------------------------------------
    // PARAR
    // -------------------------------------------------

    pararMotor();

    delay(100);


    // -------------------------------------------------
    // GIRAR DIRECCIÓN HACIA DERECHA
    // -------------------------------------------------

    moverServoSuave(115);


    // -------------------------------------------------
    // REVERSA
    // -------------------------------------------------

    reversa(VEL_REVERSA);


    unsigned long inicioReversa =
      millis();


    // -------------------------------------------------
    // RETROCEDER 1.8 SEGUNDOS
    // -------------------------------------------------

    while (millis() - inicioReversa <
           TIEMPO_REVERSA) {


      // Mantener dirección hacia derecha
      steeringServo.write(115);

      anguloServo = 115;

      delay(2);
    }


    // -------------------------------------------------
    // PARAR
    // -------------------------------------------------

    pararMotor();

    delay(100);


    // -------------------------------------------------
    // CENTRAR
    // -------------------------------------------------

    Serial.println(
      "CENTRANDO SERVO"
    );

    moverServoSuave(
      ANGULO_CENTRO
    );


    // -------------------------------------------------
    // CONTINUAR
    // -------------------------------------------------

    Serial.println(
      "RECUPERACION TERMINADA"
    );


    estado = RECTO;

    avanzar(VEL_RECTA);
  }


  delay(2);
}