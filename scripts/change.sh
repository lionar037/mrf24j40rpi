#!/bin/bash

# Directorios raíz
DIRECTORIOS=("src" "include" "files")

# Archivo de backup donde se guardarán los hashes (checksum) de los archivos
BACKUP_FILE="backup_hashes.txt"

# Comando para detectar cambios usando 'find' y 'sha256sum'
# 'find' busca archivos dentro del directorio especificado
# 'sha256sum' genera un hash único para cada archivo

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

        # Recorremos cada uno de los directorios especificados
        for DIRECTORIO in "${DIRECTORIOS[@]}"; do
            # Buscar todos los archivos en el directorio y sus subdirectorios
            find "$DIRECTORIO" -type f | while read archivo; do
                # Generamos el checksum (hash SHA256) del archivo
                hash_actual=$(sha256sum "$archivo" | cut -d " " -f 1)
                
                # Verificamos si el archivo ya existe en el backup
                hash_guardado=$(grep -E "^$archivo" "$BACKUP_FILE" | cut -d " " -f 1)
                
                # Si el hash ha cambiado o el archivo no está en el backup
                if [[ "$hash_actual" != "$hash_guardado" ]]; then
                    # Mostramos el archivo modificado
                    echo "Archivo modificado: $archivo"
                    
                    # Actualizamos o agregamos el hash del archivo al backup
                    grep -v -E "^$archivo" "$BACKUP_FILE" > "$BACKUP_FILE.tmp" && mv "$BACKUP_FILE.tmp" "$BACKUP_FILE"
                    echo "$hash_actual  $archivo" >> "$BACKUP_FILE"
                fi
            done
        done
    fi
}

# Llamamos a la función para detectar los cambios
detectar_cambios
