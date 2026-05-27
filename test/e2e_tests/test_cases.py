import hashlib
import json
import logging
from pathlib import Path
from subprocess import CompletedProcess, run
import sys
from typing import List, Optional, Union
import re

import generate_test_data
from generate_test_data import TestPrep
from test_utils import FailureLogger, TestPaths
from xml_compare import compare_xml_with_tolerance


class TestCase:
    """A test and its desired result"""
    description: str
    command: List[str]
    output_file: Optional[str]
    prep: Optional[TestPrep]
    correct_exit_code: int
    exit_code: int
    correct_output: Union[bytes, str, re.Pattern]
    output: Union[bytes, str]
    failures: FailureLogger
    paths: TestPaths

    def __init__(self, failures: FailureLogger, paths: TestPaths, description: str, command: List, correct_output: Union[bytes, str, re.Pattern]) -> None:
        self.description = description
        self.command = command
        self.output_file = None
        self.prep = None
        self.correct_exit_code = 0
        self.correct_output = correct_output
        self.failures = failures
        self.paths = paths

    def _get_command_output(self, result: CompletedProcess):
        """Get a SHA256 digest or full text output, depending on the type of the expected output"""
        if self.output_file:
            
            if isinstance(self.correct_output, bytes):
                if Path(self.output_file).exists():
                    return hashlib.sha256(Path(self.output_file).read_bytes()).digest()
                else:
                    return b''
            else:
                if Path(self.output_file).exists():
                    return Path(self.output_file).read_text(encoding='utf-8', errors='replace')
                else:
                    return ''
        else:
            if isinstance(self.correct_output, bytes):
                return hashlib.sha256(result.stdout).digest()
            else:
                return result.stdout.decode('utf-8', errors='replace').replace('\r\n', '\n')

    def prep_command(self):
        if self.prep:
            self.prep.prep_test()

    def run_command(self):
        """
        Run the command, check the result against the expected result,
        and add to the failed tests if they don't match
        """
        logging.debug(f"Test: {self.description}")
        result = run([self.paths.plugalyzer] + self.command, capture_output=True)
        if result.stderr:
            logging.warning(result.stderr.decode('utf-8', errors='replace'))

        self.exit_code = result.returncode
        self.output = self._get_command_output(result)

    def verify_output(self):
        failed = False

        if self.exit_code != self.correct_exit_code:
            logging.error(f"{self.description}: exit code expected: {self.correct_exit_code}. Received: {self.exit_code}")
            failed = True
        if isinstance(self.correct_output, bytes):
            failed = failed or self.output != self.correct_output
        elif isinstance(self.correct_output, str):
            failed = failed or self.output != self.correct_output
        elif isinstance(self.correct_output, re.Pattern):
            failed = failed or not self.correct_output.match(self.output)

        if failed:
            self.failures.failed_tests.append(self)

    def vscode_launch_config(self) -> dict:
        dbg_type = "lldb" if sys.platform == "darwin" else "cppdbg"
        return {
            "type": dbg_type,
            "request": "launch",
            "name": self.description,
            "program": "${workspaceFolder}/build/Plugalyzer_artefacts/Debug/Plugalyzer",
            "cwd": "${workspaceFolder}",
            "args": ["./" + x if "/" in x else x for x in self.command],
            "preLaunchTask": "CMake: build"
        }


    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.output_file and Path(self.output_file).exists():
            Path(self.output_file).unlink()
        if self.prep:
            self.prep.cleanup()
        return False


class Version(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Version",
            ["-v"],
            re.compile(r'Plugalyzer version \d*\.\d*\.\d*')
        )


class Help(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Help",
            ["-h"],
            re.compile(
                r"^Command-line audio plugin host\s+Usage: \[OPTIONS\] \[SUBCOMMANDS\]\s+OPTIONS:.*?SUBCOMMANDS:.*?Use 'plugalyzer <subcommand> --help'.*$",
                re.DOTALL | re.MULTILINE
            )
        )

