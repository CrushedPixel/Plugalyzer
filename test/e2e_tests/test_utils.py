import json
from pathlib import Path
from typing import List


class FailureLogger:
    """Keeps track of all the failed tests"""
    failed_tests: List

    def generate_vscode_launch_json(self):
        configs = []
        for fail in self.failed_tests:
            configs.append(fail.vscode_launch_config())

        output = {
            "version": "0.2.0",
            "configurations": configs
        }
        with Path("launch.json").open('w') as out:
            json.dump(output, out, indent=4)

    def __init__(self) -> None:
        self.failed_tests = []

    def __iter__(self):
        return iter(self.failed_tests)

    def __len__(self) -> int:
        return len(self.failed_tests)

    def __getitem__(self, index: int):
        return self.failed_tests[index]
    
    def __bool__(self) -> bool:
        return len(self.failed_tests) > 0


class TestPaths:
    def __init__(self):
        self._plugalyzer = ""
        self._plugalyzee = ""
        self._plugalyzee_sidechain = ""
        self._config_folder = Path()
        self._output_folder = Path()
        self._expected_folder = Path()

    def config(self, filename: str) -> str:
        """Get a config file as a string to pass to subprocess.run"""
        return str(self.config_folder / filename)

    def output(self, filename: str) -> str:
        """Get an output file as a string to pass to subprocess.run"""
        return str(self.output_folder / filename)

    def expected(self, filename: str) -> str:
        """Get an expected file as a string to pass to subprocess.run"""
        return str(self.expected_folder / filename)

    @property
    def plugalyzer(self) -> str:
        return self._plugalyzer

    @plugalyzer.setter
    def plugalyzer(self, value: str) -> None:
        if not Path(value).exists():
            raise FileNotFoundError(f"plugalyzer path does not exist: {value}")
        self._plugalyzer = value

    @property
    def plugalyzee(self) -> str:
        return self._plugalyzee

    @plugalyzee.setter
    def plugalyzee(self, value: str) -> None:
        if not Path(value).exists():
            raise FileNotFoundError(f"plugalyzee path does not exist: {value}")
        self._plugalyzee = value

    @property
    def plugalyzee_sidechain(self) -> str:
        return self._plugalyzee_sidechain

    @plugalyzee_sidechain.setter
    def plugalyzee_sidechain(self, value: str) -> None:
        if not Path(value).exists():
            raise FileNotFoundError(f"plugalyzee_sidechain path does not exist: {value}")
        self._plugalyzee_sidechain = value

    @property
    def config_folder(self) -> Path:
        return self._config_folder

    @config_folder.setter
    def config_folder(self, value: Path) -> None:
        if not value.exists():
            raise FileNotFoundError(f"configs path does not exist: {value}")
        self._config_folder = value

    @property
    def output_folder(self) -> Path:
        return self._output_folder

    @output_folder.setter
    def output_folder(self, value: Path) -> None:
        if not value.exists():
            raise FileNotFoundError(f"output path does not exist: {value}")
        self._output_folder = value

    @property
    def expected_folder(self) -> Path:
        return self._expected_folder

    @expected_folder.setter
    def expected_folder(self, value: Path) -> None:
        if not value.exists():
            raise FileNotFoundError(f"expected path does not exist: {value}")
        self._expected_folder = value

