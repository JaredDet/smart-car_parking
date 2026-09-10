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
const float DISTANCIA_MAXIMA_CM = 5.0;
const float DISTANCIA_OCUPADO_CM = 10.0;

const float FACTOR_CONVERSION_CM = 58.0;
const int CANTIDAD_MEDICIONES = 10;
const int MAX_FALLOS_CONSECUTIVOS = 3;
const unsigned long INTERVALO_MEDICION_MS = 60;

const unsigned long TIMEOUT_ECHO_US = 30000;
const unsigned long INTERVALO_ENVIO_MS = 500;
const unsigned long TIEMPO_MOVIMIENTO_MS = 1000;

unsigned long ultimoEnvio = 0;
unsigned long inicioMovimiento = 0;
unsigned long ultimaMedicion = 0;
int turnoMedicion = 0;
float distanciaPeaje = -1;
float distanciaEstacionamiento1 = -1;
float distanciaEstacionamiento2 = -1;

struct PromedioMovil {
  float lecturas[CANTIDAD_MEDICIONES] = {};
  int siguiente = 0;
  int cantidad = 0;
  int fallosConsecutivos = 0;
  float suma = 0;

  float actualizar(float distancia) {
    // Tolera fallos aislados sin incluirlos en el promedio.
    // Tras varios fallos, descarta los datos antiguos y cuenta como ocupado.
    if (distancia < 0) {
      if (fallosConsecutivos < MAX_FALLOS_CONSECUTIVOS) {
        fallosConsecutivos++;
      }
      if (fallosConsecutivos < MAX_FALLOS_CONSECUTIVOS && cantidad > 0) {
        return suma / cantidad;
      }
      siguiente = 0;
      cantidad = 0;
      suma = 0;
      return -1;
    }

    fallosConsecutivos = 0;
    if (cantidad == CANTIDAD_MEDICIONES) {
      suma -= lecturas[siguiente];
    } else {
      cantidad++;
    }

    lecturas[siguiente] = distancia;
    suma += distancia;
    siguiente = (siguiente + 1) % CANTIDAD_MEDICIONES;
    return suma / cantidad;
  }
};

PromedioMovil promedioEstacionamiento1;
PromedioMovil promedioEstacionamiento2;

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

void actualizarMediciones() {

  if (millis() - ultimaMedicion < INTERVALO_MEDICION_MS) {
    return;
  }

  // Alterna acceso, plaza 1, acceso, plaza 2 para priorizar la barrera.
  // Una lectura por turno; las plazas filtran fallos aislados y el acceso no.
  if (turnoMedicion == 0 || turnoMedicion == 2) {
    distanciaPeaje = medirDistanciaUltrasonido(TRIGGER_PEAJE, ECHO_PEAJE);
  } else if (turnoMedicion == 1) {
    distanciaEstacionamiento1 = promedioEstacionamiento1.actualizar(medirDistanciaUltrasonido(
      TRIGGER_ESTACIONAMIENTO_1, ECHO_ESTACIONAMIENTO_1
    ));
  } else {
    distanciaEstacionamiento2 = promedioEstacionamiento2.actualizar(medirDistanciaUltrasonido(
      TRIGGER_ESTACIONAMIENTO_2, ECHO_ESTACIONAMIENTO_2
    ));
  }

  ultimaMedicion = millis();
  turnoMedicion = (turnoMedicion + 1) % 4;
}

bool vehiculoDetectado(float distancia) {

  return (
    distancia >= DISTANCIA_MINIMA_CM &&
    distancia <= DISTANCIA_MAXIMA_CM
  );
}

bool estacionamientoOcupado(float distancia) {

  // Fail-safe: una lectura invalida (sensor sin eco) se trata como ocupado,
  // para no reportar ni asumir nunca una plaza "desconocida" como libre.
  return (
    distancia < 0 ||
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
  Serial.print(estacionamientoOcupado(distanciaEstacionamiento1) ? "OCUPADO" : "LIBRE");

  Serial.print(";EST2:");
  Serial.print(estacionamientoOcupado(distanciaEstacionamiento2) ? "OCUPADO" : "LIBRE");

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

  actualizarMediciones();

  switch (estadoActual) {

    case CERRADO:

      if (
        vehiculoDetectado(distanciaPeaje) &&
        hayEstacionamientoLibre(
          distanciaEstacionamiento1,
          distanciaEstacionamiento2
        )
      ) {

        servo.write(ANGULO_ABIERTO);

        inicioMovimiento = millis();
        estadoActual = ABRIENDO;
      }

      break;

    case ABRIENDO:

      if (millis() - inicioMovimiento >= TIEMPO_MOVIMIENTO_MS) {
        estadoActual = ABIERTO;
      }

      break;

    case ABIERTO:

      // Una lectura fallida o demasiado cercana no confirma una zona despejada.
      if (distanciaPeaje > DISTANCIA_MAXIMA_CM) {

        servo.write(ANGULO_CERRADO);

        inicioMovimiento = millis();
        estadoActual = CERRANDO;
      }

      break;

    case CERRANDO:

      // Reabre tambien si falla el sensor o si la lectura es demasiado cercana.
      if (distanciaPeaje <= DISTANCIA_MAXIMA_CM) {

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

}
