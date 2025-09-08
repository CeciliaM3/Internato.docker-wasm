import sys
import pandas 
import matplotlib.pyplot as plotter
from matplotlib.ticker import FixedLocator, NullLocator, FixedFormatter

if len(sys.argv) != 2:
    print(f"Required parameter missing. Usage: {sys.argv[0]} <cmd_or_curl>")
    sys.exit(1)
    
opt = sys.argv[1]
if opt == "cmd":
    exec_info = "full_command"
    src_csv = "commands_benchmark_results.csv"
elif opt == "curl":
    exec_info = "endpoint"
    src_csv = "curl_benchmark_results.csv"
else:
    print(f"Invalid parameter. Accepted values: cmd, curl")
    sys.exit(1)

##############################################################################################################
# Definizione di tutte le configurazioni per cui sono stati effettuati i test e delle necessarie informazioni associate

all_configurations = [
    # FS not containerized
    {
       "full_command": "./statistics_calc_sqlite",
       "endpoint": "/run_query/filesystem/noncontainerized/execsqlite/-/-/-",
       "config_name": "Executable using sqlite3, run on wsl",
       "line_color": "#0055b3",
       "line_style": "solid",
    }, 
    {
       "full_command": "wasmtime --dir db statistics_calc_sqlite.wasm",
       "endpoint": "/run_query/filesystem/noncontainerized/wasmwasi/-/wasmtime/-",
       "config_name": "Wasm bytecode using sqlite3 + WASI, run on wsl in Wasmtime runtime",
       "line_color": "#cc0000",
       "line_style": "solid",
    }, 
    {
       "full_command": "wasmedge --dir db:db statistics_calc_sqlite.wasm",
       "endpoint": "/run_query/filesystem/noncontainerized/wasmwasi/-/wasmedge/-",
       "config_name": "Wasm bytecode using sqlite3 + WASI, run on wsl in Wasmedge runtime",
       "line_color": "#d3b5d3",
       "line_style": "solid",
    }, 
    {
       "full_command": "node statistics_calc_sqlite_nodefs.js",
       "endpoint": "/run_query/filesystem/noncontainerized/wasmfs/nodefs/node/-",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODEFS, run on wsl in Node runtime",
       "line_color": "#aed580",
       "line_style": "solid",
    }, 
    {
       "full_command": "bun statistics_calc_sqlite_nodefs.js",
       "endpoint": "/run_query/filesystem/noncontainerized/wasmfs/nodefs/bun/-",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODEFS, run on wsl in Bun runtime",
       "line_color": "#f9c70c",
       "line_style": "solid",
    }, 
    {
       "full_command": "node statistics_calc_sqlite_noderawfs.js",
       "endpoint": "/run_query/filesystem/noncontainerized/wasmfs/noderawfs/node/-",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODERAWFS, run on wsl in Node runtime",
       "line_color": "#265828",
       "line_style": "solid",
    }, 
    {
       "full_command": "bun statistics_calc_sqlite_noderawfs.js",
       "endpoint": "/run_query/filesystem/noncontainerized/wasmfs/noderawfs/bun/-",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODERAWFS, run on wsl in Bun runtime",
       "line_color": "#92623a",
       "line_style": "solid",
    }, 
    # FS containerized
    {
       "full_command": "docker run --rm -v ./db:/app/db statistics_calc_sqlite_executable",
       "endpoint": "/run_query/filesystem/containerized/execsqlite/-/-/docker",
       "config_name": "Executable using sqlite3, run in Docker container",
       "line_color": "#0055b3",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm -v ./db:/app/db statistics_calc_sqlite_executable",
       "endpoint": "/run_query/filesystem/containerized/execsqlite/-/-/podman",
       "config_name": "Executable using sqlite3, run in Podman container",
       "line_color": "#0055b3",
       "line_style": "dotted",
    }, 
    {
       "full_command": "docker run --rm -v ./db:/app/db statistics_calc_sqlite_wasmtime",
       "endpoint": "/run_query/filesystem/containerized/wasmwasi/-/wasmtime/docker",
       "config_name": "Wasm bytecode using sqlite3 + WASI, in Wasmtime runtime run in Docker container",
       "line_color": "#cc0000",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm -v ./db:/app/db statistics_calc_sqlite_wasmtime",
       "endpoint": "/run_query/filesystem/containerized/wasmwasi/-/wasmtime/podman",
       "config_name": "Wasm bytecode using sqlite3 + WASI, in Wasmtime runtime run in Podman container",
       "line_color": "#cc0000",
       "line_style": "dotted",
    },
    {
       "full_command": "docker run --rm -v ./db:/app/db statistics_calc_sqlite_wasmedge",
       "endpoint": "/run_query/filesystem/containerized/wasmwasi/-/wasmedge/docker",
       "config_name": "Wasm bytecode using sqlite3 + WASI, in Wasmedge runtime run in Docker container",
       "line_color": "#d3b5d3",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm -v ./db:/app/db statistics_calc_sqlite_wasmedge",
       "endpoint": "/run_query/filesystem/containerized/wasmwasi/-/wasmedge/podman",
       "config_name": "Wasm bytecode using sqlite3 + WASI, in Wasmedge runtime run in Podman container",
       "line_color": "#d3b5d3",
       "line_style": "dotted",
    }, 
    {
       "full_command": "docker run --rm -v ./db:/app/db statistics_calc_sqlite_node_nodefs",
       "endpoint": "/run_query/filesystem/containerized/wasmfs/nodefs/node/docker",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODEFS, in Node runtime in Docker container",
       "line_color": "#aed580",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm -v ./db:/app/db statistics_calc_sqlite_node_nodefs",
       "endpoint": "/run_query/filesystem/containerized/wasmfs/nodefs/node/podman",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODEFS, in Node runtime in Podman container",
       "line_color": "#aed580",
       "line_style": "dotted",
    }, 
    {
       "full_command": "docker run --rm -v ./db:/app/db statistics_calc_sqlite_bun_nodefs",
       "endpoint": "/run_query/filesystem/containerized/wasmfs/nodefs/bun/docker",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODEFS, in Bun runtime in Docker container",
       "line_color": "#f9c70c",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm -v ./db:/app/db statistics_calc_sqlite_bun_nodefs",
       "endpoint": "/run_query/filesystem/containerized/wasmfs/nodefs/bun/podman",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODEFS, in Bun runtime in Podman container",
       "line_color": "#f9c70c",
       "line_style": "dotted",
    }, 
    {
       "full_command": "docker run --rm -v ./db:/app/db statistics_calc_sqlite_node_noderawfs",
       "endpoint": "/run_query/filesystem/containerized/wasmfs/noderawfs/node/docker",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODERAWFS, in Node runtime in Docker container",
       "line_color": "#265828",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm -v ./db:/app/db statistics_calc_sqlite_node_noderawfs",
       "endpoint": "/run_query/filesystem/containerized/wasmfs/noderawfs/node/podman",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODERAWFS, in Node runtime in Podman container",
       "line_color": "#265828",
       "line_style": "dotted",
    }, 
    {
       "full_command": "docker run --rm -v ./db:/app/db statistics_calc_sqlite_bun_noderawfs",
       "endpoint": "/run_query/filesystem/containerized/wasmfs/noderawfs/bun/docker",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODERAWFS, in Bun runtime in Docker container",
       "line_color": "#92623a",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm -v ./db:/app/db statistics_calc_sqlite_bun_noderawfs",
       "endpoint": "/run_query/filesystem/containerized/wasmfs/noderawfs/bun/podman",
       "config_name": "Wasm bytecode + JS glue code using sqlite3 + NODERAWFS, in Bun runtime in Podman container",
       "line_color": "#92623a",
       "line_style": "dotted",
    }, 
    # HTTP not containerized
    {
       "full_command": "../src_libcurl/statistics_calc_libcurl",
       "endpoint": "/run_query/httpconn/noncontainerized/execlibcurl/-/-/-",
       "config_name": "Executable using libcurl, run on wsl",
       "line_color": "#0055b3",
       "line_style": "solid",
    }, 
    {
       "full_command": "node ../src_fetch/statistics_calc_fetch.js",
       "endpoint": "/run_query/httpconn/noncontainerized/wasmfetch/-/node/-",
       "config_name": "Wasm bytecode + JS glue code using Fetch API, run on wsl in Node runtime",
       "line_color": "#3d8c40",
       "line_style": "solid",
    }, 
    {
       "full_command": "bun ../src_fetch/statistics_calc_fetch.js",
       "endpoint": "/run_query/httpconn/noncontainerized/wasmfetch/-/bun/-",
       "config_name": "Wasm bytecode + JS glue code using Fetch API, run on wsl in Bun runtime",
       "line_color": "#dbba51",
       "line_style": "solid",
    }, 
    # HTTP containerized
    {
       "full_command": "docker run --rm statistics_calc_executable",
       "endpoint": "/run_query/httpconn/containerized/execlibcurl/-/-/docker",
       "config_name": "Executable using libcurl, run in Docker container",
       "line_color": "#0055b3",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm statistics_calc_executable",
       "endpoint": "/run_query/httpconn/containerized/execlibcurl/-/-/podman",
       "config_name": "Executable using libcurl, run in Podman container",
       "line_color": "#0055b3",
       "line_style": "dotted",
    }, 
    {
       "full_command": "docker run --rm statistics_calc_wasm_node",
       "endpoint": "/run_query/httpconn/containerized/wasmfetch/-/node/docker",
       "config_name": "Wasm bytecode + JS glue code using Fetch API, run in Node runtime in Docker container",
       "line_color": "#3d8c40",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm statistics_calc_wasm_node",
       "endpoint": "/run_query/httpconn/containerized/wasmfetch/-/node/podman",
       "config_name": "Wasm bytecode + JS glue code using Fetch API, run in Node runtime in Podman container",
       "line_color": "#3d8c40",
       "line_style": "dotted",
    }, 
    {
       "full_command": "docker run --rm statistics_calc_wasm_bun",
       "endpoint": "/run_query/httpconn/containerized/wasmfetch/-/bun/docker",
       "config_name": "Wasm bytecode + JS glue code using Fetch API, run in Bun runtime in Docker container",
       "line_color": "#dbba51",
       "line_style": "dashed",
    }, 
    {
       "full_command": "podman run --rm statistics_calc_wasm_bun",
       "endpoint": "/run_query/httpconn/containerized/wasmfetch/-/bun/podman",
       "config_name": "Wasm bytecode + JS glue code using Fetch API, run in Bun runtime in Podman container",
       "line_color": "#dbba51",
       "line_style": "dotted",
    }
]

