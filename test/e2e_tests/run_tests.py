import subprocess
import shutil
from pathlib import Path
import logging
from datetime import datetime
import sys

import test_cases
from test_utils import TestPaths

def set_up_logging():
    """
    Send debug logs to terminal if you're watching and to a file if you're not.
    """

    log_filename = f'e2e-{datetime.strftime(datetime.now(), "%Y-%m-%d_%H-%M-%S")}.log'

    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)

    stdout_handler = logging.StreamHandler()
    stdout_handler.setLevel(logging.DEBUG)
    stdout_handler.setFormatter(logging.Formatter('%(asctime)s %(message)s', datefmt='%H:%M:%S'))
    logger.addHandler(stdout_handler)

    if not sys.stdout.isatty():
        file_handler = logging.FileHandler(log_filename, mode='w')
        file_handler.setLevel(logging.INFO)
        file_handler.setFormatter(logging.Formatter('%(asctime)s %(message)s', datefmt='%H:%M:%S'))
        logger.addHandler(file_handler)


def run_command(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def build_plugalyzer() -> str:
    plugalyzer_path = "build/Plugalyzer_artefacts/Debug/Plugalyzer"
    if sys.platform == "win32":
        plugalyzer_path = plugalyzer_path + ".exe"
    if not Path(plugalyzer_path).exists():
        shutil.rmtree("build", ignore_errors=True)
        run_command(["cmake", "-S", ".", "-B", "build"])
        run_command(["cmake", "--build", "build"])
    return plugalyzer_path


def build_plugalyzee() -> str:
    plugalyzee_path = "test/output/PlugalyzeeAudio/VST3/PlugalyzeeAudio.vst3"
    if not Path(plugalyzee_path).exists():
        shutil.rmtree("test/PlugalyzeeAudio/build", ignore_errors=True)
        run_command(["cmake", "-S", "test/PlugalyzeeAudio", "-B", "test/PlugalyzeeAudio/build"])
        run_command(["cmake", "--build", "test/PlugalyzeeAudio/build", "--config", "Release"])
        Path("test/output/PlugalyzeeAudio/VST3").mkdir(parents=True, exist_ok=True)
        shutil.move("test/PlugalyzeeAudio/build/PlugalyzeeAudio_artefacts/Release/VST3/PlugalyzeeAudio.vst3", "test/output/PlugalyzeeAudio/VST3/PlugalyzeeAudio.vst3")
    return plugalyzee_path


def build_plugalyzee_sidechain() -> str:
    plugalyzee_path = "test/output/PlugalyzeeAudio_sidechain/VST3/PlugalyzeeAudio.vst3"
    if not Path(plugalyzee_path).exists():
        shutil.rmtree("test/PlugalyzeeAudio/build", ignore_errors=True)
        run_command(["cmake", "-S", "test/PlugalyzeeAudio", "-B", "test/PlugalyzeeAudio/build", "-DCMAKE_CXX_FLAGS=-DPLUGALYZEE_HAS_SIDECHAIN=1"])
        run_command(["cmake", "--build", "test/PlugalyzeeAudio/build", "--config", "Release"])
        Path("test/output/PlugalyzeeAudio_sidechain/VST3").mkdir(parents=True, exist_ok=True)
        shutil.move("test/PlugalyzeeAudio/build/PlugalyzeeAudio_artefacts/Release/VST3/PlugalyzeeAudio.vst3", "test/output/PlugalyzeeAudio_sidechain/VST3/PlugalyzeeAudio.vst3")
    return plugalyzee_path


def main():
    paths = TestPaths()
    paths.plugalyzer = build_plugalyzer()
    paths.plugalyzee = build_plugalyzee()
    paths.plugalyzee_sidechain = build_plugalyzee_sidechain()
    paths.config_folder = Path('test/configs')
    paths.output_folder = Path('test/output')
    paths.expected_folder = Path('test/expected')

    failures = test_cases.FailureLogger()
    incomplete = []

    tests = test_cases.get_all_test_cases(failures, paths)

    for test in tests:
        try:
            with test as t:
                t.prep_command()
                t.run_command()
                t.verify_output()
        except Exception as e:
            logging.exception(f"Test didn't complete: {test.description}")
            incomplete.append(test)

    for fail in failures:
        logstring = f"Failed: {fail.description}\n"
        if isinstance(fail.correct_output, bytes):
            logstring += f"Expected: {fail.correct_output.hex()}\n"
            logstring += f"Got:      {fail.output.hex()}\n"
        else:
            logstring += f"Expected: {fail.correct_output}\n"
            logstring += f"Got:      {fail.output}\n"
        if fail.correct_exit_code != 0:
            logstring += f"Expected exit code {fail.correct_exit_code}, got {fail.exit_code}\n"

        logging.error(logstring)

    if incomplete:
        print("Incomplete tests:")
        for broken in incomplete:
            print("\t" + broken.description)

    if failures:
        logging.error("Failed tests:")
        for fail in failures:
            logging.error("\t" + fail.description)

        if sys.stdout.isatty():
            failures.generate_vscode_launch_json()
            print("Generated vscode launch.json so you can debug them.")
        
        exit(1)
        
    if not failures and not incomplete:
        logging.info("All tests passed")
        exit(0)
    else:
        exit(1)
    
    

if __name__ == "__main__":
    set_up_logging()
    main()
