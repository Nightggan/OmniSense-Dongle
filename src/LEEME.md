# OmniSense Dongle — DS5Dongle Advanced Edition

Un fork de [DS5Dongle](https://github.com/loteran/DS5Dongle) para Raspberry Pi Pico 2W que lleva la experiencia del DualSense a otro nivel. Hereda todas las características del firmware original y añade funciones avanzadas para personalizar tu mando sin necesidad de soporte nativo en los juegos.

---

## Características principales

Además de las funciones base del fork de loteran, como Auto Hápticas por audio, este firmware añade:

- **Modos de Barra de Luz:** Controla el color de la barra de luz con múltiples modos y 4 ranuras de colores favoritos personalizados.
- **Modos de Gatillo:** Toma el control total de los gatillos adaptativos incluso en juegos sin soporte. Añade modos **Rígido**, **Disparo** y **Ametralladora**.
- **Modo Configuración desde el mando:** Cambia los modos de la barra de luz, gatillos y volúmen del parlante directamente desde el DualSense.
- **Configuración Web:** Gestiona todas las funciones mediante una aplicación web personalizada.

---

## Configuración y Web-App

Para configurar tu OmniSense Dongle, utiliza la nueva Web-App ubicada en la carpeta `custom-config-web`.

1. Descarga el archivo de la Web-App y ábrelo en un navegador compatible (Chrome/Edge).
2. Conecta tu dongle vía USB.
3. Haz clic en **"Conectar"** y ajusta tus preferencias.

---

## Modo Configuración

Para entrar en modo configuración desde tu mando, presiona el botón **Mute** (Silencio).
- **Modo Configuración:** El botón Mute tendrá un efecto de **respiración**.
- **Modo Normal:** El botón Mute estará apagado.

Los cambios se guardan en flash al salir del modo. Los atajos son bloqueados y no llegan al host.

### Controles en modo Configuración:

| Botón | Función |
| :--- | :--- |
| **Create** | Cambiar modo de la Barra de Luz |
| **Options** | Activar/Desactivar efecto de respiración |
| **L1** | Cambiar modo del Gatillo Izquierdo |
| **R1** | Cambiar modo del Gatillo Derecho |
| **Stick Izquierdo Arriba** | Aumentar volumen (Parlante/Auriculares) |
| **Stick Izquierdo Abajo** | Disminuir volumen (Parlante/Auriculares) |

---

## Work in Progress (Próximas funciones)

- Compatibilidad total con el micrófono integrado con su atajo dedicado desde el mando.
- Atajo desde el mano para cambiar los modos de auto hápticas

---

## Créditos

Este proyecto es un fork de **[DS5Dongle](https://github.com/loteran/DS5Dongle)**. Todo el mérito del desarrollo base y la arquitectura de memoria es de sus creadores originales.

*Licenciado bajo la **MIT License**.*