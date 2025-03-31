# modalità di build:
    # collocarsi in project_v2/docker/dynamic
    # docker build -t sample_dynamic_mount -f mount/Dockerfile .

FROM ubuntu:latest 
RUN apt-get update && apt-get install -y sqlite3 libsqlite3-dev 
WORKDIR /app  
COPY sample_dynamic . 
ENTRYPOINT ["./sample_dynamic"]