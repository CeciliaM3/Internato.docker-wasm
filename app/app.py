#!/bin/python3

import os
import re
import subprocess
import time

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


# template generico dell'url:
# /run_query/<filesystem_or_httpconn>/<containerized_or_noncontainerized>/<program_type>/<nodefs_or_noderawfs>/<docker_or_podman>/<runtime_type>/<container_type>
# combinazioni attualmente disponibili:

# /run_query/filesystem/noncontainerized/execsqlite/-/-/-
# /run_query/filesystem/noncontainerized/wasmwasi/-/wasmtime/-
# /run_query/filesystem/noncontainerized/wasmwasi/-/wasmedge/-
# /run_query/filesystem/noncontainerized/wasmfs/nodefs/node/-
# /run_query/filesystem/noncontainerized/wasmfs/nodefs/bun/-
# /run_query/filesystem/noncontainerized/wasmfs/noderawfs/node/-
# /run_query/filesystem/noncontainerized/wasmfs/noderawfs/bun/-
# /run_query/filesystem/containerized/execsqlite/-/-/docker
# /run_query/filesystem/containerized/execsqlite/-/-/podman
# /run_query/filesystem/containerized/wasmwasi/-/wasmtime/docker
# /run_query/filesystem/containerized/wasmwasi/-/wasmtime/podman
# /run_query/filesystem/containerized/wasmwasi/-/wasmtime/wasmworkload   ->   al momento non funzionante
# /run_query/filesystem/containerized/wasmwasi/-/wasmedge/docker
# /run_query/filesystem/containerized/wasmwasi/-/wasmedge/podman
# /run_query/filesystem/containerized/wasmwasi/-/wasmedge/wasmworkload   ->   al momento non funzionante
# /run_query/filesystem/containerized/wasmfs/nodefs/node/docker
# /run_query/filesystem/containerized/wasmfs/nodefs/node/podman
# /run_query/filesystem/containerized/wasmfs/nodefs/bun/docker
# /run_query/filesystem/containerized/wasmfs/nodefs/bun/podman
# /run_query/filesystem/containerized/wasmfs/noderawfs/node/docker
# /run_query/filesystem/containerized/wasmfs/noderawfs/node/podman
# /run_query/filesystem/containerized/wasmfs/noderawfs/bun/docker
# /run_query/filesystem/containerized/wasmfs/noderawfs/bun/podman
# /run_query/httpconn/noncontainerized/execlibcurl/-/-/-
# /run_query/httpconn/noncontainerized/wasmfetch/-/node/-
# /run_query/httpconn/noncontainerized/wasmfetch/-/bun/-
# /run_query/httpconn/containerized/execlibcurl/-/-/docker
# /run_query/httpconn/containerized/execlibcurl/-/-/podman
# /run_query/httpconn/containerized/wasmfetch/-/node/docker
# /run_query/httpconn/containerized/wasmfetch/-/node/podman
# /run_query/httpconn/containerized/wasmfetch/-/bun/docker
# /run_query/httpconn/containerized/wasmfetch/-/bun/podman

