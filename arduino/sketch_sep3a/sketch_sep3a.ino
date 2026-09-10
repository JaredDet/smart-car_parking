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
const float DISTANCIA_OCUPADO_CM = 30.0;

// Mediciones
const float FACTOR_CONVERSION_CM = 58.0;
const int CANTIDAD_MEDICIONES = 10;
const int MAX_FALLOS_CONSECUTIVOS = 3;

// Tiempos
const unsigned long TIEMPO_DETECCION_MS = 300;
const unsigned long TIEMPO_DESPEJADO_MS = 300;

const unsigned long INTERVALO_MEDICION_MS = 60;
const unsigned long TIMEOUT_ECHO_US = 30000;
const unsigned long INTERVALO_ENVIO_MS = 500;
const unsigned long TIEMPO_MOVIMIENTO_MS = 1000;

const int ANGULO_CERRADO = 0;
const int ANGULO_ABIERTO = 90;

const float VELOCIDAD_SERVO_GRADOS_MS =
  (float)(ANGULO_ABIERTO - ANGULO_CERRADO) /
  TIEMPO_MOVIMIENTO_MS;

// Temporizadores
unsigned long ultimoEnvio = 0;
unsigned long inicioMovimiento = 0;
unsigned long ultimaMedicion = 0;

unsigned long inicioDeteccion = 0;
unsigned long inicioDespejado = 0;

// Movimiento del servo
int anguloInicialMovimiento = ANGULO_CERRADO;
unsigned long tiempoMovimientoActual = 0;

