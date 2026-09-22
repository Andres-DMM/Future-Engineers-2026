#include "HUSKYLENS.h"
#include <Wire.h>

HUSKYLENS huskylens;

// --- IDs DE LA CÁMARA ---
const int ID_NARANJA = 1;
const int ID_AZUL = 2;
const int ID_VERDE = 3;
const int ID_ROJO = 4;

// --- PINES DEL DRV8833 ---
// IMPORTANTE: Los 4 deben ser pines PWM (con el símbolo ~ en la placa)
// Motor Tracción (Atrás)
const int IN1 = 9;  // Hacia adelante
const int IN2 = 10; // Hacia atrás
// Motor Dirección (Adelante)
const int IN3 = 5;  // Derecha
const int IN4 = 6;  // Izquierda

// --- VARIABLES DE ESTADO Y COMPETENCIA ---
int sentidoPista = 0; // 0 = Indefinido, 1 = Horario (Naranja), 2 = Antihorario (Azul)
int lineasGiradas = 0;
int vueltas = 0;
int estadoDireccion = 0; // 0 = Centro, 1 = Derecha, -1 = Izquierda

// --- CONFIGURACIÓN DE POTENCIA Y TIEMPOS ---
const int VEL_MAX = 250;      // Velocidad en rectas (0-255)
const int VEL_GIRO = 180;     // Velocidad para maniobras y esquinas (0-255)
const int VEL_PULSO = 200;    // Fuerza del pulso para enderezar dirección
const int TIEMPO_PULSO = 60;  // Milisegundos que dura el pulso para centrar (CALIBRAR)
const int TIEMPO_ESQUINA = 300; // Milisegundos que dura el giro en la esquina (CALIBRAR)

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Configurar pines de salida para los motores
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); 
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Inicialización de HuskyLens
  while (!huskylens.begin(Wire)) {
    Serial.println(F("Fallo I2C HuskyLens. Revisa conexiones."));
    delay(100);
  }
  
  // Arrancar el carro a máxima velocidad al iniciar el programa
  avanzar(VEL_MAX);
}

void loop() {
  // Solicitar datos a la cámara
  if (!huskylens.request()) return;

  if (huskylens.available()) {
    HUSKYLENSResult result = huskylens.read();

    // 1. ESTABLECER SENTIDO DE LA PISTA
    if (sentidoPista == 0) {
      if (result.ID == ID_NARANJA) {
        sentidoPista = 1; // Sentido Horario
      } else if (result.ID == ID_AZUL) {
        sentidoPista = 2; // Sentido Antihorario
      }
    }

    // 2. EVASIÓN DE OBSTÁCULOS
    if (result.ID == ID_ROJO) {
      // Bloque rojo: Esquivar por la izquierda
      girarIzquierda(VEL_GIRO);
    }
    else if (result.ID == ID_VERDE) {
      // Bloque verde: Esquivar por la derecha
      girarDerecha(VEL_GIRO);
    }
    
    // 3. GIROS EN LAS ESQUINAS
    else if ((result.ID == ID_NARANJA && sentidoPista == 1) || (result.ID == ID_AZUL && sentidoPista == 2)) {
      ejecutarGiroEsquina(sentidoPista);
    }
    
  } else {
    // 4. SI NO HAY NADA EN PANTALLA: Centrar dirección y acelerar
    centrarDireccion();
    avanzar(VEL_MAX);
  }
}

// --- FUNCIONES DE CONTROL MECÁNICO (DRV8833) ---

void avanzar(int pwm) {
  analogWrite(IN1, pwm);
  analogWrite(IN2, 0); // Al mandar 0 al otro pin, el motor gira en un sentido
}

void detenerTraccion() {
  analogWrite(IN1, 0);
  analogWrite(IN2, 0); // Ambos en 0 libera el motor (costeo)
  // Nota: si mandas 255 a ambos pines en el DRV8833, haces un freno en seco.
}

void girarDerecha(int pwm) {
  analogWrite(IN3, pwm);
  analogWrite(IN4, 0);
  estadoDireccion = 1; // Registramos que las llantas están a la derecha
}

void girarIzquierda(int pwm) {
  analogWrite(IN3, 0);
  analogWrite(IN4, pwm);
  estadoDireccion = -1; // Registramos que las llantas están a la izquierda
}

void centrarDireccion() {
  // Si las llantas ya están centradas, no hacemos nada
  if (estadoDireccion == 0) return; 

  // Si estaban a la derecha, damos un pulso hacia la izquierda
  if (estadoDireccion == 1) {
    analogWrite(IN3, 0);
    analogWrite(IN4, VEL_PULSO);
    delay(TIEMPO_PULSO); 
  } 
  // Si estaban a la izquierda, damos un pulso hacia la derecha
  else if (estadoDireccion == -1) {
    analogWrite(IN3, VEL_PULSO);
    analogWrite(IN4, 0);
    delay(TIEMPO_PULSO);
  }

  // Liberamos el motor de dirección para que quede suelto
  analogWrite(IN3, 0);
  analogWrite(IN4, 0);
  
  // Actualizamos el estado a centrado
  estadoDireccion = 0; 
}

void ejecutarGiroEsquina(int sentido) {
  if (sentido == 1) {
    girarDerecha(VEL_GIRO);
  } else if (sentido == 2) {
    girarIzquierda(VEL_GIRO);
  }
  
  // Tiempo que el carro mantiene el giro en la esquina
  delay(TIEMPO_ESQUINA); 

  // Conteo de líneas y vueltas
  lineasGiradas++;
  if (lineasGiradas >= 4) {
    vueltas++;
    lineasGiradas = 0; // Reiniciamos las líneas para la siguiente vuelta
    
    // Si se completan las 4 vueltas, frenar todo
    if (vueltas >= 4) {
      detenerTraccion();   // Apagar motor trasero
      centrarDireccion();  // Enderezar llantas delanteras
      while(true);         // Bucle infinito para terminar el round
    }
  }
}