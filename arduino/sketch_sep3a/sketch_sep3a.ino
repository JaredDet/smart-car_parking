#include <Servo.h>

// Pines
const int TRIGGER_PEAJE = 13;
const int ECHO_PEAJE = 11;
const int SERVO_PIN = 2;

const int TRIGGER_ESTACIONAMIENTO_1 = 10;
const int ECHO_ESTACIONAMIENTO_1 = 9;

const int TRIGGER_ESTACIONAMIENTO_2 = 7;
const int ECHO_ESTACIONAMIENTO_2 = 5;

// Distancias
const float DISTANCIA_MINIMA_CM = 1.0;
const float DISTANCIA_MAXIMA_CM = 20.0;
const float DISTANCIA_OCUPADO_CM = 10.0;

const float FACTOR_CONVERSION_CM = 58.0;
const int CANTIDAD_MEDICIONES = 10;

const unsigned long TIMEOUT_ECHO_US = 30000;
const unsigned long INTERVALO_ENVIO_MS = 500;
const unsigned long TIEMPO_MOVIMIENTO_MS = 1000;

unsigned long ultimoEnvio = 0;
unsigned long inicioMovimiento = 0;

const int ANGULO_CERRADO = 0;
const int ANGULO_ABIERTO = 90;

enum EstadoPeaje {
  CERRADO,
  ABRIENDO,
  ABIERTO,
  CERRANDO
};

EstadoPeaje estadoActual = CERRADO;

Servo servo;

float medirDistanciaUltrasonido(int triggerPin, int echoPin) {

  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(triggerPin, LOW);

  long tiempoEcho = pulseIn(
    echoPin,
    HIGH,
    TIMEOUT_ECHO_US
  );

  if (tiempoEcho == 0) {
    return -1;
  }

  return tiempoEcho / FACTOR_CONVERSION_CM;
}

float medirDistanciaPromedio(int triggerPin, int echoPin) {

  float suma = 0;
  int medicionesValidas = 0;

  for (int i = 0; i < CANTIDAD_MEDICIONES; i++) {

    float distancia =
      medirDistanciaUltrasonido(
        triggerPin,
        echoPin
      );

    if (distancia >= 0) {
      suma += distancia;
      medicionesValidas++;
    }

    delay(10);
  }

  if (medicionesValidas == 0) {
    return -1;
  }

  return suma / medicionesValidas;
}

bool vehiculoDetectado(float distancia) {

  return (
    distancia >= DISTANCIA_MINIMA_CM &&
    distancia <= DISTANCIA_MAXIMA_CM
  );
}

bool estacionamientoOcupado(float distancia) {

  return (
    distancia >= 0 &&
    distancia < DISTANCIA_OCUPADO_CM
  );
}

bool hayEstacionamientoLibre(
  float distancia1,
  float distancia2
) {

  return (
    !estacionamientoOcupado(distancia1) ||
    !estacionamientoOcupado(distancia2)
  );
}

void enviarEstado(
  float distanciaEstacionamiento1,
  float distanciaEstacionamiento2
) {

  Serial.print("EST1:");

  if (estacionamientoOcupado(distanciaEstacionamiento1)) {
    Serial.print("OCUPADO");
  } else {
    Serial.print("LIBRE");
  }

  Serial.print(";EST2:");

  if (estacionamientoOcupado(distanciaEstacionamiento2)) {
    Serial.print("OCUPADO");
  } else {
    Serial.print("LIBRE");
  }

  Serial.print(";PEAJE:");

  switch (estadoActual) {

    case CERRADO:
      Serial.println("CERRADO");
      break;

    case ABRIENDO:
      Serial.println("ABRIENDO");
      break;

    case ABIERTO:
      Serial.println("ABIERTO");
      break;

    case CERRANDO:
      Serial.println("CERRANDO");
      break;
  }
}

void setup() {

  Serial.begin(9600);

  pinMode(TRIGGER_PEAJE, OUTPUT);
  pinMode(ECHO_PEAJE, INPUT);
  digitalWrite(TRIGGER_PEAJE, LOW);

  pinMode(TRIGGER_ESTACIONAMIENTO_1, OUTPUT);
  pinMode(ECHO_ESTACIONAMIENTO_1, INPUT);
  digitalWrite(TRIGGER_ESTACIONAMIENTO_1, LOW);

  pinMode(TRIGGER_ESTACIONAMIENTO_2, OUTPUT);
  pinMode(ECHO_ESTACIONAMIENTO_2, INPUT);
  digitalWrite(TRIGGER_ESTACIONAMIENTO_2, LOW);

  servo.attach(SERVO_PIN);
  servo.write(ANGULO_CERRADO);
}

void loop() {

  float distanciaPeaje =
    medirDistanciaPromedio(
      TRIGGER_PEAJE,
      ECHO_PEAJE
    );

  float distanciaEstacionamiento1 =
    medirDistanciaPromedio(
      TRIGGER_ESTACIONAMIENTO_1,
      ECHO_ESTACIONAMIENTO_1
    );

  float distanciaEstacionamiento2 =
    medirDistanciaPromedio(
      TRIGGER_ESTACIONAMIENTO_2,
      ECHO_ESTACIONAMIENTO_2
    );

  switch (estadoActual) {

    case CERRADO:

      if (vehiculoDetectado(distanciaPeaje)) {

        if (hayEstacionamientoLibre(
              distanciaEstacionamiento1,
              distanciaEstacionamiento2
            )) {

          servo.write(ANGULO_ABIERTO);

          inicioMovimiento = millis();
          estadoActual = ABRIENDO;
        }
      }

      break;

    case ABRIENDO:

      if (millis() - inicioMovimiento >= TIEMPO_MOVIMIENTO_MS) {
        estadoActual = ABIERTO;
      }

      break;

    case ABIERTO:

      if (!vehiculoDetectado(distanciaPeaje)) {

        servo.write(ANGULO_CERRADO);

        inicioMovimiento = millis();
        estadoActual = CERRANDO;
      }

      break;

    case CERRANDO:

      if (vehiculoDetectado(distanciaPeaje)) {

        servo.write(ANGULO_ABIERTO);

        inicioMovimiento = millis();
        estadoActual = ABRIENDO;

      } else if (millis() - inicioMovimiento >= TIEMPO_MOVIMIENTO_MS) {

        estadoActual = CERRADO;
      }

      break;
  }

  if (millis() - ultimoEnvio >= INTERVALO_ENVIO_MS) {

    enviarEstado(
      distanciaEstacionamiento1,
      distanciaEstacionamiento2
    );

    ultimoEnvio = millis();
  }

  delay(50);
}