// Mediciones
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
    if (distancia < 0) {
      if (fallosConsecutivos < MAX_FALLOS_CONSECUTIVOS) {
        fallosConsecutivos++;
      }

      if (
        fallosConsecutivos < MAX_FALLOS_CONSECUTIVOS &&
        cantidad > 0
      ) {
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

enum EstadoPeaje {
  CERRADO,
  ABRIENDO,
  ABIERTO,
  CERRANDO
};

EstadoPeaje estadoActual = CERRADO;

Servo servo;

float medirDistanciaUltrasonido(
  int triggerPin,
  int echoPin
) {
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
  if (
    millis() - ultimaMedicion <
    INTERVALO_MEDICION_MS
  ) {
    return;
  }

  if (
    turnoMedicion == 0 ||
    turnoMedicion == 2
  ) {
    distanciaPeaje =
      medirDistanciaUltrasonido(
        TRIGGER_PEAJE,
        ECHO_PEAJE
      );
  } else if (turnoMedicion == 1) {
    distanciaEstacionamiento1 =
      promedioEstacionamiento1.actualizar(
        medirDistanciaUltrasonido(
          TRIGGER_ESTACIONAMIENTO_1,
          ECHO_ESTACIONAMIENTO_1
        )
      );
  } else {
    distanciaEstacionamiento2 =
      promedioEstacionamiento2.actualizar(
        medirDistanciaUltrasonido(
          TRIGGER_ESTACIONAMIENTO_2,
          ECHO_ESTACIONAMIENTO_2
        )
      );
  }

  ultimaMedicion = millis();
  turnoMedicion =
    (turnoMedicion + 1) % 4;
}

bool vehiculoDetectado(float distancia) {
  return (
    distancia >= DISTANCIA_MINIMA_CM &&
    distancia <= DISTANCIA_MAXIMA_CM
  );
}

bool estacionamientoOcupado(float distancia) {
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

void iniciarMovimiento(int anguloObjetivo) {
  anguloInicialMovimiento = servo.read();

  int diferencia =
    abs(
      anguloObjetivo -
      anguloInicialMovimiento
    );

  if (diferencia == 0) {
    servo.write(anguloObjetivo);

    if (anguloObjetivo == ANGULO_ABIERTO) {
      estadoActual = ABIERTO;
    } else {
      estadoActual = CERRADO;
    }

    inicioDeteccion = 0;
    inicioDespejado = 0;

    return;
  }

  tiempoMovimientoActual =
    (unsigned long)(
      diferencia /
      VELOCIDAD_SERVO_GRADOS_MS
    );

  inicioMovimiento = millis();

  inicioDeteccion = 0;
  inicioDespejado = 0;

  if (anguloObjetivo == ANGULO_ABIERTO) {
    estadoActual = ABRIENDO;
  } else {
    estadoActual = CERRANDO;
  }
}

void actualizarMovimientoServo() {
  if (
    estadoActual != ABRIENDO &&
    estadoActual != CERRANDO
  ) {
    return;
  }

  unsigned long tiempoTranscurrido =
    millis() - inicioMovimiento;

  int anguloObjetivo =
    estadoActual == ABRIENDO
      ? ANGULO_ABIERTO
      : ANGULO_CERRADO;

  if (
    tiempoTranscurrido >=
    tiempoMovimientoActual
  ) {
    servo.write(anguloObjetivo);
    return;
  }

  int angulo = map(
    tiempoTranscurrido,
    0,
    tiempoMovimientoActual,
    anguloInicialMovimiento,
    anguloObjetivo
  );

  servo.write(angulo);
}

void actualizarEstadoPeaje() {
  switch (estadoActual) {

    case CERRADO:
      if (vehiculoDetectado(distanciaPeaje)) {

        if (inicioDeteccion == 0) {
          inicioDeteccion = millis();
        }

        if (
          millis() - inicioDeteccion >=
          TIEMPO_DETECCION_MS
        ) {
          if (
            hayEstacionamientoLibre(
              distanciaEstacionamiento1,
              distanciaEstacionamiento2
            )
          ) {
            inicioDeteccion = 0;
            iniciarMovimiento(ANGULO_ABIERTO);
          } else {
            inicioDeteccion = 0;
          }
        }

      } else {
        inicioDeteccion = 0;
      }
      break;

    case ABRIENDO:
      if (
        millis() - inicioMovimiento >=
        tiempoMovimientoActual
      ) {
        servo.write(ANGULO_ABIERTO);
        estadoActual = ABIERTO;

        inicioDeteccion = 0;
        inicioDespejado = 0;
      }
      break;

    case ABIERTO:
      if (distanciaPeaje > DISTANCIA_MAXIMA_CM) {

        if (inicioDespejado == 0) {
          inicioDespejado = millis();
        }

        if (
          millis() - inicioDespejado >=
          TIEMPO_DESPEJADO_MS
        ) {
          inicioDespejado = 0;
          iniciarMovimiento(ANGULO_CERRADO);
        }

      } else {
        inicioDespejado = 0;
      }
      break;

    case CERRANDO:
      if (
        millis() - inicioMovimiento >=
        tiempoMovimientoActual
      ) {
        servo.write(ANGULO_CERRADO);
        estadoActual = CERRADO;

        inicioDeteccion = 0;
        inicioDespejado = 0;
      }
      break;
  }
}

void enviarEstado() {
  Serial.print("EST1:");
  Serial.print(
    estacionamientoOcupado(
      distanciaEstacionamiento1
    )
      ? "OCUPADO"
      : "LIBRE"
  );

  Serial.print(";EST2:");
  Serial.print(
    estacionamientoOcupado(
      distanciaEstacionamiento2
    )
      ? "OCUPADO"
      : "LIBRE"
  );

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

void actualizarComunicacion() {
  if (
    millis() - ultimoEnvio >=
    INTERVALO_ENVIO_MS
  ) {
    enviarEstado();
    ultimoEnvio = millis();
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIGGER_PEAJE, OUTPUT);
  pinMode(ECHO_PEAJE, INPUT);
  digitalWrite(TRIGGER_PEAJE, LOW);

  pinMode(
    TRIGGER_ESTACIONAMIENTO_1,
    OUTPUT
  );
  pinMode(
    ECHO_ESTACIONAMIENTO_1,
    INPUT
  );
  digitalWrite(
    TRIGGER_ESTACIONAMIENTO_1,
    LOW
  );

  pinMode(
    TRIGGER_ESTACIONAMIENTO_2,
    OUTPUT
  );
  pinMode(
    ECHO_ESTACIONAMIENTO_2,
    INPUT
  );
  digitalWrite(
    TRIGGER_ESTACIONAMIENTO_2,
    LOW
  );

  servo.attach(SERVO_PIN);
  servo.write(ANGULO_CERRADO);
}

void loop() {
  actualizarMovimientoServo();
  actualizarMediciones();
  actualizarEstadoPeaje();
  actualizarComunicacion();
}