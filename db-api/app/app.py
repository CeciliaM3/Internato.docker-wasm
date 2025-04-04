import os
from typing import List

import mysql.connector
from fastapi import FastAPI

app = FastAPI()

# Senza di questo le richieste provenienti da WASM vengono bloccate
from fastapi.middleware.cors import CORSMiddleware

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


# Parametri di connessione al database MySQL
DB_HOST = os.getenv("DB_HOST", "172.17.0.1")
DB_PORT = int(os.getenv("DB_PORT", 3306))
DB_USER = os.getenv("DB_USER", "root")
DB_PASSWORD = os.getenv("DB_PASSWORD", "password")
DB_NAME = os.getenv("DB_NAME", "mydb")


# Funzione per connettersi al database MySQL
def get_db_connection():
    connection = mysql.connector.connect(
        host=DB_HOST,
        port=DB_PORT,
        user=DB_USER,
        password=DB_PASSWORD,
        database=DB_NAME,
    )
    return connection


# Endpoint per ottenere gli ultimi N record
@app.get("/records/{n}", response_model=List[dict])
def get_last_n_records(n: int):
    conn = get_db_connection()
    cursor = conn.cursor(dictionary=True)

    query = "SELECT * FROM data ORDER BY timestamp DESC LIMIT %s;"
    cursor.execute(query, (n,))

    records = cursor.fetchall()

    cursor.close()
    conn.close()

    return records
