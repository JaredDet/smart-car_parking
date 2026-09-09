import processing.serial.*;

Serial puertoSerial;

String estadoEstacionamiento1 = "DESCONOCIDO";
String estadoEstacionamiento2 = "DESCONOCIDO";
String estadoPeaje = "DESCONOCIDO";

final color FONDO = #121212;
final color PANEL = #202020;
final color BORDE = #3A3A3A;
final color TEXTO = #F5F5F5;
final color SECUNDARIO = #B0B0B0;
final color VERDE = #66AD85;
final color ROJO = #D98080;
final color AMBAR = #D0AD66;
final color GRIS = #A8A8A8;

void setup() {
  size(900, 700);
  println(Serial.list());
  if (Serial.list().length > 0) {
    puertoSerial = new Serial(this, Serial.list()[0], 9600);
    puertoSerial.bufferUntil('\n');
  }
  textFont(createFont("Arial", 24));
}

void draw() {
  background(FONDO);
  textAlign(LEFT);
  fill(TEXTO);
  textSize(32);
  text("Estacionamiento", 40, 55);
  fill(SECUNDARIO);
  textSize(16);
  text("Disponibilidad y control de acceso", 40, 83);

  textSize(18);
  text("PLAZAS DE ESTACIONAMIENTO", 40, 132);
  dibujarEspacio(40, 150, "PLAZA 1", estadoEstacionamiento1);
  dibujarEspacio(462, 150, "PLAZA 2", estadoEstacionamiento2);

  fill(SECUNDARIO);
  textAlign(LEFT);
  textSize(18);
  text("BARRERA DE ACCESO", 40, 415);
  dibujarBarrera(40, 433, estadoPeaje);
}

void dibujarPanel(int x, int y, int ancho, int alto) {
  stroke(BORDE);
  strokeWeight(1);
  fill(PANEL);
  rect(x, y, ancho, alto, 16);
}

color colorEstado(String estado) {
  if (estado.equals("LIBRE") || estado.equals("ABIERTO")) {
    return VERDE;
  }
  if (estado.equals("OCUPADO") || estado.equals("CERRADO")) {
    return ROJO;
  }
  if (estado.equals("ABRIENDO") || estado.equals("CERRANDO")) {
    return AMBAR;
  }
  return GRIS;
}

void dibujarEspacio(int x, int y, String nombre, String estado) {
  dibujarPanel(x, y, 398, 220);
  color acento = colorEstado(estado);

  noStroke();
  fill(acento);
  rect(x + 24, y + 24, 5, 24, 2);
  fill(TEXTO);
  textAlign(LEFT);
  textSize(18);
  text(nombre, x + 42, y + 43);

  // El símbolo y el texto permiten reconocer el estado sin depender del color.
  noFill();
  stroke(acento);
  strokeWeight(3);
  ellipse(x + 53, y + 111, 48, 48);
  if (estado.equals("LIBRE")) {
    line(x + 42, y + 111, x + 50, y + 119);
    line(x + 50, y + 119, x + 65, y + 102);
  } else if (estado.equals("OCUPADO")) {
    line(x + 45, y + 103, x + 61, y + 119);
    line(x + 61, y + 103, x + 45, y + 119);
  } else {
    fill(acento);
    textAlign(CENTER, CENTER);
    textSize(28);
    text("?", x + 53, y + 109);
  }

  textAlign(LEFT, BASELINE);
  fill(acento);
  textSize(26);
  text(estado, x + 92, y + 121);
  fill(SECUNDARIO);
  textSize(16);
  String detalle = "Sin una lectura válida del sensor";
  if (estado.equals("LIBRE")) {
    detalle = "Disponible para estacionar";
  } else if (estado.equals("OCUPADO")) {
    detalle = "Plaza ocupada por un vehículo";
  }
  text(detalle, x + 24, y + 187);
}

void dibujarBarrera(int x, int y, String estado) {
  dibujarPanel(x, y, 820, 225);
  noStroke();
  fill(colorEstado(estado));
  ellipse(x + 30, y + 32, 12, 12);
  fill(TEXTO);
  textAlign(LEFT);
  textSize(17);
  String etiqueta = estado;
  if (estado.equals("ABIERTO")) {
    etiqueta = "ABIERTA";
  } else if (estado.equals("CERRADO")) {
    etiqueta = "CERRADA";
  }
  text("Estado de la barrera: " + etiqueta, x + 48, y + 38);

  stroke(BORDE);
  strokeWeight(1);
  line(x + 24, y + 59, x + 796, y + 59);
  dibujarMensaje(x, y);
}

void dibujarMensaje(int x, int y) {
  boolean espacioLibre =
    estadoEstacionamiento1.equals("LIBRE") ||
    estadoEstacionamiento2.equals("LIBRE");
  boolean datosDesconocidos =
    estadoEstacionamiento1.equals("DESCONOCIDO") ||
    estadoEstacionamiento2.equals("DESCONOCIDO") ||
    estadoPeaje.equals("DESCONOCIDO");

  String mensaje;
  String detalle;
  color acento;

  if (datosDesconocidos) {
    mensaje = "ESPERANDO DATOS";
    detalle = "No se dispone de todos los estados del sistema.";
    acento = GRIS;
  } else if (!espacioLibre) {
    mensaje = "ESTACIONAMIENTO LLENO";
    detalle = "Espere a que quede una plaza disponible.";
    acento = ROJO;
  } else if (estadoPeaje.equals("ABRIENDO")) {
    mensaje = "ESPERE";
    detalle = "La barrera se está abriendo.";
    acento = AMBAR;
  } else if (estadoPeaje.equals("CERRANDO")) {
    mensaje = "ESPERE";
    detalle = "La barrera se está cerrando.";
    acento = AMBAR;
  } else if (estadoPeaje.equals("ABIERTO")) {
    mensaje = "PASE";
    detalle = "Barrera abierta. Avance hacia una plaza libre.";
    acento = VERDE;
  } else {
    mensaje = "ESPERE";
    detalle = "Deténgase ante la barrera hasta que se abra.";
    acento = ROJO;
  }

  textAlign(CENTER);
  fill(acento);
  textSize(32);
  text(mensaje, x + 410, y + 121);
  fill(SECUNDARIO);
  textSize(18);
  text(detalle, x + 410, y + 162);
}

void serialEvent(Serial puerto) {

  String mensaje = puerto.readStringUntil('\n');

  if (mensaje == null) {
    return;
  }

  mensaje = trim(mensaje);

  String[] campos = split(mensaje, ';');

  if (
    campos.length != 3 ||
    !campos[0].startsWith("EST1:") ||
    !campos[1].startsWith("EST2:") ||
    !campos[2].startsWith("PEAJE:")
  ) {
    return;
  }

  String estacionamiento1 = campos[0].substring(5);
  String estacionamiento2 = campos[1].substring(5);
  String peaje = campos[2].substring(6);

  if (
    !(estacionamiento1.equals("LIBRE") || estacionamiento1.equals("OCUPADO") ||
      estacionamiento1.equals("DESCONOCIDO")) ||
    !(estacionamiento2.equals("LIBRE") || estacionamiento2.equals("OCUPADO") ||
      estacionamiento2.equals("DESCONOCIDO")) ||
    !(peaje.equals("CERRADO") || peaje.equals("ABRIENDO") ||
      peaje.equals("ABIERTO") || peaje.equals("CERRANDO"))
  ) {
    return;
  }

  estadoEstacionamiento1 = estacionamiento1;
  estadoEstacionamiento2 = estacionamiento2;
  estadoPeaje = peaje;
}