class HelpListParameters(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Help: listParameters",
            ["listParameters", "-h"],
            re.compile(
                r"^Lists a plugin's parameters\s+listParameters \[OPTIONS\]\s+OPTIONS:.*?$",
                re.DOTALL | re.MULTILINE
            )
        )

class HelpGenerateAutomation(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Help: generateAutomation",
            ["generateAutomation", "-h"],
            re.compile(
                r"^Generates an example automation file you can modify\s+generateAutomation \[OPTIONS\]\s+OPTIONS:.*?$",
                re.DOTALL | re.MULTILINE
            )
        )

class HelpBusLayouts(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Help: busLayouts",
            ["busLayouts", "-h"],
            re.compile(
                r"^Outputs bus layouts supported by the plugin\s+busLayouts \[OPTIONS\]\s+OPTIONS:.*?$",
                re.DOTALL | re.MULTILINE
            )
        )

class HelpProcess(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Help: process",
            ["process", "-h"],
            re.compile(
                r"^Processes audio using a plugin\s+process \[OPTIONS\]\s+OPTIONS:.*?$",
                re.DOTALL | re.MULTILINE
            )
        )

class HelpAudioDiff(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Help: audioDiff",
            ["audioDiff", "-h"],
            re.compile(
                r"^Compares two audio files\s+audioDiff \[OPTIONS\]\s+OPTIONS:.*?$",
                re.DOTALL | re.MULTILINE
            )
        )

class HelpState(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Help: state",
            ["state", "-h"],
            re.compile(
                r"^Load parameter automation and save as binary plugin state.*?\s+state \[OPTIONS\]\s+OPTIONS:.*?$",
                re.DOTALL | re.MULTILINE
            )
        )

class ListParametersDefaultStdout(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "List parameters: default, stdout",
            ["listParameters", "-p", paths.plugalyzee],
            (paths.expected_folder / "plug-audio-list-parameters-text.txt").read_text('utf-8')
        )

class ListParametersTextStdout(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "List parameters: text, stdout",
            ["listParameters", "-p", paths.plugalyzee, "-f", "text"],
            (paths.expected_folder / "plug-audio-list-parameters-text.txt").read_text('utf-8')
        )

class ListParametersJsonStdout(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "List parameters: json, stdout",
            ["listParameters", "-p", paths.plugalyzee, "-f", "json"],
            (paths.expected_folder / "plug-audio-list-parameters-json.json").read_text('utf-8')
        )