@app.get(
    "/run_query/<path:urlparams>",
    strict_slashes=False,
)
def run_query(urlparams):
    try:
        #############################################################################################################
        # Determinazione del tipo di client per sapere se mostrare i risultati tramite
        # rendering di pagina html o come plain text (scelto per dare maggiore leggibilità per curl)
        user_agent = request.headers.get("User-Agent", "").lower()
        response_mode = ""
        if not user_agent or "curl" in user_agent:
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

        #############################################################################################################
         # Estrazione dall'url dei parametri necessari a determinare il comando da eseguire, controllo della loro validità e early exits
        url_components = urlparams.split("/")
        filesystem_or_httpconn = url_components[0]
        containerized_or_noncontainerized = url_components[1]
        program_type = url_components[2] 
        nodefs_or_noderawfs = url_components[3] 
        runtime_type = url_components[4] 
        container_type = url_components[5] 
        
        # filesystem_or_httpconn
        if filesystem_or_httpconn not in ["filesystem", "httpconn"]:
            return return_error_rightformat(
                response_mode, "Invalid url for this service: 2nd parameter not recognized. Valid values: filesystem, httpconn.", 400
            )
        
        # containerized_or_noncontainerized
        if containerized_or_noncontainerized not in ["containerized", "noncontainerized"]:
            return return_error_rightformat(
                response_mode, "Invalid url for this service: 3rd parameter not recognized. Valid values for it: containerized, noncontainerized.", 400
            )
            
        # program_type
        if (filesystem_or_httpconn == "filesystem" and program_type not in ["execsqlite", "wasmwasi", "wasmfs"]):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 4th parameter not recognized. Valid values for it: execsqlite, wasmwasi, wasmfs (for 2nd parameter = filesystem)", 400
                )
        if (filesystem_or_httpconn == "htppconn" and program_type not in ["execlibcurl", "wasmfetch"]):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 4th parameter not recognized. Valid values for it: execlibcurl, wasmfetch (for 2nd parameter = httpconn)", 400
                )
            
        # nodefs_or_noderawfs
        if (program_type == "wasmfs" and nodefs_or_noderawfs not in ["nodefs", "noderawfs"]):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 5th parameter not recognized. Valid values for it: nodefs, noderawfs (for 4th parameter = wasmfs)", 400
                )
        if (program_type != "wasmfs" and nodefs_or_noderawfs != "-"):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 5th parameter not recognized. Must be blank: - (for 4th parameter = wasmfs)", 400
                )
            
        # runtime_type
        if (program_type == "wasmwasi" and runtime_type not in ["wasmtime", "wasmedge"]):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 6th parameter not recognized. Valid values for it: wasmtime, wasmedge (for 4th parameter = wasmwasi)", 400
                )
        if (program_type in ["wasmfs", "wasmfetch"] and runtime_type not in ["node", "bun"]):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 6th parameter not recognized. Valid values for it: node, bun (for 4th parameter = wasmfs or wasmfetch)", 400
                )
        if (program_type in ["execsqlite", "execlibcurl"] and runtime_type != "-"):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 6th parameter not recognized. Must be blank: - (for 4th parameter = execsqlite or execlibcurl)", 400
                )
            
        #container_type
        if (containerized_or_noncontainerized == "containerized" and container_type not in ["docker", "podman", "wasmworkload"]):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 7th parameter not recognized. Valid values for it: docker, podman, wasmworkload (for 3rd parameter = containerized)", 400
                )
        if (containerized_or_noncontainerized == "containerized" and container_type == "wasmworkload" and program_type != "wasmwasi"):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 7th parameter not recognized. wasmworkload is only valid for 3rd parameter = containerized and 4th parameter = wasmfs)", 400
                )
        if (containerized_or_noncontainerized == "noncontainerized" and container_type != "-"):
            return return_error_rightformat(
                    response_mode, "Invalid url for this service: 7th parameter not recognized. Must be blank: - (for 3rd parameter = noncontainerized)", 400
                )
            
        #############################################################################################################
        # Esecuzione effettiva del comando richiesto
        workdir = "."
        command = ""
        execution_mode = ""
        bunpath = os.path.join(os.path.expanduser("~"), ".bun", "bin", "bun")
        
        if filesystem_or_httpconn == "filesystem":
            if containerized_or_noncontainerized == "noncontainerized":
                if program_type == "execsqlite":
                    workdir = "../src_rawsqlite"
                    command = ["./statistics_calc_sqlite", str(num_rows)]
                    execution_mode = "Program using sqlite3 for filesystem access compiled to native executable with g++ and run directly on wsl"
                elif program_type == "wasmwasi":
                    workdir = "../src_wasi"
                    if runtime_type == "wasmtime":
                        command = ["wasmtime", "--dir", "db", "statistics_calc_sqlite.wasm", str(num_rows)]
                        execution_mode = "Program using sqlite3 and WASI for filesystem access compiled to wasm with clang++ (no JS glue code present) and run directly on wsl in Wasmtime runtime"
                    elif runtime_type == "wasmedge":
                        command = ["wasmedge", "--dir", "db:db", "statistics_calc_sqlite.wasm", str(num_rows)]
                        execution_mode = "Program using sqlite3 and WASI for filesystem access compiled to wasm with clang++ (no JS glue code present) and run directly on wsl in Wasmedge runtime"
                elif program_type == "wasmfs":
                    workdir = "../src_fs"
                    if nodefs_or_noderawfs == "nodefs":
                        if runtime_type == "node":
                            command = ["node", "statistics_calc_sqlite_nodefs.js", str(num_rows)]
                            execution_mode = "Program using sqlite3 and NODEFS for filesystem access compiled to wasm with emcc (JS glue code present) and run directly on wsl in Node runtime"
                        elif runtime_type == "bun":
                            command = [bunpath, "statistics_calc_sqlite_nodefs.js", str(num_rows)]
                            execution_mode = "Program using sqlite3 and NODEFS for filesystem access compiled to wasm with emcc (JS glue code present) and run directly on wsl in Bun runtime"
                    elif nodefs_or_noderawfs == "noderawfs":
                        if runtime_type == "node":
                            command = ["node", "statistics_calc_sqlite_noderawfs.js", str(num_rows)]
                            execution_mode = "Program using sqlite3 and NODERAWFS for filesystem access compiled to wasm with emcc (JS glue code present) and run directly on wsl in Node runtime"
                        elif runtime_type == "bun":
                            command = [bunpath, "statistics_calc_sqlite_noderawfs.js", str(num_rows)]
                            execution_mode = "Program using sqlite3 and NODERAWFS for filesystem access compiled to wasm with emcc (JS glue code present) and run directly on wsl in Bun runtime"
            elif containerized_or_noncontainerized == "containerized":
                if program_type == "execsqlite":
                    workdir = "../src_rawsqlite"
                    command = [container_type, "run", "--rm", "-v", "./db:/app/db", "statistics_calc_sqlite_executable", str(num_rows)]
                    execution_mode = f"Program using sqlite3 for filesystem access compiled to native executable with g++ and run in a {container_type.capitalize()} container"
                elif program_type == "wasmwasi":
                    workdir = "../src_wasi"
                    if runtime_type == "wasmtime":
                        if container_type == "wasmworkload": # endpoint da considerare attualmente non funzionante, comando probabilmente da rivedere
                            command = ["docker", "run", "--rm", "-v", "./db:/app/db", "--platform=wasi/wasm", "--runtime=io.containerd.wasmtime.v1", "statistics_calc_sqlite_wasmworkload"]
                            execution_mode = "Program using sqlite3 and WASI for filesystem access compiled to wasm with clang++ (no JS glue code present) and run in Wasmtime directly over Containerd (no Runc)"
                        else:
                            command = [container_type, "run", "--rm", "-v", "./db:/app/db", "statistics_calc_sqlite_wasmtime", str(num_rows)]
                            execution_mode = f"Program using sqlite3 and WASI for filesystem access compiled to wasm with clang++ (no JS glue code present) and run in Wasmtime runtime in a {container_type.capitalize()} container"
                    elif runtime_type == "wasmedge":
                        if container_type == "wasmworkload": # endpoint da considerare attualmente non funzionante, comando probabilmente da rivedere
                            command = ["docker", "run", "--rm", "-v", "./db:/app/db", "--platform=wasi/wasm", "--runtime=io.containerd.wasmedge.v1", "statistics_calc_sqlite_wasmworkload"]
                            execution_mode = "Program using sqlite3 and WASI for filesystem access compiled to wasm with clang++ (no JS glue code present) and run in Wasmedge directly over Containerd (no Runc)"
                        else:
                            command = [container_type, "run", "--rm", "-v", "./db:/app/db", "statistics_calc_sqlite_wasmedge", str(num_rows)]
                            execution_mode = f"Program using sqlite3 and WASI for filesystem access compiled to wasm with clang++ (no JS glue code present) and run in Wasmedge runtime in a {container_type.capitalize()} container"
                elif program_type == "wasmfs":
                    workdir = "../src_fs"
                    if nodefs_or_noderawfs == "nodefs":
                        if runtime_type == "node":
                            command = [container_type, "run", "--rm", "-v", "./db:/app/db", "statistics_calc_sqlite_node_nodefs", str(num_rows)]
                            execution_mode = f"Program using sqlite3 and NODEFS for filesystem access compiled to wasm with em++ (JS glue code present) and run in Node runtime in a {container_type.capitalize()} container"
                        elif runtime_type == "bun":
                            command = [container_type, "run", "--rm", "-v", "./db:/app/db", "statistics_calc_sqlite_bun_nodefs", str(num_rows)]
                            execution_mode = f"Program using sqlite3 and NODEFS for filesystem access compiled to wasm with em++ (JS glue code present) and run in Bun runtime in a {container_type.capitalize()} container"
                    elif nodefs_or_noderawfs == "noderawfs":
                        if runtime_type == "node":
                            command = [container_type, "run", "--rm", "-v", "./db:/app/db", "statistics_calc_sqlite_node_noderawfs", str(num_rows)]
                            execution_mode = f"Program using sqlite3 and NODERAWFS for filesystem access compiled to wasm with em++ (JS glue code present) and run in Node runtime in a {container_type.capitalize()} container"
                        elif runtime_type == "bun":
                            command = [container_type, "run", "--rm", "-v", "./db:/app/db", "statistics_calc_sqlite_bun_noderawfs", str(num_rows)]
                            execution_mode = f"Program using sqlite3 and NODERAWFS for filesystem access compiled to wasm with em++ (JS glue code present) and run in Bun runtime in a {container_type.capitalize()} container"
        elif filesystem_or_httpconn == "httpconn":
            if containerized_or_noncontainerized == "noncontainerized":
                if program_type == "execlibcurl":
                    command = ["../src_libcurl/statistics_calc_libcurl", str(num_rows)]
                    execution_mode = "Program using libcurl for http requests compiled to native executable with g++ and run directly on wsl"
                elif program_type == "wasmfetch":
                    if runtime_type == "node":
                        command = ["node", "../src_fetch/statistics_calc_fetch.js", str(num_rows)]
                        execution_mode = "Program using Emscripten fetch API for http requests compiled to wasm with emcc (JS glue code present) and run directly on wsl in Node runtime"
                    elif runtime_type == "bun":
                        command = [bunpath, "../src_fetch/statistics_calc_fetch.js", str(num_rows)]
                        execution_mode = "Program using Emscripten fetch API for http requests compiled to wasm with emcc (JS glue code present) and run directly on wsl in Bun runtime"
            elif containerized_or_noncontainerized == "containerized":
                if program_type == "execlibcurl":
                    command = [container_type, "run", "--rm", "statistics_calc_executable", str(num_rows)]
                    execution_mode = f"Program using libcurl for http requests compiled to native executable with g++ and run in a {container_type.capitalize()} container"
                elif program_type == "wasmfetch":
                    if runtime_type == "node":
                        command = [container_type, "run", "--rm", "statistics_calc_wasm_node", str(num_rows)]
                        execution_mode = f"Program using Emscripten fetch API for http requests compiled to wasm with emcc (JS glue code present) and run in Node runtime in a {container_type.capitalize()} container"
                    elif runtime_type == "bun":
                        command = [container_type, "run", "--rm", "statistics_calc_wasm_bun", str(num_rows)]
                        execution_mode = f"Program using Emscripten fetch API for http requests compiled to wasm with emcc (JS glue code present) and run in Bun runtime in a {container_type.capitalize()} container"
            
        #############################################################################################################
        # Esecuzione del comando richiesto e cattura dell'output
        try:
            starttime = time.perf_counter()
            result = subprocess.run(command, capture_output=True, text=True, cwd=workdir, timeout=300)
            totaltime_ms = (time.perf_counter() - starttime)*1000
        except subprocess.TimeoutExpired:
            return return_error_rightformat(
                response_mode,
                "Ran command gave no response. Timeout for its execution has been exceeded.",
                500,
            )

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

            # Estrazione dei dati in strutture più piccole e mirate per poter presentare i risultati
            # anche all'interno di una tabella in maniera ordinata
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
                    subprocess_time=f"{totaltime_ms:.3f}"
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
        print("/run_query/filesystem/noncontainerized/execsqlite/-/-/-")
        print("/run_query/filesystem/noncontainerized/wasmwasi/-/wasmtime/-")
        print("/run_query/filesystem/noncontainerized/wasmwasi/-/wasmedge/-")
        print("/run_query/filesystem/noncontainerized/wasmfs/nodefs/node/-")
        print("/run_query/filesystem/noncontainerized/wasmfs/nodefs/bun/-")
        print("/run_query/filesystem/noncontainerized/wasmfs/noderawfs/node/-")
        print("/run_query/filesystem/noncontainerized/wasmfs/noderawfs/bun/-")
        print("/run_query/filesystem/containerized/execsqlite/-/-/docker")
        print("/run_query/filesystem/containerized/execsqlite/-/-/podman")
        print("/run_query/filesystem/containerized/wasmwasi/-/wasmtime/docker")
        print("/run_query/filesystem/containerized/wasmwasi/-/wasmtime/podman")
        print("/run_query/filesystem/containerized/wasmwasi/-/wasmtime/wasmworkload")
        print("/run_query/filesystem/containerized/wasmwasi/-/wasmedge/docker")
        print("/run_query/filesystem/containerized/wasmwasi/-/wasmedge/podman")
        print("/run_query/filesystem/containerized/wasmwasi/-/wasmedge/wasmworkload")
        print("/run_query/filesystem/containerized/wasmfs/nodefs/node/docker")
        print("/run_query/filesystem/containerized/wasmfs/nodefs/node/podman")
        print("/run_query/filesystem/containerized/wasmfs/nodefs/bun/docker")
        print("/run_query/filesystem/containerized/wasmfs/nodefs/bun/podman")
        print("/run_query/filesystem/containerized/wasmfs/noderawfs/node/docker")
        print("/run_query/filesystem/containerized/wasmfs/noderawfs/node/podman")
        print("/run_query/filesystem/containerized/wasmfs/noderawfs/bun/docker")
        print("/run_query/filesystem/containerized/wasmfs/noderawfs/bun/podman")
        print("/run_query/httpconn/noncontainerized/execlibcurl/-/-/-")
        print("/run_query/httpconn/noncontainerized/wasmfetch/-/node/-")
        print("/run_query/httpconn/noncontainerized/wasmfetch/-/bun/-")
        print("/run_query/httpconn/containerized/execlibcurl/-/-/docker")
        print("/run_query/httpconn/containerized/execlibcurl/-/-/podman")
        print("/run_query/httpconn/containerized/wasmfetch/-/node/docker")
        print("/run_query/httpconn/containerized/wasmfetch/-/node/podman")
        print("/run_query/httpconn/containerized/wasmfetch/-/bun/docker")
        print("/run_query/httpconn/containerized/wasmfetch/-/bun/podman")
    app.run(debug=True, host="0.0.0.0", port=5000)
