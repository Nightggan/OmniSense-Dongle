#!/bin/bash

ORIGEN="/home/deck/audio_mix"

echo "=================================================="
echo " INICIANDO RESTAURACIÃ“N AUTOMÃTICA DE AUDIO USB"
echo "=================================================="

# 1. Cambiar perfil de audio a Pro-Audio
echo "Cambiando perfil de audio del Dongle a Pro-Audio..."

pactl set-card-profile alsa_card.usb-Sony_Interactive_Entertainment_DualSense_Wireless_Controller-00 pro-audio

# 2. Verificar que la carpeta de respaldos exista
if [ ! -d "$ORIGEN" ]; then
    echo "[ERROR] No se encontrÃ³ la carpeta de respaldos en $ORIGEN"
    echo "AsegÃºrate de tener tus archivos respaldados antes de continuar."
    exit 1
fi

# 3. Asignar un nombre fijo al Dongle para Wireplumber
echo "Asignando un nombre fijo al Dongle para Wireplumber..."
if [ -f "$ORIGEN/51-ds5dongle.conf" ]; then
    sudo mkdir -p ~/.config/wireplumber/wireplumber.conf.d
    sudo cp "$ORIGEN/51-ds5dongle.conf" /home/deck/.config/wireplumber/wireplumber.conf.d/
    systemctl --user restart wireplumber
    echo "[OK] Nombre fijo asignado y servicio reiniciado."
else
    echo "[AVISO] No se encontrÃ³ 51-ds5dongle.conf en la carpeta."
fi

# 4. Desactivar temporalmente el modo de solo lectura de SteamOS
echo "Desbloqueando sistema de archivos de SteamOS..."
sudo steamos-readonly disable

echo "Copiando archivos a sus rutas originales..."

# 5. Restaurar Reglas de Udev
if [ -f "$ORIGEN/99-usb-search.rules" ]; then
    sudo cp "$ORIGEN/99-usb-search.rules" /etc/udev/rules.d/
    echo "[OK] Reglas de Udev restauradas."
else
    echo "[AVISO] No se encontrÃ³ 99-usb-search.rules en la carpeta."
fi

# 6. Restaurar Servicio Puente (Sistema)
if [ -f "$ORIGEN/usb-audio-trigger.service" ]; then
    sudo cp "$ORIGEN/usb-audio-trigger.service" /etc/systemd/system/
    echo "[OK] Servicio Puente (Sistema) restaurado."
else
    echo "[AVISO] No se encontrÃ³ usb-audio-trigger.service en la carpeta."
fi

# 7. Restaurar Servicio Mezclador (Usuario)
if [ -f "$ORIGEN/usb-audio-mixer-serv.service" ]; then
    mkdir -p /home/deck/.config/systemd/user/
    cp "$ORIGEN/usb-audio-mixer-serv.service" /home/deck/.config/systemd/user/
    echo "[OK] Servicio Mezclador (Usuario) restaurado."
else
    echo "[AVISO] No se encontrÃ³ usb-audio-mixer-serv.service en la carpeta."
fi

# 8. Restaurar Script Mezclador de PipeWire y dar permisos
if [ -f "$ORIGEN/usb-audio-mixer.sh" ]; then
    sudo cp "$ORIGEN/usb-audio-mixer.sh" /usr/local/bin/
    sudo chmod +x /usr/local/bin/usb-audio-mixer.sh
    echo "[OK] Script de PipeWire restaurado y con permisos de ejecuciÃ³n."
else
    echo "[AVISO] No se encontrÃ³ usb-audio-mixer.sh en la carpeta."
fi

# 9. Recargar configuraciones del sistema para aplicar cambios en caliente
echo "Aplicando cambios y recargando daemons..."

# A. Primero el sistema global y forzar el disparo de hardware como "add"
sudo systemctl daemon-reload
sudo udevadm control --reload-rules
sudo udevadm trigger --action=add --subsystem-match=usb_device

# B. SEGUNDO: Recarga limpia de la memoria de Systemd del usuario deck
sudo runuser -l deck -c "XDG_RUNTIME_DIR=/run/user/1000 systemctl --user daemon-reload"

# C. Habilitar el servicio de usuario de forma segura
sudo runuser -l deck -c "XDG_RUNTIME_DIR=/run/user/1000 systemctl --user enable usb-audio-mixer-serv.service"

# D. Habilitar el servicio de sistema global
sudo systemctl enable usb-audio-trigger.service

# 10. Volver a activar el modo solo lectura para proteger SteamOS
echo "Bloqueando sistema de archivos de SteamOS (Seguridad)..."
sudo steamos-readonly enable

echo "--------------------------------------------------"
echo "Â¡PROCESO FINALIZADO CON Ã‰XITO!"
echo "Mando avanzado y redirecciÃ³n de audio listos."
echo "=================================================="