class ListParametersDefaultFile(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("list-parameters-default.txt")
        super().__init__(failures, paths,
            "List parameters: default, file",
            [
                "listParameters", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
            ],
            (paths.expected_folder / "plug-audio-list-parameters-text.txt").read_text('utf-8')
        )
        self.output_file = outfile

class ListParametersTextFile(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("list-parameters-text.txt")
        super().__init__(failures, paths,
            "List parameters: text, file",
            [
                "listParameters", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
                "-f", "text"
            ],
            (paths.expected_folder / "plug-audio-list-parameters-text.txt").read_text('utf-8')
        )
        self.output_file = outfile

class ListParametersJsonFile(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("list-parameters-json.json")
        super().__init__(failures, paths,
            "List parameters: json, file",
            [
                "listParameters", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
                "-f", "json"
            ],
            (paths.expected_folder / "plug-audio-list-parameters-json.json").read_text('utf-8')
        )
        self.output_file = outfile

class GenerateAutomationStdout(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Generate Automation: stdout",
            ["generateAutomation", "-p", paths.plugalyzee],
            (paths.expected_folder / "plug-audio-generate-automation.json").read_text('utf-8')
        )

class GenerateAutomationFile(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("generate-automation.json")
        super().__init__(failures, paths,
            "Generate Automation: file",
            [
                "generateAutomation", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
            ],
            (paths.expected_folder / "plug-audio-generate-automation.json").read_text('utf-8')
        )
        self.output_file = outfile

class BusLayoutsDefaultStdout(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Bus layouts: default, stdout",
            ["busLayouts", "-p", paths.plugalyzee],
            (paths.expected_folder / "plug-audio-bus-layouts.txt").read_text('utf-8')
        )

class BusLayoutsTextStdout(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Bus layouts: text, stdout",
            ["busLayouts", "-p", paths.plugalyzee, "-f", "text"],
            (paths.expected_folder / "plug-audio-bus-layouts.txt").read_text('utf-8')
        )

class BusLayoutsJsonStdout(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        super().__init__(failures, paths,
            "Bus layouts: json, stdout",
            ["busLayouts", "-p", paths.plugalyzee, "-f", "json"],
            (paths.expected_folder / "plug-audio-bus-layouts.json").read_text('utf-8')
        )

class BusLayoutsDefaultFile(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("buslayouts-default.txt")
        super().__init__(failures, paths,
            "Bus layouts: default, file",
            [
                "busLayouts", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
            ],
            (paths.expected_folder / "plug-audio-bus-layouts.txt").read_text('utf-8')
        )
        self.output_file = outfile

class BusLayoutsTextFile(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("buslayouts-text.txt")
        super().__init__(failures, paths,
            "Bus layouts: text, file",
            [
                "busLayouts", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
                "-f", "text"
            ],
            (paths.expected_folder / "plug-audio-bus-layouts.txt").read_text('utf-8')
        )
        self.output_file = outfile

class BusLayoutsJsonFile(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("buslayouts-json.json")
        super().__init__(failures, paths,
            "Bus layouts: json, file",
            [
                "busLayouts", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
                "-f", "json",
            ],
            (paths.expected_folder / "plug-audio-bus-layouts.json").read_text('utf-8')
        )
        self.output_file = outfile

class AudiodiffSucceed(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        prep = generate_test_data.AudioDiffSucceedPrep(paths)
        super().__init__(failures, paths,
            "Audiodiff: succeed",
            [
                "audioDiff",
                "-t", f"{prep.prepped_data}",
                "-r", f"{prep.prepped_data}"
            ],
            "Reading audio files\nDifference in RMS between test audio and reference: 0\nCancellation test successful.\n"
        )
        self.prep = prep

class AudiodiffFail(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        prep = generate_test_data.AudioDiffFailPrep(paths)
        super().__init__(failures, paths,
            "Audiodiff: fail",
            [
                "audioDiff",
                "-t", f"{prep.prepped_data_t}",
                "-r", f"{prep.prepped_data_r}"
            ],
            re.compile(
                r"^Reading audio files\sDifference in RMS between test audio and reference.*?\sCancellation test failed.*?\s$",
                re.DOTALL | re.MULTILINE
            )
        )
        self.prep = prep
        self.correct_exit_code = 2

class ProcessWithGenerator(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("process-with-generator.wav")
        super().__init__(failures, paths,
            "Process with generator",
            [
                "process", "-p", paths.plugalyzee,
                "-g", paths.config('generator-2ch-sine-noise.json'),
                "-o", f"{outfile}",
                "--paramFile", paths.config('plug-audio-process-with-generator.json')
            ],
            bytes.fromhex("98ea9a73c5e839aa46ec4b963e3534bc1ca519e518bd9640e7110258be939c4f")
        )
        self.output_file = outfile

    def verify_output(self):
        if sys.platform != 'darwin':
            return super().verify_output()
        
        expected_output = self.paths.expected('process-with-generator.wav')
        cmd = [
            "audioDiff",
            "-t", self.output_file,
            "-r", expected_output
        ]

        result = run([self.paths.plugalyzer] + cmd, capture_output=True)

        failed = result.returncode != 0
        if failed:
            self.failures.failed_tests.append(self)

class ProcessWithGeneratorTextInput(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("process-with-generator.wav")
        generator_config = json.loads((paths.config_folder / 'generator-2ch-sine-noise.json').read_text('utf-8'))

        super().__init__(failures, paths,
            "Process with generator using configuration on command line",
            [
                "process", "-p", paths.plugalyzee,
                "-g", json.dumps(generator_config),
                "-o", f"{outfile}",
                "--paramFile", paths.config('plug-audio-process-with-generator.json')
            ],
            bytes.fromhex("98ea9a73c5e839aa46ec4b963e3534bc1ca519e518bd9640e7110258be939c4f")
        )
        self.output_file = outfile

    def verify_output(self):
        if sys.platform != 'darwin':
            return super().verify_output()
        
        expected_output = self.paths.expected('process-with-generator.wav')
        cmd = [
            "audioDiff",
            "-t", self.output_file,
            "-r", expected_output
        ]

        result = run([self.paths.plugalyzer] + cmd, capture_output=True)

        failed = result.returncode != 0
        if failed:
            self.failures.failed_tests.append(self)

class ProcessWithAudioAndGeneratorSidechain(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        prep = generate_test_data.ProcessWithAudioAndGeneratorSidechainPrep(paths)
        outfile = paths.output("process-with-audio-and-generator.wav")
        super().__init__(failures, paths,
            "Process with audio input and generator sidechain",
            [
                "process", "-p", paths.plugalyzee_sidechain,
                "-i", f"{prep.prepped_data}",
                "-g", paths.config("generator-2ch-sine-440.json"),
                "-o", f"{outfile}",
            ],
            bytes.fromhex("7a8ea3f9e22a65f1f80c4fa8e655ebf393f4e6512f2c6018418e13c1a0f5fb46")
        )
        self.output_file = outfile
        self.prep = prep

    def verify_output(self):
        if sys.platform != 'darwin':
            return super().verify_output()
        
        expected_output = self.paths.expected('process-with-audio-and-generator.wav')
        cmd = [
            "audioDiff",
            "-t", self.output_file,
            "-r", expected_output
        ]

        result = run([self.paths.plugalyzer] + cmd, capture_output=True)

        failed = result.returncode != 0
        if failed:
            self.failures.failed_tests.append(self)

class ProcessSidechainMissingSidechain(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("process-with-audio-missing-sidechain.wav")
        super().__init__(failures, paths,
            "Process sidechain plugin with no sidechain supplied by user",
            [
                "process", "-p", paths.plugalyzee_sidechain,
                "-g", paths.config("generator-2ch-sine-noise.json"),
                "-o", f"{outfile}",
            ],
            bytes.fromhex("15777e34e030212bf14decccd6f94fca0039b6477fa407684e8a3d797f70b8fb")
        )
        self.output_file = outfile

    def verify_output(self):
        if sys.platform != 'darwin':
            return super().verify_output()
        
        expected_output = self.paths.expected('process-with-audio-missing-sidechain.wav')
        cmd = [
            "audioDiff",
            "-t", self.output_file,
            "-r", expected_output
        ]

        result = run([self.paths.plugalyzer] + cmd, capture_output=True)

        failed = result.returncode != 0
        if failed:
            self.failures.failed_tests.append(self)

class StateSaveDefaultBinary(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("plug-audio-state-default.bin")
        expected_hash = (
            "6dd1271b0d556c2c869bd6842e2859a515f5c83ac400118eca76bf58acbb45df"
            if sys.platform == 'darwin'
            else "882b3ac1ead9aa5283a3c2c6685e4f7095490244b6c6b653dc558ec46348c3db"
        )
        super().__init__(failures, paths,
            "Save default state as binary",
            [
                "state", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
            ],
            bytes.fromhex(expected_hash)
        )
        self.output_file = outfile

class SaveDefaultStateXml(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("plug-audio-state-default.xml")
        super().__init__(failures, paths,
            "Save default state as xml",
            [
                "state", "-p", paths.plugalyzee,
                "-o", f"{outfile}",
                "-f", "xml"
            ],
            (paths.expected_folder / "plug-audio-state-default.xml").read_text('utf-8')
        )
        self.output_file = outfile

    def verify_output(self):
        if sys.platform != 'darwin':
            return super().verify_output()
        
        success, err = compare_xml_with_tolerance(self.output, self.correct_output) # type: ignore
        
        if not success:
            logging.error(err)
            self.failures.failed_tests.append(self)

class StateDefaultBinaryToJsonParams(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        prep = generate_test_data.StateDefaultBinaryToJsonParamsPrep(paths)
        outfile = paths.output("plug-audio-state-default.json")
        super().__init__(failures, paths,
            "State as binary -> JSON params",
            [
                "state", "-p", paths.plugalyzee,
                "-i", f"{prep.prepped_data}",
                "-o", f"{outfile}",
            ],
            (paths.expected_folder / "plug-audio-state-default-params.json").read_text('utf-8')
        )
        self.output_file = outfile
        self.prep = prep

class StateJsonParamsToBinary(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("plug-audio-state-params.bin")
        expected_hash = (
            "c7df17b00866bd5fd952e18f42b82b6d76010ec9df2b6bfb103d1ef10b1dd342"
            if sys.platform == 'darwin'
            else "1fbf069efe5d7d6543c55a25766fa9f08feffc1c41131abaed1063f739b864fe"
        )
        super().__init__(failures, paths,
            "JSON params -> state as binary",
            [
                "state", "-p", paths.plugalyzee,
                "-i", paths.config("plug-audio-state-json-params.json"),
                "-o", f"{outfile}",
            ],
            bytes.fromhex(expected_hash)
        )
        self.output_file = outfile

class StateJsonParamsToXml(TestCase):
    def __init__(self, failures: FailureLogger, paths: TestPaths) -> None:
        outfile = paths.output("plug-audio-state-params.xml")
        super().__init__(failures, paths,
            "JSON params -> state as XML",
            [
                "state", "-p", paths.plugalyzee,
                "-i", paths.config("plug-audio-state-json-params.json"),
                "-o", f"{outfile}",
                "-f", "xml",
            ],
            (paths.expected_folder / "plug-audio-state-params.xml").read_text('utf-8')
        )
        self.output_file = outfile

    def verify_output(self):
        if sys.platform != 'darwin':
            return super().verify_output()
        
        success, err = compare_xml_with_tolerance(self.output, self.correct_output) # type: ignore
        
        if not success:
            logging.error(err)
            self.failures.failed_tests.append(self)


def get_all_test_cases(failures: FailureLogger, paths: TestPaths) -> List[TestCase]:
    return [
        Version(failures, paths),
        Help(failures, paths),
        HelpListParameters(failures, paths),
        HelpGenerateAutomation(failures, paths),
        HelpBusLayouts(failures, paths),
        HelpProcess(failures, paths),
        HelpAudioDiff(failures, paths),
        HelpState(failures, paths),
        ListParametersDefaultStdout(failures, paths),
        ListParametersTextStdout(failures, paths),
        ListParametersJsonStdout(failures, paths),
        ListParametersDefaultFile(failures, paths),
        ListParametersTextFile(failures, paths),
        ListParametersJsonFile(failures, paths),
        GenerateAutomationStdout(failures, paths),
        GenerateAutomationFile(failures, paths),
        BusLayoutsDefaultStdout(failures, paths),
        BusLayoutsTextStdout(failures, paths),
        BusLayoutsJsonStdout(failures, paths),
        BusLayoutsDefaultFile(failures, paths),
        BusLayoutsTextFile(failures, paths),
        BusLayoutsJsonFile(failures, paths),
        AudiodiffSucceed(failures, paths),
        AudiodiffFail(failures, paths),
        ProcessWithGenerator(failures, paths),
        ProcessWithGeneratorTextInput(failures, paths),
        ProcessWithAudioAndGeneratorSidechain(failures, paths),
        ProcessSidechainMissingSidechain(failures, paths),
        StateSaveDefaultBinary(failures, paths),
        SaveDefaultStateXml(failures, paths),
        StateDefaultBinaryToJsonParams(failures, paths),
        StateJsonParamsToBinary(failures, paths),
        StateJsonParamsToXml(failures, paths),
    ]
