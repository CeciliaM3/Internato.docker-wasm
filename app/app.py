#!/bin/python3

import os
import re
import subprocess

from flask import Flask, Response, render_template, request

app = Flask(__name__)


def return_error_rightformat(response_mode, message, http_code):
    if response_mode == "html":
        return render_template("results.html", error=message), http_code
    else:
        return Response(message + "\n", status=http_code, mimetype="text/plain")


# endpoints


@app.get("/")
def index():
    # Endpoint accessibile soltanto da browser
    user_agent = request.headers.get("User-Agent", "").lower()
    if "curl" in user_agent or user_agent == "":
        return Response(
            "Endpoint for user-friendly input mode is only accessible to browsers.\n",
            status=403,
            mimetype="text/plain",
        )
    else:
        return render_template("choose_mode.html"), 200


@app.get(
    "/run_query/",
    strict_slashes=False,
)
def run_query_index():
    # Endpoint accessibile soltanto da browser
    user_agent = request.headers.get("User-Agent", "").lower()
    if "curl" in user_agent or user_agent == "":
        return Response(
            "Endpoint for user-friendly input mode is only accessible to browsers.\n",
            status=403,
            mimetype="text/plain",
        )
    else:
        return render_template("choose_mode.html"), 200


# template generico dell'url: /run_query/<containerized_or_noncontainerized>/<docker_or_podman>/<executable_or_wasm>/<node_or_bun>
# combinazioni attualmente funzionanti:

# /run_query/containerized/docker/wasm/node
# /run_query/containerized/docker/wasm/bun
# /run_query/containerized/docker/executable


