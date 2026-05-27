"""
This script can provide some ideas for how you might automate regression testing with Plugalyzer.

To see what it's doing, you can build the test plugin(s) and run/step through it with:
python -m regression-test.py "test/output/PlugalyzeeAudio/VST3/PlugalyzeeAudio.vst3" "test/output/PlugalyzeeAudio_sidechain/VST3/PlugalyzeeAudio.vst3"
"""


import argparse
from difflib import Differ
import io
import json
from os import environ as env
import os
from pprint import pprint
from pathlib import Path
import platform
from tempfile import NamedTemporaryFile
import requests
from subprocess import run
from typing import Dict, List, Optional
import sys
import zipfile

# --------------------------------------------------------------------------------
# ---------------                    Setting up                    ---------------

PLUGALYZER = ""

def find_executable(name) -> Optional[str]:
    """
    Find an executable in the user's path
    """

    if sys.platform == "win32":
        name = name + ".exe"
    for directory in os.get_exec_path():
        path = os.path.join(directory, name)
        if os.path.isfile(path) and os.access(path, os.X_OK):
            return path

    return None


def get_latest_pluglayzer_url() -> str:
    """
    Use the Github API to find the latest release for this OS.
    """
    api_address = "https://api.github.com/repos/CrushedPixel/Plugalyzer/releases/latest"
    response = requests.get(api_address)
    response.raise_for_status()
    dl_info = json.loads(response.content)
    urls = [x['browser_download_url'] for x in dl_info['assets']]
    system = platform.system()
    for url in urls:
        if "macOS" in url and system == 'Darwin':
            return url
        if "Linux" in url and system == 'Linux':
            return url
        if "Windows" in url and system == 'Windows':
            return url
        
    raise RuntimeError("Plugalyzer download not found for your OS.")


def download_and_unzip_plugalyzer(extract_path: str = ".") -> str:
    """
    Download the latest release from Github and unzip it, returning the path.
    """
    system = platform.system()

    dl_url = get_latest_pluglayzer_url()
    response = requests.get(dl_url)
    response.raise_for_status()

    extract_dir = Path(extract_path)
    extract_dir.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(io.BytesIO(response.content)) as zip_ref:
        zip_ref.extractall(extract_dir)

    executable_name = "Plugalyzer" if system != "Windows" else "Plugalyzer.exe"
    plugalyzer_path = extract_dir / executable_name

    if system != "Windows":
        plugalyzer_path.chmod(0o755)

    return str(plugalyzer_path.resolve())


def log_plugalyzer_version():
    """
    Run Plugalyzer to get its version and log the result.
    """
    print(run([PLUGALYZER, '-v'], capture_output=True, text=True).stdout)


def gather_plugalyzer():
    """
    Find a working copy of Plugalyzer and put the path in the global variable.
    Order of priority:
        - Local path
        - System path
        - Download from Github and place in local path
    """
    global PLUGALYZER

    print("Finding plugalyzer")

    print("Checking local path")
    name = "Plugalyzer"
    if sys.platform == "win32":
        name = name + ".exe"
    candidate = Path(name)
    if candidate.exists():
        print("Plugalyzer found in local folder")
        PLUGALYZER = str(Path(candidate).resolve())
        log_plugalyzer_version()
        return

    print("Checking system path")
    candidate = find_executable("Plugalyzer")
    if candidate:
        print("Plugalyzer found in system path")
        PLUGALYZER = str(Path(candidate).resolve())
        log_plugalyzer_version()
        return

    print("Can't find it. Downloading from Github releases")
    PLUGALYZER = download_and_unzip_plugalyzer()
    print("Plugalyzer downloaded")
    log_plugalyzer_version()

# --------------------------------------------------------------------------------
# ---------------           Getting info about a plugin            ---------------

def get_parameter_list(plugin_path: Path) -> Dict:
    """
    Run the listParameters command and get the result as a Dict
    """

    result = run(
        [PLUGALYZER, 'listParameters', f'--plugin={plugin_path}', '-f', 'json'],
        check=True, capture_output=True, text=True
    )
    return json.loads(result.stdout)


def get_parameter_automation(plugin_path: Path) -> Dict:
    """
    Run the generateAutomation command and get the result as a Dict
    """
    result = run(
        [PLUGALYZER, 'generateAutomation', f'--plugin={plugin_path}'],
        check=True, capture_output=True, text=True
    )
    return json.loads(result.stdout)


def get_bus_layouts(plugin_path: Path) -> Dict:
    """
    Run the busLayouts command and get the result as a Dict
    """
    result = run(
        [PLUGALYZER, 'busLayouts', f'--plugin={plugin_path}', '-f', 'json'],
        check=True, capture_output=True, text=True
    )
    return json.loads(result.stdout)


def get_default_state(plugin_path: Path) -> bytes:
    """
    Run the state command and get the result in binary
    """
    result = run(
        [PLUGALYZER, 'busLayouts', f'--plugin={plugin_path}'],
        check=True, capture_output=True
    )
    return result.stdout


def get_parameters_from_state(plugin_path: Path, state: bytes) -> dict:
    """
    Load a binary plugin state and get the resulting parameter values as JSON
    """
    with NamedTemporaryFile() as temp_file:
        temp_file.write(state)
        temp_file.flush()
        
        result = run(
            [
                PLUGALYZER, 'state',
                f'--plugin={plugin_path}',
                f'--input={temp_file.name}',
                f'--format=json',
            ],
            check=True, capture_output=True, text=True
        )

    return json.loads(result.stdout)


