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

### Instalación del Firmware

1. Descarga la última versión del firmware en la pestaña `Releases`
2. Presiona y sostén el botón `BOOTSEL` de la Raspberry y conectala a tu pc.
3. Aparecerá una nueva unidad en el explorador de archivos.
4. Arrastra el archivo `*.uf2` a la nueva unidad.
5. La Raspberry se desconectará y reiniciará con el nuevo firmware.
6. Coloca el control en modo `emparejamiento`.
7. El dongle detectará el control y se conectará automáticamente.

### Configuración

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

## ⚙️ Steam Deck / SteamOS

Si quieres enviar audio al dongle simultáneamente con tu salida de audio predeterminada en todo momento puedes usar este **[script](https://github.com/Nightggan/Audio-Mix-Steam-Deck/tree/main/OmniSense%20Timed%20Audio%20Mix)**. Esto le dará un nombre fijo al dongle y, cada vez que lo conectes, la Deck enviará audio a tu salida seleccionada al mismo tiempo que al dongle, para que puedas disfrutar de la retroalimentación háptica a la vez que audio en otro dispositivo. Además, configurará el perfil de audio del dongle a pro-audio, lo cual es necesario para que la retroalimentación háptica funcione correctamente en Linux.

1. Descarga el contenido de la carpeta **[OmniSense Timed Audio Mix](https://github.com/Nightggan/Audio-Mix-Steam-Deck/tree/main/OmniSense%20Timed%20Audio%20Mix)** en /home/deck/audio-mix o la carpeta de tu preferencia.
2. Navega a la carpeta que contiene los scripts, presiona el botón derecho y selecciona "Abrir terminal aquí" o presiona la combinación Alt+Shift+F4.
3. Asegúrate de tener una contraseña configurada para el usuario `deck`. 
    - Ejecuta `passwd`. 
    - Ingresa una nueva contraseña
    - Repite la nueva contraseña y confírmala.
4. Conecta el dongle y empareja el mando.
5. En la terminal ejecuta `sh ./timed_service_install.sh`.
6. El script te pedirá la contraseña de superusuario (Creada en el paso 3) e instalará el servicio.
7. Una vez finalizado el proceso, desconecta y conecta el dongle para que se apliquen los cambios.

Tendrás que hacer esto una sola vez, pero, después de una actualización, Steam podría sobrescribir los cambios, por lo que tendrás que ejecutar el script de nuevo (solo el paso 5).

---

## ⚙️ Corrección de errores

### Atajos del Host en Windows

Si luego de una actualización del firmware notas que los atajos de `Suspender Host`, `Despertar Host` o `Control de Volumen del Host` no funcionan como se espera sigue los siguientes pasos.

1. Descarga **[USB Deview](https://usbdeview.com)**
2. Descomprime el contenido en una carpeta.
3. Ejecuta `USBDeview.exe`
4. Ordena las columnas por `VendorID`
5. Selecciona todas las filas que digan en la columna `VendorID` `054c` y en la columna `ProductID` `0ce6`
    - Esto corresponde con los ID oficiales de Dualsense, los mismos que informa el Dongle
6. Presiona el `botón derecho del mouse` sobre la selección y pincha `Uninstall Selected Devices`
7. Desconecta y conecta el dongle
8. Sólo por ser Windows, nunca está demás un reinicio.

Esto eliminará el caché de dispositivos USB del SO lo que hará que lea nuevamente la descripción de reporte del Dongle y permitirá que Windows acepte los comandos enviados por los atajos.

### Actualización del firmware desde una versión anterior a 1.8.3

Antes de actualizar a una nueva versión del firmware primero flashea en el dongle el archivo `RP-008273-DS-3-flash_nuke.uf2` ubicado en la carpeta `utils`. Luego flashea la última versión del firmware siguiendo los pasos de siempre. Esto ayudará a limpiar cualquier basura o remanente de una versión anterior del firmware.

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