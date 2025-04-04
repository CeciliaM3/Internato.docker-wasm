FROM ubuntu:latest  
RUN mkdir -p /app/data
WORKDIR /app  
COPY data/aircompressor_database.db ./data
COPY data/statistics_output.txt ./data
COPY docker/static/sample_static .
ENTRYPOINT ["./sample_static"]
