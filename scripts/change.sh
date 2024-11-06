#!/bin/bash

# Directorios raíz
DIRECTORIOS=("src" "include" "files")

# Archivo de backup donde se guardarán los hashes (checksum) de los archivos
BACKUP_FILE="backup_hashes.txt"

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

        # Creamos un archivo temporal para actualizar los hashes
        TEMP_FILE=$(mktemp)

        # Leemos el archivo de backup y comparamos los hashes
        while read -r backup_line; do
            # Extraemos el hash guardado y el archivo desde el backup
            hash_guardado=$(echo "$backup_line" | cut -d " " -f 1)
            archivo_backup=$(echo "$backup_line" | cut -d " " -f 2-)

            # Verificamos si el archivo existe en el sistema
            if [[ -f "$archivo_backup" ]]; then
                # Generamos el checksum (hash SHA256) del archivo actual
                hash_actual=$(sha256sum "$archivo_backup" | cut -d " " -f 1)

                # Si el hash ha cambiado, mostramos el archivo modificado
                if [[ "$hash_actual" != "$hash_guardado" ]]; then
                    echo "Archivo modificado: $archivo_backup"
                    # Guardamos el nuevo hash del archivo modificado
                    echo "$hash_actual  $archivo_backup" >> "$TEMP_FILE"
                else
                    # Si no ha cambiado, mantenemos la línea original del archivo de backup
                    echo "$backup_line" >> "$TEMP_FILE"
                fi
            else
                # Si el archivo no existe, lo eliminamos del archivo de backup
                echo "Archivo eliminado: $archivo_backup"
            fi
        done < "$BACKUP_FILE"

        # Recorremos cada uno de los directorios especificados para encontrar archivos nuevos
        for DIRECTORIO in "${DIRECTORIOS[@]}"; do
            # Buscar todos los archivos en el directorio y sus subdirectorios
            find "$DIRECTORIO" -type f | while read archivo; do
                # Verificamos si el archivo ya está en el archivo de backup
                if ! grep -q "$archivo" "$BACKUP_FILE"; then
                    # Generamos el checksum (hash SHA256) del archivo nuevo
                    hash_actual=$(sha256sum "$archivo" | cut -d " " -f 1)
                    
                    # Imprimimos que es un archivo nuevo
                    echo "Archivo nuevo: $archivo"
                    
                    # Agregar el nuevo archivo y su hash al archivo temporal
                    echo "$hash_actual  $archivo" >> "$TEMP_FILE"
                fi
            done
        done

        # Reemplazamos el archivo de backup original con el temporal
        mv "$TEMP_FILE" "$BACKUP_FILE"
    fi
}

# Llamamos a la función para detectar los cambios
detectar_cambios
