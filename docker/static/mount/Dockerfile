# modalità di build:
    # collocarsi in project_v2/docker/static
    # docker build -t sample_static_mount -f mount/Dockerfile .

FROM ubuntu:latest  
WORKDIR /app  
COPY sample_static . 
ENTRYPOINT ["./sample_static"]
