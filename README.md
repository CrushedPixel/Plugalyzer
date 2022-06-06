# Plugalyzer
A command-line VST3, AU and LADSPA host meant to ease debugging of audio plugins and instruments by making it possible to run them in non-realtime outside of a conventional DAW.

It processes audio and MIDI from input files using the desired plugin, writing the result to an output file.  
Plugins with multiple input buses (such as sidechains) are supported.

# Table of Contents
- [Usage](#usage)
  - [Process audio files](#process-audio-files)
    - [Bus layouts](#bus-layouts) 
    - [Parameter automation](#parameter-automation)
    - [Processing limitations](#processing-limitations)
  - [List plugin parameters](#list-plugin-parameters)
    - [Limitations](#limitations)
- [Installation](#installation)

# Usage
The general usage of plugalyzer follows the pattern `plugalyzer [command] [options...]`.  
Using the `--help` flag, detailed usage information can be obtained for every command.

## Process audio files
The `process` command processes the given audio and/or MIDI files using the given plugin in non-realtime,
writing the processed audio to an output file.

| Option                       | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  | Required                         |
|------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------|
| `--plugin=<path>`            | Path to, or identifier of the plugin to use.                                                                                                                                                                                                                                                                                                                                                                                                                                                 | Yes                              |
| `--input=<path>`             | Path to an audio input file.<br>To supply multiple input files, provide the `--input` argument multiple times.                                                                                                                                                                                                                                                                                                                                                                               | Yes, unless `--midiInput` is set |
| `--midiInput=<path>`         | Path to a MIDI input file.                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | No                               |
| `--output=<path>`            | Path to write the processed audio to.                                                                                                                                                                                                                                                                                                                                                                                                                                                        | Yes                              |
| `--overwrite`                | Overwrite the output file if it exists.<br>If this option is not set, processing is aborted if the output file exists.                                                                                                                                                                                                                                                                                                                                                                       | No                               |
| `--sampleRate=<number>`      | The sample rate to use for processing.<br>Only allowed if no audio input is provided.<br>Defaults to 44100.                                                                                                                                                                                                                                                                                                                                                                                  | No                               |
| `--blockSize=<number>`       | The amount of samples to send to the audio plugin at once for processing.<br>Defaults to 1024.                                                                                                                                                                                                                                                                                                                                                                                               | No                               |
| `--outChannels=<number>`     | The amount of channels to use for the plugin's output bus. Defaults to the amount of channels of the first input file.                                                                                                                                                                                                                                                                                                                                                                       | No                               |
| `--paramFile=<path>`         | Specifies a JSON file to read parameter and automation data from. For more information, refer to [Parameter automation](#parameter-automation)                                                                                                                                                                                                                                                                                                                                               | No                               | 
| `--param=<name>:<value>[:n]` | Sets the plugin parameter with the given name or index to the given value.<br>Both `name` and `value` can be quoted using single or double quotes.<br>If the `:n` suffix is given, the value is treated as a normalized value between 0 and 1, otherwise the string will be converted to the normalized value.<br>To set multiple parameters, supply the `--param` argument multiple times.<br>Use the [`listParameters`](#list-plugin-parameters) command to list all available parameters. | No                               |

Example usage for a plugin with a main and a sidechain input bus:
```shell
plugalyzer process                    \
  --plugin=/path/to/my/plugin.vst3    \
  --input=main_input.wav              \
  --input=sidechain_input.wav         \
  --output=out.wav                    \
  --param="Wet/Dry Mix":0.2:n         \
  --param=Distortion:Off
```

### Parameter automation
Aside from the `--param` option, plugin parameters can also be supplied via JSON file using the `--paramFile` option.  
This JSON file also allows for automation by supplying multiple keyframes that are linearly interpolated between.

The JSON file's main object contains an entry for each parameter.

Parameter values can either be a (floating-point) number or a string.  
Numbers are interpreted as normalized parameter values between 0 and 1.  
Strings are passed to the parameter's `getValueForText` function to convert them to normalized values.  
Note that string values can only be supplied for parameters that support text-to-value conversion.

Keyframe times can be specified in the following formats:
- a string containing an integer number is interpreted as a sample index.
- a string suffixed with `s` is interpreted as an amount of seconds.
- a string suffixed with `%` is interpreted as percentage relative to the total duration of the input files.

The following example illustrates the different options.
Please note that comments aren't supported by JSON, and need to be removed before using this example.

```json5
{
    "Power": "On",   // no automation
    "Cutoff": 0.2,   // no automation, normalized value

    // keyframe locations specified in samples:
    "Dry/Wet Mix": { 
        // keyframe at 0 is not required, 
        // it will use the value of the first keyframe until that keyframe is reached
        "13024": "50%",
        "22050": "70%",
        "52316": "20%"
    },

    // keyframe locations specified in seconds:
    "Saturation": {  
        "0s":     "20%", 
        "2.012s": "40%",
        "6.5s":   "100%"
    },

    // keyframe locations specified in percentage of total input duration:
    "Detune": {
        "0%":   "12",
        "50%":  "-12",
        "100%": "12"
    },

    // it's possible to mix keyframe time units:
    "Fuzziness": {
        "0":   0.15,
        "50%": 0,
        "7s":  1
    }
}
```

### Bus layouts
The bus layout requested from the plugin is based on the audio input files.
Each audio input file is provided to the plugin on a separate bus, each bus having the same amount of channels as the respective input file.

Plugalyzer only supports a single output bus. The amount of channels can be specified using the `--outChannels` option.
If `--outChannels` is not set, it defaults to the amount of channels of the first audio input file.
If no audio input is provided (e.g. when testing MIDI instruments), the plugin's default output bus layout is used.

### Processing limitations
- Plugalyzer does not support showing plugin GUIs of any kind. Since processing is not done in real-time, this wouldn't be too useful, either way.
- Only a single output bus is supported.

## List plugin parameters
The `listParameters` command lists all available plugin parameters and their value range, as well as whether they support parameter values in text form.

| Option                  | Description                                                                                                                                                                 | Required |
|-------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------|
| `--plugin=<path>`       | Path to, or identifier of the plugin to use.                                                                                                                                | Yes      |
| `--blockSize=<number>`  | The processing block size to initialize the plugin with. This is only needed when a plugin doesn't support initialization with the default block size.<br>Defaults to 1024. | No       |
| `--sampleRate=<number>` | The sample rate to initialize the plugin with. This is only needed when a plugin doesn't support initialization with the default sample rate.<br>Defaults to 44100.         | No       |

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
   Supports text values: true
1: Saturation Frequency
   Values:  Low, Flat, High
   Default: Flat
   Supports text values: true
2: Saturation In
   Values:  Off, On
   Default: Off
   Supports text values: true
3: Saturation
   Values:  0 % to 100 %
   Default: 50 %
  Supports text values: true

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