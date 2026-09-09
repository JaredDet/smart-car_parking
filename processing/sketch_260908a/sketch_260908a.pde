import processing.serial.*;

Serial puertoSerial;

String estadoEstacionamiento1 = "DESCONOCIDO";
String estadoEstacionamiento2 = "DESCONOCIDO";
String estadoPeaje = "CERRADO";

void setup() {

  size(900, 650);

  println(Serial.list());

  puertoSerial = new Serial(this, Serial.list()[0], 9600);
  puertoSerial.bufferUntil('\n');

  textFont(createFont("Arial", 24));
}

void draw() {

  background(35);

  // Título
  fill(255);
  textAlign(CENTER);
  textSize(36);
  text("ESTACIONAMIENTO", width / 2, 60);

  stroke(90);
  strokeWeight(2);
  line(80, 90, width - 80, 90);

  dibujarEspacio(
    100,
    130,
    "ESPACIO 1",
    estadoEstacionamiento1
  );

  dibujarEspacio(
    500,
    130,
    "ESPACIO 2",
    estadoEstacionamiento2
  );

  dibujarCabina(
    300,
    390,
    estadoPeaje
  );

  dibujarMensaje();
}

void dibujarEspacio(
  int x,
  int y,
  String nombre,
  String estado
) {

  stroke(80);
  strokeWeight(2);
  fill(45);
  rect(x, y, 300, 200, 15);

  fill(255);
  textAlign(CENTER);
  textSize(24);
  text(nombre, x + 150, y + 45);

  boolean ocupado = estado.equals("OCUPADO");

  if (ocupado) {
    fill(220, 60, 60);
  } else {
    fill(60, 200, 100);
  }

  noStroke();
  ellipse(x + 150, y + 105, 55, 55);

  fill(255);
  textSize(30);

  if (ocupado) {
    text("X", x + 150, y + 116);
  } else {
    text("✓", x + 150, y + 116);
  }

  fill(220);
  textSize(20);

  if (ocupado) {
    text("OCUPADO", x + 150, y + 160);
  } else {
    text("LIBRE", x + 150, y + 160);
  }
}

void dibujarCabina(
  int x,
  int y,
  String estado
) {

  stroke(80);
  strokeWeight(2);
  fill(45);
  rect(x, y, 300, 150, 15);

  fill(255);
  textAlign(CENTER);
  textSize(24);
  text("CABINA", x + 150, y + 40);

  if (estado.equals("ABIERTA")) {

    fill(60, 200, 100);

  } else if (
    estado.equals("ABRIENDO") ||
    estado.equals("CERRANDO")
  ) {

    fill(240, 160, 50);

  } else {

    fill(220, 60, 60);
  }

  noStroke();
  ellipse(x + 80, y + 95, 35, 35);

  fill(230);
  textSize(21);
  text(estado, x + 190, y + 102);
}

void dibujarMensaje() {

  boolean espacioLibre =
    estadoEstacionamiento1.equals("LIBRE") ||
    estadoEstacionamiento2.equals("LIBRE");

  String mensaje;

  if (!espacioLibre) {

    mensaje = "ESTACIONAMIENTO LLENO";

  } else if (
    estadoPeaje.equals("ABRIENDO") ||
    estadoPeaje.equals("CERRANDO")
  ) {

    if (estadoPeaje.equals("ABRIENDO")) {
      mensaje = "ESPERE MIENTRAS LA CABINA SE ABRE";
    } else {
      mensaje = "ESPERE MIENTRAS LA CABINA SE CIERRA";
    }

  } else if (estadoPeaje.equals("ABIERTA")) {

    mensaje = "PASE";

  } else {

    mensaje = "ESPERE MIENTRAS LA CABINA SE ABRE";
  }

  stroke(80);
  strokeWeight(2);
  fill(45);
  rect(150, 570, 600, 55, 12);

  fill(255);
  textAlign(CENTER);
  textSize(22);
  text(mensaje, width / 2, 606);
}

void serialEvent(Serial puerto) {

  String mensaje = puerto.readStringUntil('\n');

  if (mensaje == null) {
    return;
  }

  mensaje = trim(mensaje);

  if (mensaje.startsWith("EST1:")) {

    estadoEstacionamiento1 =
      mensaje.substring(5);

  } else if (mensaje.startsWith("EST2:")) {

    estadoEstacionamiento2 =
      mensaje.substring(5);

  } else if (mensaje.startsWith("PEAJE:")) {

    estadoPeaje =
      mensaje.substring(6);
  }
}
