# Plugalyzer
Plugalyzer is a command-line VST3, AU and LADSPA host meant to ease debugging of audio plugins by making it possible to run them in non-realtime outside of a conventional DAW.

It processes audio from input files using the desired plugin, writing the result to an output file.  
Plugins with multiple input buses (such as sidechains) are supported.

## Usage
| Option                   | Description                                                                                                            | Required |
|--------------------------|------------------------------------------------------------------------------------------------------------------------|----------|
| `--plugin=<path>`        | Path to, or identifier of the plugin to use.                                                                           | Yes      |
| `--input=<path>`         | Path to an input file.<br>To supply multiple input files, provide the `--input` argument multiple times.               | Yes      |
| `--output=<path>`        | Path to write the processed audio to.                                                                                  | Yes      |
| `--override`             | Overridde the output file if it exists.<br>If this option is not set, processing is aborted if the output file exists. | No       |
| `--blockSize=<number>`   | The amount of samples to send to the audio plugin at once for processing. Defaults to 1024.                            | No       |
| `--outChannels=<number>` | The amount of channels to use for the plugin's output bus. Defaults to the amount of channels of the first input file. | No       | 

Example usage for a plugin with a main and a sidechain input bus:
```shell
plugalyzer --plugin=/path/to/my/plugin.vst3 \
  --input=main_input.wav                    \
  --input=sidechain_input.wav               \
  --output=out.wav
```

## Bus layouts
The bus layout requested from the plugin is based on the input files.
Each input file is provided to the plugin on a separate bus, each bus having the same amount of channels as the respective input file.

Plugalyzer only supports a single output bus, defaulting to the same amount of channels as the main input bus.
This can be overridden using the `--outChannels` option.
