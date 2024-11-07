#!/bin/bash

# Directorios raíz
DIRECTORIOS=("src" "include")

# Archivo de backup donde se guardarán los hashes (checksum) de los archivos
BACKUP_FILE="files/backup_hashes.txt"

# Función para detectar archivos modificados
detectar_cambios() {
    # Si el archivo de backup no existe, lo creamos
    if [[ ! -f "$BACKUP_FILE" ]]; then
        echo "Primer ejecución, creando archivo de backup..."
        touch "$BACKUP_FILE"
        
        # Recorremos cada uno de los directorios especificados
        for DIRECTORIO in "${DIRECTORIOS[@]}"; do
            # Buscar todos los archivos en el directorio y sus subdirectorios
            find "$DIRECTORIO" -type f | while read archivo; do
                # Generamos el checksum (hash SHA256) del archivo
                hash_actual=$(sha256sum "$archivo" | cut -d " " -f 1)
                
                # Guardamos el hash en el archivo de backup
                echo "$hash_actual  $archivo" >> "$BACKUP_FILE"
                # Imprimimos el archivo ya que es la primera ejecución
                echo "Archivo procesado (primer ejecución): $archivo"
            done
        done
    else
        # Si el archivo de backup ya existe, solo mostramos los archivos modificados
        echo "Detectando archivos modificados..."

        # Creamos un archivo temporal para generar los nuevos hashes
        TEMP_FILE=$(mktemp)

        # Recorremos cada uno de los directorios especificados
        for DIRECTORIO in "${DIRECTORIOS[@]}"; do
            # Buscar todos los archivos en el directorio y sus subdirectorios
            find "$DIRECTORIO" -type f | while read archivo; do
                # Generamos el checksum (hash SHA256) del archivo
                hash_actual=$(sha256sum "$archivo" | cut -d " " -f 1)

                # Agregar el archivo y su hash al archivo temporal
                echo "$hash_actual  $archivo" >> "$TEMP_FILE"
            done
        done

        # Comparamos el archivo de backup con el temporal
        echo "Comparando archivos..."

        # Comparamos las líneas de ambos archivos (backup y temporal)
        diff "$BACKUP_FILE" "$TEMP_FILE" | while read line; do
            if [[ "$line" =~ ">" ]]; then
                # Línea diferente en el archivo temporal (modificación)
                echo "Archivo modificado: $(echo $line | cut -d ' ' -f 2-)"
            fi
        done

        # Si hay diferencias, reemplazamos el archivo de backup con el temporal
        if diff "$BACKUP_FILE" "$TEMP_FILE" > /dev/null; then
            echo "No hay cambios."
        else
            mv "$TEMP_FILE" "$BACKUP_FILE"
            echo "Backup actualizado con los cambios."
        fi
    fi
}

# Llamamos a la función para detectar los cambios
detectar_cambios
