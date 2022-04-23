# syntax=docker/dockerfile
FROM ubuntu:20.04
RUN DEBIAN_FRONTEND=noninteractive apt update && apt upgrade -y
RUN DEBIAN_FRONTEND=noninteractive apt install -y build-essential
RUN DEBIAN_FRONTEND=noninteractive apt install -y cmake
RUN DEBIAN_FRONTEND=noninteractive apt install -y openjdk-8-jre openjdk-8-jdk 
WORKDIR /home/workspace/IFCTO3DTILES
COPY . /home/workspace/IFCTO3DTILES