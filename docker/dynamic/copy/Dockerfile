# modalità di build:
    # collocarsi in project_v2
    # docker build -t sample_dynamic_copy -f docker/dynamic/copy/Dockerfile .

FROM ubuntu:latest  
RUN mkdir -p /app/data
RUN apt-get update && apt-get install -y sqlite3 libsqlite3-dev
WORKDIR /app  
COPY data/aircompressor_database.db ./data
COPY data/statistics_output.txt ./data
COPY docker/dynamic/sample_dynamic .
ENTRYPOINT ["./sample_dynamic"]
    