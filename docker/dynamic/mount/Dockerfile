FROM ubuntu:latest 
RUN apt-get update && apt-get install -y sqlite3 libsqlite3-dev 
WORKDIR /app  
COPY docker/dynamic/sample_dynamic . 
ENTRYPOINT ["./sample_dynamic"]
