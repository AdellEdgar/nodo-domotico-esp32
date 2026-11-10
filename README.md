# Nodo domótico de habitación con ESP32

Nodo domótico basado en un **ESP32 DevKit V1** con **FreeRTOS** (framework ESP-IDF).
Mide temperatura, humedad, luz y presencia, y controla la iluminación (LED RGB)
y una persiana (servo). Se gestiona desde un dashboard de Node-RED, una app
Android, una consola UART y gestos reconocidos con TinyML.

Proyecto de las asignaturas de **IoT** y **Redes y Dispositivos Móviles** (UJI).

## Estado

| Parte | Estado |
|---|---|
| Entorno (PlatformIO + ESP-IDF) | ✅ |
| LED RGB con PWM (LEDC) | ✅ probado |
| Pulsador con ISR y antirrebote | ✅ |
| Resto de sensores y actuadores | ⏳ |
| Servidor Docker (MQTT, Node-RED, InfluxDB) | ⏳ |
| TinyML (gestos) | ⏳ |
| App Android | ⏳ |

## Estructura del repositorio

| Carpeta | Contenido |
|---|---|
| `proyecto/` | Firmware del ESP32 (proyecto PlatformIO, ESP-IDF) |
| `servidor/` | Docker Compose: Mosquitto, Node-RED e InfluxDB |
| `ml/` | Captura de gestos y entrenamiento del modelo |
| `app/` | App Android |
| `documentos/` | Esquemas, medidas e imágenes |

## Hardware y conexiones

ESP32 DevKit V1 sobre una placa de expansión de 30 pines (jumper de V en 3,3 V).

| Componente | Pin del ESP32 | Notas |
|---|---|---|
| LED RGB: rojo / verde / azul | GPIO25 / GPIO26 / GPIO27 | Ánodo común (común a 3V3), PWM invertido |
| Pulsador | GPIO32 | Pull-up interna, el otro pin a GND |
| Servo SG90 (persiana) | GPIO13 | 5 V desde la placa de expansión (alimentada por USB-C aparte) |
| PIR HC-SR501 | GPIO33 | VCC a 5 V, salida de 3,3 V. Tiempo y sensibilidad al mínimo, jumper en H |
| Sensor de luz BH1750 (I2C, 0x23) | SDA GPIO21, SCL GPIO22 | VCC a 3,3 V. Sustituye al LDR analógico |

## Compilar y subir

Requisitos: VSCode con la extensión PlatformIO.

1. Abrir la carpeta `proyecto/` en VSCode.
2. Compilar con **Build** (✓) y subir con **Upload** (→).
3. Abrir el monitor serie (115200 baudios) con **Serial Monitor**.

## Firmware

| Fichero | Contenido |
|---|---|
| `config.h` | Pines y constantes |
| `pulsador.c/.h` | Pulsador: ISR → timer de antirrebote (50 ms) → semáforo binario |
| `main.c` | Arranque: servicio de ISR, semáforos, módulos y tareas |
| `led_rgb.c/.h` | LED RGB por PWM (LEDC, 5 kHz, 8 bits, salida invertida) |
| `servo.c/.h` | Servo por PWM (LEDC, 50 Hz, 16 bits, pulso 0,5-2,5 ms) |
| `pir.c/.h` | PIR: ISR en flanco de subida → semáforo binario `sPresencia` |
| `i2c_bus.c/.h` | Bus I2C compartido (400 kHz) y escáner |
| `bh1750.c/.h` | Sensor de luz BH1750: medida continua en lux |
