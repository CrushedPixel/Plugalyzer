#!/bin/bash

set -euo pipefail

# Build APH
cmake -S JUCE -B juce_build -DJUCE_BUILD_EXTRAS=ON -DCMAKE_CXX_FLAGS="-DJUCE_LOG_ASSERTIONS=1"
cmake --build juce_build --target=AudioPluginHost

# Build Plugalyzer
cmake -S . -B build
cmake --build build

# Build Plugalyzee
cmake -S test/PlugalyzeeAudio -B test/PlugalyzeeAudio/build
cmake --build test/PlugalyzeeAudio/build


# Get arguments
plugalyzee="test/PlugalyzeeAudio/build/PlugalyzeeAudio_artefacts/Debug/LV2/PlugalyzeeAudio.lv2"
plugalyzee="test/PlugalyzeeAudio/build/PlugalyzeeAudio_artefacts/Debug/VST3/PlugalyzeeAudio.vst3"
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
echo "Help: state"
"$plugalzer" state -h

echo ""
echo "------"
echo "List parameters: default, stdout"
"$plugalzer" \
  listParameters \
  -p "$plugalyzee"

echo ""
echo "------"
echo "List parameters: text, stdout"
"$plugalzer" \
  listParameters \
  -p "$plugalyzee" \
  -f text

echo ""
echo "------"
echo "List parameters: json, stdout"
"$plugalzer" \
  listParameters \
  -p "$plugalyzee" \
  -f json

echo ""
echo "------"
echo "List parameters: default, file"
"$plugalzer" \
  listParameters \
  -p "$plugalyzee" \
  -o ./test/output/listParameters.txt \
  -y

echo ""
echo "------"
echo "List parameters: text, file"
"$plugalzer" \
  listParameters \
  -p "$plugalyzee" \
  -o ./test/output/listParameters.txt \
  -f text \
  -y

echo ""
echo "------"
echo "List parameters: json, file"
"$plugalzer" \
  listParameters \
  -p "$plugalyzee" \
  -o ./test/output/listParameters.json \
  -f json \
  -y

echo ""
echo "------"
echo "Generate Automation: stdout"
"$plugalzer" \
  generateAutomation \
  -p "$plugalyzee"

echo ""
echo "------"
echo "Generate Automation: file"
"$plugalzer" \
  generateAutomation \
  -p "$plugalyzee" \
  -o ./test/output/generateAutomation.json \
  -y

echo ""
echo "------"
echo "Bus layouts: default, stdout"
"$plugalzer" \
  busLayouts \
  -p "$plugalyzee"

echo ""
echo "------"
echo "Bus layouts: text, stdout"
"$plugalzer" \
  busLayouts \
  -p "$plugalyzee" \
  -f text

echo ""
echo "------"
echo "Bus layouts: json, stdout"
"$plugalzer" \
  busLayouts \
  -p "$plugalyzee" \
  -f json

echo ""
echo "------"
echo "Bus layouts: default, file"
"$plugalzer" \
  busLayouts \
  -p "$plugalyzee" \
  -o ./test/output/busLayouts.txt \
  -y

echo ""
echo "------"
echo "Bus layouts: text, file"
"$plugalzer" \
  busLayouts \
  -p "$plugalyzee" \
  -o ./test/output/busLayouts.txt \
  -f text \
  -y

echo ""
echo "------"
echo "Bus layouts: json, file"
"$plugalzer" \
  busLayouts \
  -p "$plugalyzee" \
  -o ./test/output/busLayouts.json \
  -f json \
  -y

echo ""
echo "------"
echo "Process"
"$plugalzer" \
  process \
  --plugin="$plugalyzee" \
  --input="$audio_sample" \
  --output="$audio_output" \
  --paramFile=./test/output/preset1.json \
  -y

echo ""
echo "------"
echo "Audiodiff: succeed"
"$plugalzer" \
  audioDiff \
  -t "$audio_output" \
  -r "$audio_output"

echo ""
echo "------"
echo "Audiodiff: fail"
"$plugalzer" \
  audioDiff \
  -t "$audio_output" \
  -r "$audio_sample" \
  || true

echo ""
echo "------"
echo "Save default state as binary"
"$plugalzer" \
  state \
  -p "$plugalyzee" \
  -o "./test/output/default-state.bin" \
  -y

echo ""
echo "------"
echo "Save default state as xml"
"$plugalzer" \
  state \
  -p "$plugalyzee" \
  -o "./test/output/default-state.xml" \
  -f "xml" \
  -y

echo ""
echo "------"
echo "State as binary -> JSON params"
"$plugalzer" \
  state \
  -p "$plugalyzee" \
  -i "./test/output/default-state.bin" \
  -o "./test/output/default-state.json" \
  -y

echo ""
echo "------"
echo "JSON params -> state as binary"
"$plugalzer" \
  state \
  -p "$plugalyzee" \
  -i "./test/output/preset1.json" \
  -o "./test/output/state-preset1.bin" \
  -y


echo ""
echo "------"
echo "JSON params -> state as XML"
"$plugalzer" \
  state \
  -p "$plugalyzee" \
  -i "./test/output/preset1.json" \
  -o "./test/output/state-preset1.xml" \
  -f "xml" \
  -y


echo ""
echo "------"
echo "Diffing default state and preset state"
diff "./test/output/default-state.bin" "./test/output/state-preset1.bin"

rm "./test/output/default-state.bin"
rm "./test/output/default-state.xml"
rm "./test/output/state-preset1.bin"

