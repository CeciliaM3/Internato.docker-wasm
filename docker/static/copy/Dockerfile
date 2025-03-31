# modalità di build:
    # collocarsi in project_v2
    # docker build -t sample_static_copy -f docker/static/copy/Dockerfile .

FROM ubuntu:latest  
RUN mkdir -p /app/data
WORKDIR /app  
COPY data/aircompressor_database.db ./data
COPY data/statistics_output.txt ./data
COPY docker/static/sample_static .
ENTRYPOINT ["./sample_static"]