# /run_query/noncontainerized/wasm/node
# /run_query/noncontainerized/wasm/bun
# /run_query/noncontainerized/executable
@app.get(
    "/run_query/<path:urlparams>",
    strict_slashes=False,
)
def run_query(urlparams):
    try:
        # Determinazione del tipo di client per sapere se mostrare i risultati tramite
        # rendering di pagina html o come plain text (scelto per dare maggiore leggibilità per curl)
        user_agent = request.headers.get("User-Agent", "").lower()
        response_mode = ""
        if "curl" in user_agent or user_agent == "":
            response_mode = "text"
        else:
            response_mode = "html"

        # Determinazione del numero di righe da considerare (estrazione dalla query string della richiesta http get)
        num_rows = request.args.get("rows", default=10, type=int)
        if num_rows < 1:
            return return_error_rightformat(
                response_mode, "The number of rows must be a positive integer.", 400
            )
        if num_rows > 1516949:
            num_rows = 1516949

        # Estrazione dall'url dei parametri necessari a determinare il comando da eseguire
        command = ""
        execution_mode = ""
        url_components = urlparams.split("/")
        containerized_or_noncontainerized = url_components[0]
        if containerized_or_noncontainerized == "containerized":
            docker_or_podman = url_components[1]
            if docker_or_podman == "docker":
                executable_or_wasm = url_components[2]
                if executable_or_wasm == "wasm":
                    node_or_bun = url_components[3]
                    if node_or_bun == "node":
                        command = [
                            "docker",
                            "run",
                            "--rm",
                            "statistics_calc_wasm_node",
                            str(num_rows),
                        ]
                        execution_mode = "program compiled to wasm with emcc and run in node runtime in a Docker container"
                    elif node_or_bun == "bun":
                        command = [
                            "docker",
                            "run",
                            "--rm",
                            "statistics_calc_wasm_bun",
                            str(num_rows),
                        ]
                        execution_mode = "program compiled to wasm with emcc and run in bun runtime in a Docker container"
                    else:
                        return return_error_rightformat(
                            response_mode, "Invalid url for this service.", 400
                        )
                elif executable_or_wasm == "executable":
                    command = [
                        "docker",
                        "run",
                        "--rm",
                        "statistics_calc_executable",
                        str(num_rows),
                    ]
                    execution_mode = "program compiled to native executable with g++ and run in a Docker container"
                else:
                    return return_error_rightformat(
                        response_mode, "Invalid url for this service.", 400
                    )
            elif docker_or_podman == "podman":
                return return_error_rightformat(
                    response_mode, "Modes not yet available.", 503
                )
            else:
                return return_error_rightformat(
                    response_mode, "Invalid url for this service.", 400
                )
        elif containerized_or_noncontainerized == "noncontainerized":
            executable_or_wasm = url_components[1]
            if executable_or_wasm == "wasm":
                node_or_bun = url_components[2]
                if node_or_bun == "node":
                    command = [
                        "node",
                        "../src_fetch/statistics_calc_fetch.js",
                        str(num_rows),
                    ]
                    execution_mode = "program compiled to wasm with emcc and run directly on wsl in node runtime"
                elif node_or_bun == "bun":
                    bunpath = os.path.join(
                        os.path.expanduser("~"), ".bun", "bin", "bun"
                    )
                    command = [
                        bunpath,
                        "../src_fetch/statistics_calc_fetch.js",
                        str(num_rows),
                    ]
                    execution_mode = "program compiled to wasm with emcc and run directly on wsl in bun runtime"
                else:
                    return return_error_rightformat(
                        response_mode, "Invalid url for this service.", 400
                    )
            elif executable_or_wasm == "executable":
                command = ["../src_libcurl/statistics_calc_libcurl", str(num_rows)]
                execution_mode = "program compiled to native executable with g++ and run directly on wsl"
            else:
                return return_error_rightformat(
                    response_mode, "Invalid url for this service.", 400
                )
        else:
            return return_error_rightformat(
                response_mode, "Invalid url for this service.", 400
            )

        # Esecuzione del comando richiesto e cattura dell'output
        result = subprocess.run(command, capture_output=True, text=True)

        # Aggiunte print di debug
        # print(result.stdout)
        print(result.stderr)

        # Verifica della correttezza dell'esecuzione del comando
        if result.returncode != 0:
            return return_error_rightformat(
                response_mode,
                "Error during the execution of the requested command.",
                500,
            )

        # Restituzione dei risultati al browser/chiamante
        if response_mode == "html":
            raw_result = result.stdout

            # Estrazione dei dati in strutture più piccole e mirate per poter presentare i risultati anche all'interno di una tabella in maniera ordinata
            dividing_basedonsectiontitle_pattern = (
                r"### (.*?) ###\s*([\s\S]*?)(?=###|$)"
            )
            data_blocks_strings = dict(
                re.findall(dividing_basedonsectiontitle_pattern, raw_result)
            )
            data_blocks_lists = {
                title: values.split() for title, values in data_blocks_strings.items()
            }
            third_section_key = "AVERAGE, VARIANCE, STANDARD DEVIATION"
            data_blocks_lists[third_section_key] = [
                data_blocks_lists[third_section_key][i : i + 4]
                for i in range(0, len(data_blocks_lists[third_section_key]), 4)
            ]
            fourth_section_key = "Number of anomalies (values outside of the avg +/- 3*stddev interval) per column"
            data_blocks_lists[fourth_section_key] = [
                elem
                for index, elem in enumerate(data_blocks_lists[fourth_section_key])
                if index % 2 != 0
            ]

            return (
                render_template(
                    "results.html",
                    output=raw_result,
                    processed_output=data_blocks_lists,
                    mode=execution_mode,
                    num_rows=num_rows,
                ),
                200,
            )
        else:
            return Response(
                "Version used: " + execution_mode + "\n\n" + result.stdout,
                status=200,
                mimetype="text/plain",
            )

    except Exception as e:
        return return_error_rightformat(response_mode, str(e), 500)


if __name__ == "__main__":
    if os.environ.get("WERKZEUG_RUN_MAIN") == "true":
        print("\nUrls for available modes:\n")
        print("/run_query/containerized/docker/wasm/node")
        print("/run_query/containerized/docker/wasm/bun")
        print("/run_query/containerized/docker/executable")
        print("/run_query/noncontainerized/wasm/node")
        print("/run_query/noncontainerized/wasm/bun")
        print("/run_query/noncontainerized/executable\n")
    app.run(debug=True, host="0.0.0.0", port=5000)
