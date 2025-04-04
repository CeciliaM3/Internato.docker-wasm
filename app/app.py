#!/bin/python3

import subprocess

from flask import Flask, jsonify, request

app = Flask(__name__)

# data_path = "/home/ceciliawsl/tirocinio/project_v2/data"
data_path = "/home/gb/Scrivania/tesisti/cecilia-mauri/Internato.docker-wasm/data"


@app.get(
    "/run_query/docker/<static_or_dynamic>/<copy_or_mount>/",
    strict_slashes=False,
)
def run_query_docker(static_or_dynamic, copy_or_mount):
    try:
        # Controllo della validità dei parametri
        if static_or_dynamic not in ["static", "dynamic"]:
            return (
                jsonify({"error": "Versione non riconosciuta (static_or_dynamic)"}),
                400,
            )

        if copy_or_mount not in ["copy", "mount"]:
            return jsonify({"error": "Versione non riconosciuta (copy_or_mount)"}), 400

        # Identificazione del container richiesto
        container_name = "sample_" + static_or_dynamic + "_" + copy_or_mount

        # Determinazione del numero di righe (estrazione dalla query string della richiesta http get)
        num_rows = request.args.get("rows", default=10, type=int)
        if num_rows <= 0:
            return (
                jsonify({"error": "Il numero di righe deve essere un intero positivo"}),
                400,
            )

        # Esecuzione del container richiesto nella modalità indicata (qualora ve ne siano diverse) e cattura dell'output
        command = ""
        if copy_or_mount == "mount":
            command = [
                "docker",
                "run",
                "--rm",
                "--mount",
                f"type=bind,source={data_path},target=/app/data",
                container_name,
                str(num_rows),
            ]
        else:
            command = ["docker", "run", "--rm", container_name, str(num_rows)]

        result = subprocess.run(command, capture_output=True, text=True)
        # Aggiunte print di debug
        print(result.stdout)
        print(result.stderr)

        # Verifica della correttezza dell'esecuzione del comando
        if result.returncode != 0:
            return jsonify({"error": "Errore nell'esecuzione del container"}), 500

        # Restituzione dei risultati al browser/chiamante
        return jsonify({"output": result.stdout})

    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.get("/run_query/wasm/", strict_slashes=False)
def run_query_wasm():
    try:
        # TODO
        # parte mancante

        # Determinazione del numero di righe (estrazione dalla query string della richiesta http get)
        num_rows = request.args.get("rows", default=10, type=int)

        # Al momento recupera solo le ultime X letture e le restituisce così come sono.
        command = ["node", "../prova-fetch/fetch2.js", num_rows]
        result = subprocess.run(command, capture_output=True, text=True)

        # Verifica della correttezza dell'esecuzione del comando
        if result.returncode != 0:
            return jsonify({"error": "Errore nell'esecuzione del modulo WASM"}), 500

        # Restituzione dei risultati al browser/chiamante
        return jsonify({"output": result.stdout})

    except Exception as e:
        return jsonify({"error": str(e)}), 500


if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=5000)