tuple_quantities = [100, 160, 270, 450, 750, 1200, 2000, 3500, 5600, 9400, 15500, 25500, 45000, 70000, 115000, 190000, 320000, 530000, 880000, 1500000]
x_tick_labels = ["100", "160", "270", "450", "750", "1.2k", "2.0k", "3.5k", "5.6k", "9.4k", "15.5k", "25.5k", "45k", "70k", "115k", "190k", "320k", "530k", "880k", "1.5M"]
time_ticks = [0.001, 0.005, 0.025, 0.1, 0.5, 2, 10, 50, 240]

##############################################################################################################
# Definizione della funzione generica che realizza un grafico contenente le spezzate relative ad 
# un insieme configurabile di comandi all'interno dello stesso piano di coordinate
def make_graph(dataframe, configs, title, filepath):
    plotter.figure(figsize=(17,6))

    for config in configs:
        subset = dataframe[dataframe["configuration"] == config[exec_info]]
        plotter.plot(subset["tuples"], 
                     subset["mean"], 
                     linestyle=config["line_style"],
                     linewidth=2,
                     label=config["config_name"], 
                     color=config["line_color"])

        # Aggiunta intervalli verticali attorno ai punti (rimossi per chiarezza grafica)
        # plotter.errorbar(
            # subset["tuples"],
            # subset["mean"],
            # yerr=[subset["mean"] - subset["min"], subset["max"] - subset["mean"]],
            # fmt="none", color="gray", alpha=0.5, capsize=5, elinewidth=1.5, ecolor="black")

    plotter.title(title, fontsize=17, fontweight="semibold", pad=21)
    plotter.legend(loc="upper center", 
                   bbox_to_anchor=(0.5, -0.2),
                   fontsize=11, 
                   ncol=2)
    plotter.grid(True)
    
    ax = plotter.gca()
    # ax.set_autoscale_on(False)
    
    plotter.xlabel("Number of tuples", fontsize=12, labelpad=15)
    plotter.xscale("log")
    ax.xaxis.set_major_locator(FixedLocator(tuple_quantities))
    ax.xaxis.set_ticklabels(x_tick_labels)
    ax.xaxis.set_minor_locator(NullLocator())
    
    plotter.ylabel("Average time (s)", fontsize=12, labelpad=15)
    plotter.yscale("log")
    plotter.ylim(time_ticks[0], time_ticks[-1])
    ax.yaxis.set_major_locator(FixedLocator(time_ticks))
    ax.yaxis.set_major_formatter(FixedFormatter([str(tick) for tick in time_ticks]))
    ax.yaxis.set_minor_locator(NullLocator())
    
    plotter.savefig(filepath, bbox_inches="tight", dpi=300)
    
