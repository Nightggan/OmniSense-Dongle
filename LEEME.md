[English](https://github.com/Nightggan/OmniSense-Dongle/blob/master/README.md)
# OmniSense Dongle — DS5Dongle Advanced Edition

Un fork de [DS5Dongle](https://github.com/loteran/DS5Dongle) para la Raspberry Pi Pico 2W que desbloquea todo el potencial de tu mando DualSense en cualquier sistema. 

Esta edición avanzada hereda todas las geniales características del firmware original —como las Auto Hápticas por audio— y añade nuevas y potentes formas de personalizar los gatillos, la iluminación y el rendimiento a nivel de hardware, **sin necesidad de soporte nativo en los juegos**.

---

## 🌟 Características Principales

- **Gatillos Adaptativos a Nivel de Hardware:** Toma el control total de tus gatillos en *cualquier* juego. Elige entre modos como Resistencia, Disparo, Metralleta y el nuevo Gatillo Sensible (Hair Trigger).
- **Puntería por Giroscopio Universal a Nivel de Hardware:** Toma el control del joystick derecho usando el movimiento de tu mando (giroscopio), activado con cualquier botón. Funciona en cualquier juego (incluso sin soporte nativo) y cuenta con variables totalmente ajustables para adaptar la sensibilidad y respuesta a tu estilo exacto.
- **Perfiles Personalizados:** Guarda hasta 4 perfiles diferentes para cambiar fácilmente entre configuraciones de gatillos e iluminación según el juego.
- **Configuración al Vuelo (On-the-Fly):** Usa atajos con los botones del mando para cambiar perfiles, ajustar modos de gatillo/barra de luz, modificar el volumen y cambiar la ganancia háptica sin tocar tu PC.
- **Control Avanzado de la Barra de Luz:** Personaliza el brillo de tu mando con múltiples modos dinámicos y 4 ranuras para tus colores favoritos.
- **Interfaz Web (Web UI):** Gestiona fácilmente todas tus configuraciones avanzadas de forma visual a través de una aplicación WebHID personalizada.
- **Cero Aplicaciones en Segundo Plano:** Todo se ejecuta directamente en el propio dongle. No necesitas ningún software corriendo en segundo plano en tu PC. *(Nota: Actualmente hay una aplicación ligera en desarrollo específicamente para redirigir el audio del PC al mando si no usas auriculares, pero es completamente opcional).*

---

## 🚀 Primeros Pasos

Configurar tu dongle es muy sencillo y se hace directamente desde el navegador:

1. Ve al [Configurador Web de OmniSense](https://nightggan.github.io/OmniSense-Config-Web/).
2. Conecta tu dongle Raspberry Pi Pico 2W por USB y empareja tu mando DualSense.
3. Haz clic en **"Conectar"** en la aplicación web ¡y empieza a personalizar!

---

## 🎮 Modo Configuración (Atajos al Vuelo)

No siempre necesitas la aplicación web para hacer cambios. Puedes entrar al **Modo Configuración** directamente desde tu mando presionando el botón **Mute** (Silencio).

- **Modo Configuración ON:** El botón Mute parpadeará lentamente (efecto respiración). Los atajos están activos y no se enviarán a tu PC o juego.
- **Modo Configuración OFF:** El botón Mute estará apagado. El mando se comporta con normalidad.

*Cualquier cambio realizado aquí se guarda instantáneamente en la memoria flash del dongle al salir del Modo Configuración.*

### Atajos del Mando

| Botón | Acción | Alcance |
| :--- | :--- | :--- |
| **Create (Share)** | Cambiar Modo de la Barra de Luz | Guardado en Perfil |
| **Options** | Activar/Desactivar Efecto Respiración | Guardado en Perfil |
| **L1** | Cambiar Modo del Gatillo Izquierdo | Guardado en Perfil |
| **R1** | Cambiar Modo del Gatillo Derecho | Guardado en Perfil |
| **Stick Izq. Arriba/Abajo** | Subir/Bajar Volumen (Altavoz y Auriculares) | Global |
| **Stick Der. Arriba/Abajo** | Subir/Bajar Ganancia Háptica | Global |
| **Cuadrado** | Silenciar Audio / Mute (Altavoz y Auriculares) | Global |
| **Círculo** | Suspender host (Windows) | Global |
| **Cruz** | Cambiar modo de Rumble en Háoticas por Audio | Global |
| **Triangle** | Apagar Control | Global |
| **D-PAD (Cualquier dir.)** | Alternar entre Perfiles 1 al 4 | Global |

> **Pro-Tip:** El silenciado rápido (Cuadrado) se reinicia al apagar el mando. Si quieres silenciar el audio de forma permanente, usa el Stick Izquierdo hacia Abajo para bajar el volumen al 0%, ya que los niveles de volumen sí se guardan en la memoria.

---

## ⚙️ Resumen de Configuraciones

### Configuraciones Globales
Estas opciones se aplican al dongle universalmente, independientemente del perfil que esté activo:
- Configuración y Ganancia de Hápticas por Audio
- Volumen Maestro del Altavoz/Auriculares
- Perfil de Emulación USB (DualSense, DualSense Edge o Auto)
- Tasa de Sondeo (Polling Rate) USB
- Tiempo de Apagado por Inactividad y Despertar Host
- Atajo de Botón de Apagado y encendido del LED del Dongle
- Activación de atajo para apagar control
- Opción para controlar volumen del host en lugare del control
- Opción para suspender host con atajo del control

### Configuraciones de Perfil
Tienes **4 perfiles distintos**, y cada uno guarda su propia configuración para:
- Modos de funcionamiento del Gatillo Izquierdo y Derecho
- Parámetros ajustados al detalle para cada modo de gatillo
- Modo de activación para mapeo de giroscopio a joystick derecho
- Parámetros por defecto para movimientos finos de puntería
- Modo de funcionamiento de la Barra de Luz
- 4 Espacios para Colores Favoritos personalizados
- Interruptor de animación de Respiración

---

## 🔫 Modos de Gatillos Adaptativos

| Modo | Nombre | Descripción |
| :--- | :--- | :--- |
| **0** | **Controlado por el Host** | Paso directo nativo. Ideal para juegos que soportan oficialmente el DualSense. |
| **1** | **Resistencia** | Aplica una rigidez constante durante todo el recorrido del gatillo. |
| **2** | **Disparo (Weapon Click)** | Simula el "clic" táctil al disparar un arma. |
| **3** | **Metralla (Machine Gun)** | Proporciona un retroceso y vibración continua al presionarlo. |
| **4** | **Gatillo Rápido (Hair Trigger)** | El gatillo choca con un muro rígido a mitad de recorrido, enviando inmediatamente una señal de presión del 100% con el mínimo esfuerzo. Excelente para shooters. |
| **5** | **Gatillo Rumble** | Envía la señal de vibración clásica a los gatillos. |

---

## 💡 Modos de la Barra de Luz

| Modo | Nombre | Descripción |
| :--- | :--- | :--- |
| **0** | **Controlado por el Host** | Iluminación nativa del juego. *(Si hay varias fuentes enviando datos de color, el dongle se bloquea en la primera señal válida).* |
| **1** | **Apagado** | Desactiva la barra de luz por completo. |
| **2-5**| **Favoritos (0-3)** | Muestra uno de tus 4 colores personalizados definidos en la web app. |
| **6** | **Nivel de Batería** | Indicador visual de batería (Ver tabla abajo). |
| **7** | **Arcoíris** | Pasa cíclicamente por todos los colores. |
| **8** | **Desvanecimiento** | Transición suave entre tus 4 colores favoritos. |

### Indicador de Nivel de Batería (Modo 6)

| Nivel de Carga | Color y Comportamiento |
| :--- | :--- |
| **Cargando** | Desvanecimiento suave en Verde (Sobrescribe los demás modos) |
| **> 40%** | Verde Sólido |
| **10% - 39%** | Amarillo Sólido |
| **< 10%** | Parpadeo rápido en Rojo (Sobrescribe los demás modos para avisarte) |

---

## 📝 Cambios respecto al Fork Original

- Se eliminó el atajo para desactivar el panel táctil (touchpad).
- Se reformularon los valores de los parámetros por defecto para ofrecer una mejor experiencia inicial al instalarlo.

---

## 🙌 Créditos y Agradecimientos

Este proyecto es un fork de **[DS5Dongle](https://github.com/loteran/DS5Dongle)**. Todo el mérito por el desarrollo base y la arquitectura de memoria es de sus creadores originales.

Agradecimientos especiales a los siguientes proyectos:
- La arquitectura original por awalol: **[DS5Dongle](https://github.com/awalol/DS5Dongle)**
- La implementación de Auto Hápticas por loteran: **[DS5Dongle - Auto Haptics Edition](https://github.com/loteran/DS5Dongle)**
- La integración OLED por MarcelineVPQ: **[DS5Dongle-OLED-Edition](https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition)**

*Licenciado bajo la **Licencia MIT**.*