# OmniSense Dongle — DS5Dongle Advanced Edition

Un fork de [DS5Dongle](https://github.com/loteran/DS5Dongle) para Raspberry Pi Pico 2W que lleva la experiencia del DualSense a otro nivel. Hereda todas las características del firmware original y añade funciones avanzadas para personalizar tu mando sin necesidad de soporte nativo en los juegos.

---

## Características principales

Además de las funciones base del fork de loteran, como Auto Hápticas por audio, este firmware añade:

- **Perfiles:** Selecciona entre 4 perfiles para configurar gatillos y luces al detalle.
- **Modos de Barra de Luz:** Controla el color de la barra de luz con múltiples modos y 4 ranuras de colores favoritos personalizados.
- **Modos de Gatillo:** Toma el control total de los gatillos adaptativos incluso en juegos sin soporte. Añade modos **Rígido**, **Disparo** y **Ametralladora**.
- **Atajos desde el mando:** Selecciona rápidamente entre los 4 perfiles. Cambia la configuración de gatillos y luces. Ajusta el volúmen del parlante/auriculares y ganancia háptica directamente desde el Control.
- **Configuración Web:** Gestiona todas las funciones mediante una aplicación web personalizada.
- **No se necesitan aplicaciones complementarias:** Todas las funciones se manejan desde el dongle, sin necesidad de nada corriendo en el host con una excepción: se requiere redireccionar el audio del host al control si no usas auriculares (aplicación en desarrollo).

---

## Configuración y Web-App

1. Accede a la aplicación web [aquí](https://nightggan.github.io/OmniSense-Config-Web/)
2. Conecta tu dongle vía USB y enlaza el control DualSense.
3. Haz clic en **"Conectar"** y ajusta tus preferencias.

---

## Modo Configuración

Para entrar en modo configuración desde tu mando, presiona el botón **Mute** (Silencio).
- **Modo Configuración:** El botón Mute tendrá un efecto de **respiración**.
- **Modo Normal:** El botón Mute estará apagado.

Los cambios se guardan en flash al salir del modo. Los atajos son bloqueados y no llegan al host.

### Controles en modo Configuración:

| Botón | Función | Nivel |
| :--- | :--- | :--- |
| **Create** | Cambiar modo de la Barra de Luz | Por perfil |
| **Options** | Activar/Desactivar efecto de respiración | Por perfil |
| **L1** | Cambiar modo del Gatillo Izquierdo | Por perfil |
| **R1** | Cambiar modo del Gatillo Derecho | Por perfil |
| **Stick Izquierdo Arriba** | Aumentar volumen (Parlante/Auriculares) | Global |
| **Stick Izquierdo Abajo** | Disminuir volumen (Parlante/Auriculares) | Global |
| **Stick Derecho Arriba** | Aumentar ganancia háptica | Global |
| **Stick Izquierdo Abajo** | Disminuir ganancia háptica | Global |
| **Cuadrado** | Mute (Parlante/Auriculares) | Global |
| **DPAD** | Cambiar entre perfiles |   |

Mute manual no se mantiene entre ciclos de encendido pero puedes bajar el volumen a 0 con el **Stick Izquierdo** el cual sí se mantiene entre encendidos.

---

### Configuración global

Se pueden cambiar de manera global las siguientes opciones independiente del perfil seleccionado.

- Configuración de hápticas basadas en audio
- Volumen de parlantes/auriculares
- Perfil de emulación (DS - DSE - Auto)
- Frecuencia de respuesta (Polling Rate)
- Inactividad
- Atajo de apagado
- Despertar host
- Led del Dongle

### Perfiles

Cada perfil tiene las siguientes opciones que son independientes y personalizables entre cada perfil.

- Modo de funcionamiento de gatillos izquierdo y derecho
- Configuración de cada modo del gatillo para afinar a gusto del usuario
- Modo de funcionamiento de la barra de luz
    - 4 Espacios de colores favoritos por perfil
- Efecto respiración

---

### Modos de gatillo por perfil:

| Modo | Función |
| :--- | :--- |
| 0 | Controlado por el host. Compatibilidad con juegos que envian datos para el gatillo |
| 1 | Resistencia continua |
| 2 | Gatillo simple |
| 3 | Metralleta / Vibración continua

### Modos de la barra de luz por perfil:

| Modo | Función |
| :--- | :--- |
| 0 | Controlado por el Host |
| 1 | Apagada |
| 2 | Favorito 0 |
| 3 | Favorito 1 |
| 4 | Favorito 2 |
| 5 | Favorito 3 |
| 6 | Indicador de batería |
| 7 | Arcoíris |
| 8 | Desvanecimiento entre favoritos |

### Modo 0 Host:

Si 2 host envian datos diferentes (ej: Steam y GamePass) el dongle prioriza el primer color válido recibido a menos que el primer host deje de enviar datos.

### Indicador de Batería:

| Porcentaje | Color |
| :--- | :--- |
| > 40% | Verde |
| 10% - 39% | Amarillo |
| < 10% | Desvanecimiento rápido en rojo |

- Si la batería está crítica (<10%) se reemplaza cualquier modo seleccionado para alertar continuamente.
- Si el control está cargando se indica con un desvanecimiento en verde reemplazando cualquier otro modo aunque la batería esté en niveles críticos.

---

## Work in Progress (Próximas funciones)

- Compatibilidad total con el micrófono integrado con su atajo dedicado desde el mando.
- Atajo desde el mano para cambiar los modos de auto hápticas

---

## Cambios del fork original

- Se quitó atajo para desactivar el touchpad
- Se cambiaron los valores por defecto a los que me parecían mejor

---

## Créditos

Este proyecto es un fork de **[DS5Dongle](https://github.com/loteran/DS5Dongle)**. Todo el mérito del desarrollo base y la arquitectura de memoria es de sus creadores originales.

---

## Proyectos acreditados

Gracias a los siguientes proyectos, sin ellos esto no sería posible.

- Proyecto original por awalol **[DS5Dongle](https://github.com/awalol/DS5Dongle)**
- Audio Auto-Haptics por loteran **[DS5Dongle - Auto Haptics Edition](https://github.com/loteran/DS5Dongle)**
- OLED Edition por MarcelineVPQ **[DS5Dongle-OLED-Edition](https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition)**



*Licensed under the **MIT License**.*