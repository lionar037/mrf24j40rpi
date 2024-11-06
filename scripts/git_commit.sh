#!/bin/bash

VERSION=version1.1

# Obtener la fecha y hora actual en el formato YYYYMMDDHHMM
timestamp=$(date +'%Y%m%d%H%M')

# Construir el mensaje de commit
commit_message="update $timestamp"
echo "NAME GIT :"
echo ${commit_message}
# Ejecutar los comandos git
#git add . && git commit -m "$commit_message" && git push -u origin main
git add . && git commit -m "$commit_message" && git push -u origin $VERSION

#git branch --set-upstream-to=origin/main $VERSION