def get_state_from_parameters(plugin_path: Path, state: Dict) -> bytes:
    """
    Load a binary plugin state and get the resulting parameter values as JSON
    """
    with NamedTemporaryFile(mode='w') as temp_file:
        json.dump(state, temp_file)
        temp_file.flush()
        
        result = run(
            [
                PLUGALYZER, 'state',
                f'--plugin={plugin_path}',
                f'--input={temp_file.name}',
                f'--format=binary',
            ],
            check=True, capture_output=True
        )

    return result.stdout


# --------------------------------------------------------------------------------
# ---------------         Functions to compare two plugins         ---------------

def diff_parameters(old_plugin: Path, new_plugin: Path) -> List:
    """
    Compare the parameter lists between two plugins.
    """
    d = Differ()
    params_old = json.dumps(get_parameter_list(old_plugin), indent=4).splitlines(keepends=True)
    params_new = json.dumps(get_parameter_list(new_plugin), indent=4).splitlines(keepends=True)
    diffs = list(d.compare(params_old, params_new))

    return diffs


def diff_bus_layouts(old_plugin: Path, new_plugin: Path) -> List:
    """
    Compare the accepted bus layouts between two plugins.
    """
    d = Differ()
    buses_old = json.dumps(get_bus_layouts(old_plugin), indent=4).splitlines(keepends=True)
    buses_new = json.dumps(get_bus_layouts(new_plugin), indent=4).splitlines(keepends=True)
    diffs = list(d.compare(buses_old, buses_new))

    return diffs


def diff_plugin_state(old_plugin: Path, new_plugin: Path) -> List:
    """
    Try loading the default plugin state from one plugin into another
    and compare the resulting parameter values.

    This example uses the default state, but you'll also want to set some parameters
    to modify the state and then try loading that state into the new/second plugin.
    """
    d = Differ()
    default_state_old = get_default_state(old_plugin)
    default_parameters_old = get_parameters_from_state(old_plugin, default_state_old)
    # The old state loaded into the new plugin
    default_parameters_new = get_parameters_from_state(new_plugin, default_state_old)

    old = json.dumps(default_parameters_old, indent=4).splitlines(keepends=True)
    new = json.dumps(default_parameters_new, indent=4).splitlines(keepends=True)
    diffs = list(d.compare(old, new))

    return diffs


def diff_audio(old_plugin: Path, new_plugin: Path) -> List:
    """
    Process some audio with each plugin and use the audioDiff command to compare the outputs
    """
    old_preset = get_parameter_automation(old_plugin)
    new_preset = get_parameter_automation(new_plugin)

    old_preset_file = NamedTemporaryFile(mode='w', suffix='.json', delete=False)
    old_preset_file.write(json.dumps(old_preset))
    old_preset_file.close()
    
    new_preset_file = NamedTemporaryFile(mode='w', suffix='.json', delete=False)
    new_preset_file.write(json.dumps(new_preset))
    new_preset_file.close()

    old_output_file = NamedTemporaryFile(suffix='.wav', delete=False)
    old_output_file.close()
    
    new_output_file = NamedTemporaryFile(suffix='.wav', delete=False)
    new_output_file.close()

    try:
        old_result = run(
            [
                PLUGALYZER,
                'process',
                f'--plugin={old_plugin}',
                f'--generatorInput', '{"sample rate": 48000.0, "num channels": 2, "duration": "2s", "channels": [{"generator": "sine", "frequency": "10kHz", "amplitude": "0.5"}, {"generator": "white noise", "amplitude": "-12dB", "random seed": 1769063962462}]}',
                f'--output={old_output_file.name}',
                f'--paramFile={old_preset_file.name}',
                '--overwrite'
            ],
            check=True, capture_output=True, text=True
        )
        new_result = run(
            [
                PLUGALYZER,
                'process',
                f'--plugin={new_plugin}',
                f'--generatorInput', '{"sample rate": 48000.0, "num channels": 2, "duration": "2s", "channels": [{"generator": "sine", "frequency": "10kHz", "amplitude": "0.5"}, {"generator": "white noise", "amplitude": "-12dB", "random seed": 1769063962462}]}',
                f'--output={new_output_file.name}',
                f'--paramFile={new_preset_file.name}',
                '--overwrite'
            ],
            check=True, capture_output=True, text=True
        )
        diff_result = run(
            [
                PLUGALYZER,
                'audioDiff',
                f'--test={new_output_file.name}',
                f'--reference={old_output_file.name}',
            ],
            check=True, capture_output=True, text=True
        )
        return [old_result, new_result, diff_result]

    finally:
        Path(old_output_file.name).unlink(missing_ok=True)
        Path(old_preset_file.name).unlink(missing_ok=True)
        Path(new_output_file.name).unlink(missing_ok=True)
        Path(new_preset_file.name).unlink(missing_ok=True)



# --------------------------------------------------------------------------------
# ---------------                       Main                       ---------------

if __name__ == '__main__':
    # Initial the global variable and find a working copy of Plugalyzer
    gather_plugalyzer()

    parser = argparse.ArgumentParser()
    parser.add_argument('old_plugin', type=Path)
    parser.add_argument('new_plugin', type=Path)

    args = parser.parse_args()

    print(f"Old plugin: {args.old_plugin}")
    print(f"New plugin: {args.new_plugin}")

    print("Diffing parameters")
    parameter_diff = diff_parameters(args.old_plugin, args.new_plugin)
    pprint(parameter_diff)
    
    print("Diffing layouts")
    layouts_diff = diff_bus_layouts(args.old_plugin, args.new_plugin)
    pprint(layouts_diff)
    
    print("Diffing audio")
    audio_diff = diff_audio(args.old_plugin, args.new_plugin)
    pprint(audio_diff)
