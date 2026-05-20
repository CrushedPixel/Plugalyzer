from pathlib import Path
from subprocess import run
from typing import List

from test_utils import TestPaths


class TestPrep:
    """A command which will need to be run in preparation for a test case"""

    paths: TestPaths
    command: List
    prepped_data: Path

    def __init__(self, paths: TestPaths) -> None:
        self.paths = paths

    def prep_test(self):
        run([self.paths.plugalyzer] + self.command, check=True)

    def cleanup(self):
        if self.prepped_data and self.prepped_data.exists():
            self.prepped_data.unlink()



class ProcessWithAudioAndGeneratorSidechainPrep(TestPrep):
    def __init__(self, paths: TestPaths) -> None:
        super().__init__(paths)
        self.prepped_data = paths.output_folder / "process-with-audio-and-generator-input.wav"
        self.command = [
            "process", "-p", paths.plugalyzee,
            f"-g", f"{paths.config_folder / "generator-2ch-sine-880.json"}",
            "-o", self.prepped_data
        ]


class StateDefaultBinaryToJsonParamsPrep(TestPrep):
    def __init__(self, paths: TestPaths) -> None:
        super().__init__(paths)
        self.prepped_data = paths.output_folder / "state-binary-to-params.bin"
        self.command = [
            "state", "-p", paths.plugalyzee,
            "-o", self.prepped_data
        ]

class AudioDiffSucceedPrep(TestPrep):
    def __init__(self, paths: TestPaths) -> None:
        super().__init__(paths)
        self.prepped_data = paths.output_folder / "audiodiff-succeed-input.wav"
        self.command = [
            "process", "-p", paths.plugalyzee,
            f"-g", f"{paths.config_folder / "generator-2ch-noise.json"}",
            "-o", self.prepped_data
        ]

class AudioDiffFailPrep(TestPrep):
    def __init__(self, paths: TestPaths) -> None:
        super().__init__(paths)
        self.prepped_data_t = paths.output_folder / "audiodiff-succeed-input-t.wav"
        self.prepped_data_r = paths.output_folder / "audiodiff-succeed-input-r.wav"
        self.commands = (
            [
                "process", "-p", paths.plugalyzee,
                f"-g", f"{paths.config_folder / "generator-2ch-noise.json"}",
                "-o", self.prepped_data_t
            ],
            [
                "process", "-p", paths.plugalyzee,
                f"-g", f"{paths.config_folder / "generator-2ch-noise.json"}",
                "-o", self.prepped_data_r,
                "--param", "In Gain:0.15"
            ],
        )

    def prep_test(self):
        for cmd in self.commands:
            run([self.paths.plugalyzer] + cmd, check=True)

    def cleanup(self):
        if hasattr(self, 'prepped_data'):
            if self.prepped_data and self.prepped_data.exists():
                self.prepped_data.unlink()
        if self.prepped_data_r and self.prepped_data_r.exists():
            self.prepped_data_r.unlink()
        if self.prepped_data_t and self.prepped_data_t.exists():
            self.prepped_data_t.unlink()
        
