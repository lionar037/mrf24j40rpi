#!/bin/bash

# Directorios raíz
DIRECTORIOS=("src" "include")

# Comando para detectar cambios usando 'find' y 'stat'
# 'find' busca archivos dentro del directorio especificado
# 'stat' obtiene la última fecha de modificación para detectar cambios

# Mantenemos un registro de los archivos previamente procesados
declare -A archivos_previos

# Función para detectar archivos modificados
detectar_cambios() {
    # Recorremos cada uno de los directorios especificados
    for DIRECTORIO in "${DIRECTORIOS[@]}"; do
        # Buscar todos los archivos en el directorio y sus subdirectorios
        find "$DIRECTORIO" -type f | while read archivo; do
            # Obtener la última fecha de modificación del archivo
            fecha_modificacion=$(stat --format=%Y "$archivo")
            
            # Verificar si el archivo ya ha sido procesado antes
            if [[ -z "${archivos_previos[$archivo]}" || "${archivos_previos[$archivo]}" != "$fecha_modificacion" ]]; then
                # Si el archivo no ha sido procesado o su fecha ha cambiado, mostrarlo
                echo "Archivo modificado: $archivo"
                # Actualizamos la fecha de modificación para el archivo
                archivos_previos["$archivo"]=$fecha_modificacion
            fi
        done
    done
}

# Llamamos a la función para detectar los cambios
detectar_cambios
