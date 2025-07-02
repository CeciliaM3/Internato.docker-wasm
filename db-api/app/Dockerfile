FROM python:3.13-alpine

ENV PYTHONUNBUFFERED=1
ENV PYTHONDONTWRITEBYTECODE=1

WORKDIR /app

COPY requirements.txt .

# uv è molto più veloce di pip nell'installare le dipendenze (10-100x).
RUN pip install --no-cache-dir uv
RUN uv pip install --no-cache-dir -r requirements.txt --system

COPY app.py .

CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--reload"]
