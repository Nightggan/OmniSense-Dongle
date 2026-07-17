#!/bin/bash

# Asegurar entorno de PipeWire y rutas básicas para el usuario deck
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export XDG_RUNTIME_DIR="/run/user/1000"
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/1000/bus"

# Función auxiliar para esperar dispositivos de forma síncrona y limpia (SIN -m)
esperar_dispositivo() {
    local tipo_puerto=$1  # '-o' para origen, '-i' para destino
    local nombre_puerto=$2
    local max_intentos=20
    local contador=0

    while ! pw-link "$tipo_puerto" | grep -q "$nombre_puerto"; do
        if [ $contador -ge $max_intentos ]; then
            return 1
        fi
        sleep 0.5
        contador=$((contador + 1))
    done
    return 0
}

DESTINO_ACTIVO="ds5_dongle_sink"
echo "[INFO] Usando la Raspberry Pi Pico (DS5Dongle) como destino."

# Esperar de forma segura a que el dongle sea indexado por PipeWire
echo "Esperando que el destino $DESTINO_ACTIVO esté disponible..."
if ! esperar_dispositivo "-i" "$DESTINO_ACTIVO"; then
    echo "[ERROR Crítico]: El destino $DESTINO_ACTIVO nunca apareció en PipeWire."
    exit 1
fi

echo "[INFO] Entrando en modo Guardián Persistente con soporte HDMI."

# BUCLE INTEGRAL
while true; do
    errores=0

    # A. Intentar enlace del Speaker de la Steam Deck
    if pw-link -o | grep -q "alsa_output.pci-0000_04_00.5-platform-acp5x_mach.0.HiFi__Speaker__sink"; then
        salida_error=$(pw-link alsa_output.pci-0000_04_00.5-platform-acp5x_mach.0.HiFi__Speaker__sink "$DESTINO_ACTIVO" 2>&1)
        codigo_salida=$?

        if [ $codigo_salida -ne 0 ] && ! echo "$salida_error" | grep -qi "existe"; then
            errores=$((errores + 1))
        fi
    fi

    # B. Intentar enlace a salida 3.5mm (Auriculares) de la Steam Deck
    if pw-link -o | grep -q "alsa_output.pci-0000_04_00.5-platform-acp5x_mach.0.HiFi__Headphones__sink"; then
        salida_error=$(pw-link alsa_output.pci-0000_04_00.5-platform-acp5x_mach.0.HiFi__Headphones__sink "$DESTINO_ACTIVO" 2>&1)
        codigo_salida=$?

        if [ $codigo_salida -ne 0 ] && ! echo "$salida_error" | grep -qi "existe"; then
            errores=$((errores + 1))
        fi
    fi

    # C. NUEVO: Intentar enlace de la salida HDMI (TV / Monitor)
    if pw-link -o | grep -q "alsa_output.pci-0000_04_00.1.hdmi-stereo-extra2"; then
        salida_error=$(pw-link alsa_output.pci-0000_04_00.1.hdmi-stereo-extra2 "$DESTINO_ACTIVO" 2>&1)
        codigo_salida=$?

        if [ $codigo_salida -ne 0 ] && ! echo "$salida_error" | grep -qi "existe"; then
            errores=$((errores + 1))
        fi
    fi

    # Control de salud del hardware: Si da error porque desconectaste el dongle físicamente, cerramos el servicio limpio
    if [ $errores -gt 0 ] && ! pw-link -i | grep -q "$DESTINO_ACTIVO"; then
        echo "[INFO] El destino $DESTINO_ACTIVO ya no responde en PipeWire. Cerrando guardián."
        exit 0
    fi

    # Pausa de 3 segundos entre barridos de mantenimiento
    sleep 3
done
