#!/bin/bash


#instal AES 
sudo apt-get install -y openssl libssl-dev

#detecta cambios en archivos
sudo apt-get install -y inotify-tools


#Decode AES install 
pip3 install pycryptodome
