[English](https://github.com/Nightggan/OmniSense-Dongle/blob/master/README.md)
# OmniSense Dongle — DS5Dongle Advanced Edition

Un fork de [DS5Dongle](https://github.com/loteran/DS5Dongle) para la Raspberry Pi Pico 2W que desbloquea todo el potencial de tu control DualSense en cualquier sistema.

Esta Edición Avanzada hereda todas las excelentes características del firmware original —como el Audio Auto-Haptics— y añade nuevas y potentes formas de personalizar tus gatillos, iluminación y rendimiento a nivel de hardware, **no necesidad de soporte nativo en el juego**.

---

## 🌟 Características Clave

- **Gatillos Adaptativos a Nivel de Hardware:** Toma el control total de tus gatillos en *cualquier* juego. Elige entre modos como Resistencia, Clic de Arma, Ametralladora y el nuevo Gatillo Sensible (Hair Trigger).
- **Apuntado por Giroscopio Universal a Nivel de Hardware:** Toma el control del joystick derecho usando el movimiento del giroscopio, activado por el botón de tu elección. Funciona perfectamente en cualquier juego sin importar el soporte nativo, incluyendo variables totalmente ajustables para personalizar la sensibilidad y respuesta según tu estilo de juego.
- **Perfiles Personalizados:** Guarda hasta 4 perfiles diferentes para cambiar fácilmente entre configuraciones de gatillo e iluminación para distintos juegos.
- **Configuración al Vuelo:** Usa atajos de botones del control para cambiar perfiles, ajustar modos de gatillo/barra de luz, modificar el volumen y cambiar la ganancia háptica sin tocar tu PC.
- **Control Avanzado de la Barra de Luz:** Personaliza el brillo de tu control con múltiples modos dinámicos y 4 ranuras de colores favoritos personalizados.
- **Interfaz Basada en Web:** Gestiona fácilmente todas tus configuraciones avanzadas visualmente a través de una aplicación WebHID personalizada.
- **Cero Aplicaciones en Segundo Plano:** Todo funciona directamente en el dongle. No necesitas software ejecutándose en segundo plano en tu PC. *(Nota: Actualmente se está desarrollando una aplicación ligera específicamente para enrutar el audio de la PC al control si no usas auriculares, pero es totalmente opcional).*

---

## 🚀 Primeros Pasos

Configurar tu dongle es sencillo y se realiza directamente en tu navegador:

1. Ve al [Configurador Web de OmniSense](https://nightggan.github.io/OmniSense-Config-Web/).
2. Conecta tu dongle Raspberry Pi Pico 2W mediante USB y vincula tu control DualSense.
3. Haz clic en **"Connect"** (Conectar) en la aplicación web y ¡comienza a personalizar!

---

## 🎮 Modo de Configuración (Atajos al Vuelo)

No siempre necesitas la aplicación web para realizar cambios. Puedes entrar al **Modo de Configuración** directamente desde tu control presionando el botón **Mute** (Silenciar).

- **Modo Configuración ACTIVADO:** El botón Mute parpadeará lentamente (efecto de respiración). Los atajos están activos y no se enviarán a tu PC/juego.
- **Modo Configuración DESACTIVADO:** El botón Mute estará apagado. El control funciona normalmente.

*Cualquier cambio realizado aquí se guarda instantáneamente en la memoria flash del dongle al salir del Modo de Configuración.*

### Atajos del Control

| Botón | Acción | Alcance |
| :--- | :--- | :--- |
| **Create (Share)** | Ciclar Modo de Barra de Luz | Guardado en Perfil |
| **Options** | Alternar Efecto de Respiración de Barra de Luz | Guardado en Perfil |
| **L1** | Ciclar Modo de Gatillo Izquierdo | Guardado en Perfil |
| **R1** | Ciclar Modo de Gatillo Derecho | Guardado en Perfil |
| **Stick Izquierdo Arriba/Abajo** | Aumentar/Disminuir Volumen (Altavoz y Auriculares / Host Windows) | Global |
| **Stick Derecho Arriba/Abajo** | Aumentar/Disminuir Ganancia Háptica | Global |
| **Cuadrado** | Silenciar Audio (Altavoz y Auriculares / Host Windows) | Global |
| **Círculo** | Poner el Host en suspensión (Windows) | Global |
| **Equis** | Cambiar Vibración a Modo Háptico | Global |
| **Triángulo** | Apagar el control | Global |
| **D-PAD (Cualquier dir.)** | Ciclar entre Perfiles 0 a 3 | Global |

> **Consejo Pro:** El estado de Silencio rápido (Cuadrado) se restablece cuando apagas el control. Si deseas silenciar el audio de forma permanente, usa el Stick Izquierdo hacia abajo para bajar el volumen al 0%, ya que los niveles de volumen se guardan permanentemente.

---

## ⚙️ Resumen de Configuraciones

### Configuraciones Globales
Estas configuraciones se aplican al dongle universalmente, sin importar qué perfil esté activo:
- Configuración y Ganancia de Háptica basada en Audio
- Mezcla de la señal de vibración clásica con Háptica basada en Audio
- Volumen Maestro de Altavoz/Auriculares
- Perfil de Emulación USB (DualSense, DualSense Edge o Auto)
- Tasa de sondeo USB (Polling Rate)
- Tiempo de espera de desconexión por inactividad y activación del host
- Activación del atajo de apagado
- Opción para controlar el volumen del Host
- Opción para poner en suspensión al host mediante atajo de botón.

### Configuraciones de Perfil
Tienes **4 perfiles distintos**, cada uno almacenando su propia configuración única para:
- Modos de operación de gatillo Izquierdo y Derecho
- Parámetros ajustados para cada modo de gatillo
- Modo de activación para mapear el giroscopio del control al joystick analógico derecho
- Parámetros predeterminados ajustados para movimientos de apuntado preciso
- Modo de operación de la barra de luz
- 4 ranuras de Colores Favoritos Personalizados
- Interruptor de animación de respiración

---

## 🔫 Modos de Gatillo Adaptativo

| Modo | Nombre | Descripción |
| :--- | :--- | :--- |
| **0** | **Host-Controlled** | Passthrough nativo. Ideal para juegos que soportan oficialmente el DualSense. |
| **1** | **Resistance** | Aplica rigidez constante durante todo el recorrido del gatillo. |
| **2** | **Weapon Click** | Simula el "chasquido" táctil o clic al disparar un arma. |
| **3** | **Machine Gun** | Proporciona retroceso y vibración continuos al presionarlo. |
| **4** | **Hair Trigger** | El gatillo llega a un tope rígido a mitad de camino, enviando inmediatamente una señal de presión completa (100%) con mínimo esfuerzo. Ideal para shooters. |
| **5** | **Rumble Trigger** | Envía la señal de vibración clásica a los gatillos. |

---

## 💡 Modos de Barra de Luz

| Modo | Nombre | Descripción |
| :--- | :--- | :--- |
| **0** | **Host-Controlled** | Iluminación nativa del juego. *(Si múltiples fuentes envían datos de color, el dongle se bloquea en la primera señal válida).* |
| **1** | **Off** | Desactiva la barra de luz por completo. |
| **2-5**| **Favorites (0-3)** | Muestra uno de tus 4 colores personalizados definidos en la aplicación web. |
| **6** | **Battery Level** | Indicador visual de batería (Ver tabla abajo). |
| **7** | **Rainbow** | Cicla a través de todos los colores. |
| **8** | **Fade** | Transiciona suavemente entre tus 4 colores favoritos. |

### Indicador de Nivel de Batería (Modo 6)

| Nivel de Carga | Color y Comportamiento |
| :--- | :--- |
| **Cargando** | Desvanecimiento verde suave (Anula todos los demás modos) |
| **> 40%** | Verde sólido |
| **10% - 39%** | Amarillo sólido |
| **< 10%** | Parpadeo rojo rápido (Anula todos los demás modos para advertirte) |

---

## ⚙️ Configuraciones en Linux

El sistema de auto-háptica necesita el perfil pro-audio que se puede configurar manualmente en el panel de control de audio o ejecutando este comando (una sola vez):

```bash
pactl set-card-profile alsa_card.usb-Sony_Interactive_Entertainment_DualSense_Wireless_Controller-00 pro-audio
```

Si notas algo inusual después de una actualización de Linux o del firmware del Dongle, ten en cuenta que a veces la caché del perfil de audio del dispositivo se corrompe y necesita ser verificada ejecutando el comando anterior o re-activando el perfil pro-audio manualmente en el panel de control de audio.

---

## 📝 Cambios respecto al Fork Original

- Se eliminó el atajo para desactivar el panel táctil.

- Se revisaron los valores de parámetros predeterminados para ofrecer una mejor experiencia inicial.

## 🙌 Créditos y Reconocimientos

Este proyecto es un fork de **[DS5Dongle](https://github.com/loteran/DS5Dongle)**. Muchísimas gracias a los creadores originales por la base y la arquitectura de memoria que hicieron esto posible.

Por favor, echa un vistazo a su increíble trabajo:
- La arquitectura original por awalol: **[DS5Dongle](https://github.com/awalol/DS5Dongle)**
- La implementación de Audio Auto-Haptics por loteran: **[DS5Dongle - Auto Haptics Edition](https://github.com/loteran/DS5Dongle)**
- La integración OLED por MarcelineVPQ: **[DS5Dongle-OLED-Edition](https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition)**

*Licenciado bajo la Licencia **MIT**.*