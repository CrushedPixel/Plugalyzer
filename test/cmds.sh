#!/bin/bash

# Build Plugalyzer
cmake -S . -B build
cmake --build build

# Build JUCE example plugin to Plugalyze
cmake -S JUCE  -B juce_build -DJUCE_BUILD_EXAMPLES=On
cmake --build juce_build --target DSPModulePluginDemo_VST3

# Get arguments
dsp_plugin="./juce_build/examples/Plugins/DSPModulePluginDemo_artefacts/Debug/VST3/DSPModulePluginDemo.vst3"
plugalzer="./build/Plugalyzer_artefacts/Debug/Plugalyzer"
audio_sample=$(find test -maxdepth 1 -name "*.wav" -print -quit)
audio_output="${audio_sample%.*}-proc.wav"

mkdir -p test

echo ""
echo "------"
echo "Version"
"$plugalzer" -v

echo ""
echo "------"
echo "Help"
"$plugalzer" -h

echo ""
echo "------"
echo "Help: listParameters"
"$plugalzer" listParameters -h

echo ""
echo "------"
echo "Help: generateAutomation"
"$plugalzer" generateAutomation -h

echo ""
echo "------"
echo "Help: busLayouts"
"$plugalzer" busLayouts -h

echo ""
echo "------"
echo "Help: process"
"$plugalzer" process -h

echo ""
echo "------"
echo "Help: audioDiff"
"$plugalzer" audioDiff -h

echo ""
echo "------"
echo "List parameters: default, stdout"
"$plugalzer" \
  listParameters \
  -p "$dsp_plugin"

echo ""
echo "------"
echo "List parameters: text, stdout"
"$plugalzer" \
  listParameters \
  -p "$dsp_plugin" \
  -f text

echo ""
echo "------"
echo "List parameters: json, stdout"
"$plugalzer" \
  listParameters \
  -p "$dsp_plugin" \
  -f json

echo ""
echo "------"
echo "List parameters: default, file"
"$plugalzer" \
  listParameters \
  -p "$dsp_plugin" \
  -o ./test/listParameters.txt \
  -y

echo ""
echo "------"
echo "List parameters: text, file"
"$plugalzer" \
  listParameters \
  -p "$dsp_plugin" \
  -o ./test/listParameters.txt \
  -f text \
  -y

echo ""
echo "------"
echo "List parameters: json, file"
"$plugalzer" \
  listParameters \
  -p "$dsp_plugin" \
  -o ./test/listParameters.json \
  -f json \
  -y

echo ""
echo "------"
echo "Generate Automation: stdout"
"$plugalzer" \
  generateAutomation \
  -p "$dsp_plugin" 

echo ""
echo "------"
echo "Generate Automation: file"
"$plugalzer" \
  generateAutomation \
  -p "$dsp_plugin" \
  -o ./test/generateAutomation.json \
  -y

echo ""
echo "------"
echo "Bus layouts: default, stdout"
"$plugalzer" \
  busLayouts \
  -p "$dsp_plugin"

echo ""
echo "------"
echo "Bus layouts: text, stdout"
"$plugalzer" \
  busLayouts \
  -p "$dsp_plugin" \
  -f text

echo ""
echo "------"
echo "Bus layouts: json, stdout"
"$plugalzer" \
  busLayouts \
  -p "$dsp_plugin" \
  -f json

echo ""
echo "------"
echo "Bus layouts: default, file"
"$plugalzer" \
  busLayouts \
  -p "$dsp_plugin" \
  -o ./test/busLayouts.txt \
  -y

echo ""
echo "------"
echo "Bus layouts: text, file"
"$plugalzer" \
  busLayouts \
  -p "$dsp_plugin" \
  -o ./test/busLayouts.txt \
  -f text \
  -y

echo ""
echo "------"
echo "Bus layouts: json, file"
"$plugalzer" \
  busLayouts \
  -p "$dsp_plugin" \
  -o ./test/busLayouts.json \
  -f json \
  -y

echo ""
echo "------"
echo "Process"
"$plugalzer" \
  process \
  --plugin="$dsp_plugin" \
  --input="$audio_sample" \
  --output="$audio_output" \
  --paramFile=./test/generateAutomation.json \
  -y

echo ""
echo "------"
echo "Audiodiff: fail"
"$plugalzer" \
  audioDiff \
  -t "$audio_output" \
  -r "$audio_sample"

echo ""
echo "------"
echo "Audiodiff: succeed"
"$plugalzer" \
  audioDiff \
  -t "$audio_output" \
  -r "$audio_output"

