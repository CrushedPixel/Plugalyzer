# Plugalyzer
A command-line VST3, AU and LADSPA host meant to ease debugging of audio plugins by making it possible to run them in non-realtime outside of a conventional DAW.

It processes audio from input files using the desired plugin, writing the result to an output file.  
Plugins with multiple input buses (such as sidechains) are supported.

# Table of Contents
- [Usage](#usage)
  - [Process audio files](#process-audio-files)
    - [Bus layouts](#bus-layouts) 
    - [Processing limitations](#processing-limitations)
  - [List plugin parameters](#list-plugin-parameters)
    - [Limitations](#limitations)
- [Installation](#installation)

# Usage
The general usage of plugalyzer follows the pattern `plugalyzer [command] [options...]`.  
Available commands and their options are described below.

## Process audio files
The `process` command processes one or more audio files using the given plugin in non-realtime,
writing the processed audio to an output file.

| Option                   | Description                                                                                                                                                                                                                                                                                                                      | Required |
|--------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------|
| `--plugin=<path>`        | Path to, or identifier of the plugin to use.                                                                                                                                                                                                                                                                                     | Yes      |
| `--input=<path>`         | Path to an input file.<br>To supply multiple input files, provide the `--input` argument multiple times.                                                                                                                                                                                                                         | Yes      |
| `--output=<path>`        | Path to write the processed audio to.                                                                                                                                                                                                                                                                                            | Yes      |
| `--override`             | Overridde the output file if it exists.<br>If this option is not set, processing is aborted if the output file exists.                                                                                                                                                                                                           | No       |
| `--blockSize=<number>`   | The amount of samples to send to the audio plugin at once for processing. Defaults to 1024.                                                                                                                                                                                                                                      | No       |
| `--outChannels=<number>` | The amount of channels to use for the plugin's output bus. Defaults to the amount of channels of the first input file.                                                                                                                                                                                                           | No       | 
| `--param=<name>:<value>` | Sets the plugin parameter with the given name or index to the given value.<br>Both `name` and `value` can be quoted using single or double quotes.<br>To set multiple parameters, supply the `--param` argument multiple times.<br>Use the [`listParameters`](#list-plugin-parameters) command to list all available parameters. | No       |

Example usage for a plugin with a main and a sidechain input bus:
```shell
plugalyzer process                    \
  --plugin=/path/to/my/plugin.vst3    \
  --input=main_input.wav              \
  --input=sidechain_input.wav         \
  --output=out.wav                    \
  --param="Wet/Dry Mix":0.2           \
  --param=Distortion:Off
```

### Bus layouts
The bus layout requested from the plugin is based on the input files.
Each input file is provided to the plugin on a separate bus, each bus having the same amount of channels as the respective input file.

Plugalyzer only supports a single output bus, defaulting to the same amount of channels as the main input bus.
The amount of output channels can be overridden using the `--outChannels` option.

### Processing limitations
- Plugalyzer does not support showing plugin GUIs of any kind. Since processing is not done in real-time, this wouldn't be too useful, either way.
- Only a single output bus is supported.

## List plugin parameters
The `listParameters` command lists all available plugin parameters and their value range.

| Option                  | Description                                                                                                                                                              | Required |
|-------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------|
| `--plugin=<path>`       | Path to, or identifier of the plugin to use.                                                                                                                             | Yes      |
| `--blockSize=<number>`  | The processing block size to initialize the plugin with. This is only needed when a plugin doesn't support initialization with the default block size. Defaults to 1024. | No       |
| `--sampleRate=<number>` | The sample rate to initialize the plugin with. This is only needed when a plugin doesn't support initialization with the default sample rate. Defaults to 44100          | No       |

Example usage:
```shell
plugalyzer listParameters --plugin=/path/to/my/plugin.vst3
```

Example output:
```shell
Loaded plugin "Black Box Analog Design HG-2".

Plugin parameters: 
0: Power
   Values:  Off, On
   Default: On
1: Saturation Frequency
   Values:  Low, Flat, High
   Default: Flat
2: Saturation In
   Values:  Off, On
   Default: Off
3: Saturation
   Values:  0 % to 100 %
   Default: 50 %

[...]
```

### Limitations
- Plugin parameters with a set of discrete values (i.e. not a floating-point range) that aren't correctly marked as such
  will not list all valid values. This issue arises, for example, when a JUCE plugin uses the deprecated
  `juce::AudioProcessorValueTreeState::Parameter` class where `juce::AudioParameterChoice` would be a better fit.

# Installation
There are currently no pre-built binaries available for download, primarily because I do not have machines with all major operating systems available.  
To compile this project yourself, clone it and build it via CMake.  
I recommend JetBrain's CLion for its excellent CMake support.