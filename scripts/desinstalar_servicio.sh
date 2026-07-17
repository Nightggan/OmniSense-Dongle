#!/bin/bash

echo "=================================================="
echo " INICIANDO DESINSTALACIÓN DE AUDIO USB"
echo "=================================================="

# 1. Detener y deshabilitar los servicios activos primero
echo "Deteniendo y deshabilitando servicios..."

# Servicio de sistema global
sudo systemctl stop usb-audio-trigger.service 2>/dev/null
sudo systemctl disable usb-audio-trigger.service 2>/dev/null

# Servicio de usuario (Deck)
XDG_RUNTIME_DIR=/run/user/1000 systemctl --user stop usb-audio-mixer-serv.service 2>/dev/null
XDG_RUNTIME_DIR=/run/user/1000 systemctl --user disable usb-audio-mixer-serv.service 2>/dev/null

# 2. Desactivar temporalmente el modo de solo lectura
echo "Desbloqueando sistema de archivos de SteamOS..."
sudo steamos-readonly disable

echo "Eliminando archivos del sistema y de usuario..."

# 3. Eliminar Reglas de Udev
if [ -f "/etc/udev/rules.d/99-usb-search.rules" ]; then
    sudo rm /etc/udev/rules.d/99-usb-search.rules
    echo "[OK] Reglas de Udev eliminadas."
fi

# 4. Eliminar Servicio Puente (Sistema)
if [ -f "/etc/systemd/system/usb-audio-trigger.service" ]; then
    sudo rm /etc/systemd/system/usb-audio-trigger.service
    echo "[OK] Servicio Puente (Sistema) eliminado."
fi

# 5. Eliminar Servicio Mezclador (Usuario)
if [ -f "/home/deck/.config/systemd/user/usb-audio-mixer-serv.service" ]; then
    rm /home/deck/.config/systemd/user/usb-audio-mixer-serv.service
    echo "[OK] Servicio Mezclador (Usuario) eliminado."
fi

# 6. Eliminar Script Mezclador de PipeWire
if [ -f "/usr/local/bin/usb-audio-mixer.sh" ]; then
    sudo rm /usr/local/bin/usb-audio-mixer.sh
    echo "[OK] Script de PipeWire eliminado."
fi

# 7. Recargar configuraciones del sistema para limpiar la memoria
echo "Aplicando cambios y recargando daemons a su estado original..."

# Limpiar caché de Systemd (Sistema y Usuario)
sudo systemctl daemon-reload
XDG_RUNTIME_DIR=/run/user/1000 systemctl --user daemon-reload

# Reiniciar WirePlumber para que olvide el nombre fijo y recupere su política por defecto
XDG_RUNTIME_DIR=/run/user/1000 systemctl --user restart wireplumber

# Recargar udev y disparar eventos para soltar el hardware
sudo udevadm control --reload-rules
sudo udevadm trigger --action=add --subsystem-match=usb_device

# 8. Volver a activar el modo solo lectura
echo "Bloqueando sistema de archivos de SteamOS (Seguridad)..."
sudo steamos-readonly enable

echo "--------------------------------------------------"
echo "¡DESINSTALACIÓN FINALIZADA CON ÉXITO!"
echo "El sistema ha vuelto a su estado original de fábrica."
echo "=================================================="
