# Sistema de Estacionamiento

Sistema de control de acceso y monitoreo de estacionamientos desarrollado con **Arduino** y **Processing**.

## Descripción

El sistema utiliza tres sensores ultrasónicos **HC-SR04** para detectar vehículos y determinar la disponibilidad de dos espacios de estacionamiento. Un servomotor **SG90** controla la barrera de acceso.

Arduino procesa las mediciones y controla la barrera, mientras que Processing recibe los estados mediante comunicación serial y los muestra en una interfaz gráfica.

## Componentes

* Arduino Uno
* 3 × HC-SR04
* 1 × SG90
* Computador con Processing

## Funcionamiento

* Detecta vehículos en la zona de acceso.
* Determina si cada estacionamiento está libre u ocupado.
* Abre la barrera cuando hay un vehículo y al menos un espacio disponible.
* Cierra la barrera cuando el vehículo abandona la zona de detección.
* Reabre la barrera si se detecta un vehículo mientras se está cerrando.
* Envía los estados hacia Processing mediante comunicación serial.
* Muestra los estados de los estacionamientos y la barrera en una interfaz gráfica.

Si un sensor de plaza acumula tres lecturas consecutivas sin eco, su estado se informa como `OCUPADO`
y esa plaza no habilita la apertura. La barrera solo comienza a cerrar con una
lectura válida por encima de 5 cm; si el sensor de acceso falla durante el
cierre, vuelve a abrirse. La ausencia de eco no se considera una zona despejada.

Las mediciones se alternan en el orden acceso, plaza 1, acceso, plaza 2, con
60 ms de espera sin bloqueo entre lecturas. Cada lectura puede bloquear hasta
30 ms. Cada plaza mantiene un promedio móvil de las últimas diez lecturas,
actualizado con una sola medición por turno. Al iniciar se promedian las lecturas
disponibles. Uno o dos fallos consecutivos conservan el promedio sin añadir
la lectura inválida; el tercero reinicia el promedio y se trata como ocupado.
Una lectura válida reinicia el contador de fallos. Sin lecturas válidas previas,
la plaza se trata como ocupada; tras invalidar el promedio, la siguiente lectura
válida inicia uno nuevo.
El sensor de acceso utiliza la lectura directa para reaccionar sin esperar al
promedio. El filtrado suaviza las distancias de las plazas, pero retrasa los
cambios de ocupación; conviene comprobar los umbrales con el montaje físico.

## Comunicación

Arduino y Processing se comunican mediante el puerto serial a **9600 baudios**.

Los estados se envían con el siguiente formato:

```text
EST1:LIBRE;EST2:OCUPADO;PEAJE:ABIERTO
```

## Ejecución

### Arduino

1. Conectar el Arduino Uno al computador.
2. Abrir el archivo `.ino` en Arduino IDE.
3. Seleccionar la placa y el puerto correspondiente.
4. Cargar el programa en el Arduino.

### Processing

1. Abrir `Estacionamiento.pde` en Processing.
2. Verificar que Processing utilice el mismo puerto serial al que está conectado Arduino.
3. Ejecutar el sketch.

> El índice utilizado en `Serial.list()[0]` puede variar dependiendo de los puertos disponibles en el computador.
