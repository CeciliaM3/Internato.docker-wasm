import os
from typing import List
from pydantic import BaseModel
from pydantic import constr
import mysql.connector
from fastapi import FastAPI
from fastapi import HTTPException
from fastapi import Body
from fastapi.responses import JSONResponse

class SQLQuery(BaseModel):
    query: constr(strip_whitespace=True, min_length=1)

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

# Endpoint per eseguire la specifica query indicata nel body della ridchiesta http
@app.post("/query", response_model=List[dict])
def perform_query(payload: SQLQuery = Body(...)):
    upper_query = payload.query.strip().upper()
    if not (upper_query.startswith("SELECT") or upper_query.startswith("WITH")):
        raise HTTPException(status_code=400, detail="Only SELECT statements are allowed.")

    conn = get_db_connection()
    cursor = conn.cursor(dictionary=True)

    try:
        cursor.execute(payload.query)
        result = cursor.fetchall() if cursor.description else []
                
        return JSONResponse(content=result)
    
    except mysql.connector.Error as e:
        raise HTTPException(status_code=400, detail=str(e))
    
    finally:
        cursor.close()
        conn.close()
