#!/bin/bash

# Build Plugalyzer
cmake -S . -B build
cmake --build build

# Build JUCE example plugin to Plugalyze
cmake -S JUCE  -B juce_build -DJUCE_BUILD_EXAMPLES=On
cmake --build juce_build --target DSPModulePluginDemo_VST3

dsp_plugin="./juce_build/examples/Plugins/DSPModulePluginDemo_artefacts/Debug/VST3/DSPModulePluginDemo.vst3"
alias plugalzer="./build/Plugalyzer_artefacts/Debug/Plugalyzer"
mkdir -p test

plugalzer -v
plugalzer -h

plugalzer \
    listParameters \
    -p "$dsp_plugin" \
    -o ./test/generateAutomation.json \
    -y

plugalzer \
  generateAutomation \
  -p "$dsp_plugin" \
  -o ./test/generateAutomation.json \
  -y

plugalzer \
  busLayouts \
  -p "$dsp_plugin" \
  -o ./test/generateAutomation.json \
  -y

plugalzer \
  process \
  --plugin="$dsp_plugin" \
  --input=./test/noise-st.wav \
  --output=./test/noise-st-proc.wav \
  --paramFile=./test/out.json \
  -y

plugalzer \
  audioDiff \
  -p "$dsp_plugin" \
  -o ./test/generateAutomation.json \
  -y


'./build/Plugalyzer_artefacts/Debug/Plugalyzer' \
  process \
  --plugin="$dsp_plugin" \
  --input=./test/noise-st.wav \
  --output=./test/noise-st-proc.wav \
  --paramFile=./test/out.json \
  -y