##############################################################################################################
# Chiamate alla funzionemake_graph() per realizzare e salvare su file gli effettivi grafici

dataframe = pandas.read_csv(src_csv)

# FS - not containerized
make_graph(dataframe, [all_configurations[0]], title="Native", filepath=f"{opt}_graphs/A_FSnoncont_native.jpg")
make_graph(dataframe, [all_configurations[1], 
                       all_configurations[2]], title="Webassembly + WASI (no JS)", filepath=f"{opt}_graphs/B_FSnoncont_wasmwasi.jpg")
make_graph(dataframe, [all_configurations[3], 
                       all_configurations[4], 
                       all_configurations[5],
                       all_configurations[6]], title="Webassembly + JS", filepath=f"{opt}_graphs/C_FSnoncont_wasmjs.jpg")

# FS - containerized
make_graph(dataframe, [all_configurations[7], 
                       all_configurations[8]], title="Native", filepath=f"{opt}_graphs/D_FScont_native.jpg")
make_graph(dataframe, [all_configurations[9], 
                       all_configurations[10], 
                       all_configurations[11], 
                       all_configurations[12]], title="Webassembly + WASI (no JS)", filepath=f"{opt}_graphs/E_FSnoncont_wasmwasi.jpg")
make_graph(dataframe, [all_configurations[13], 
                       all_configurations[14],
                       all_configurations[15],
                       all_configurations[16],
                       all_configurations[17],
                       all_configurations[18],
                       all_configurations[19],
                       all_configurations[20]], title="Webassembly + JS", filepath=f"{opt}_graphs/F_FScont_wasmjs.jpg")

# HTTP - not containerized
make_graph(dataframe, [all_configurations[21]], title="Native", filepath=f"{opt}_graphs/G_HTTPnoncont_native.jpg")
make_graph(dataframe, [all_configurations[22], 
                       all_configurations[23]], title="Webassembly + JS", filepath=f"{opt}_graphs/H_HTTPnoncont_wasmjs.jpg")

# HTTP - containerized
make_graph(dataframe, [all_configurations[24], 
                       all_configurations[25]], title="Native", filepath=f"{opt}_graphs/I_HTTPcont_native.jpg")
make_graph(dataframe, [all_configurations[26], 
                       all_configurations[27],
                       all_configurations[28],
                       all_configurations[29]], title="Webassembly + JS", filepath=f"{opt}_graphs/J_HTTPcont_wasmjs.jpg")
