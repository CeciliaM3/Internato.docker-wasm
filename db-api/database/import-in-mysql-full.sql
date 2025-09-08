SHOW VARIABLES LIKE 'max_allowed_packet';

CREATE DATABASE IF NOT EXISTS mydb;
USE mydb;

CREATE TABLE aircompressor_data (
    `timestamp` VARCHAR(255),
    TP2 DOUBLE,
    TP3 DOUBLE,
    H1 DOUBLE,
    DV_pressure DOUBLE,
    Reservoirs DOUBLE,
    Oil_temperature DOUBLE,
    Motor_current DOUBLE,
    COMP DOUBLE,
    DV_electric DOUBLE,
    Towers DOUBLE,
    MPG DOUBLE,
    LPS DOUBLE,
    Pressure_switch DOUBLE,
    Oil_level DOUBLE,
    Caudal_impulses DOUBLE
);

LOAD DATA INFILE '/var/lib/mysql-files/aircompressor_cleaned.csv'
INTO TABLE aircompressor_data
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n';
