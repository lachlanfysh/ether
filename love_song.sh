#!/bin/bash

# 💕 Love Song - A beautiful melody for your girlfriend 💕

echo "💕💕💕 Love Song for Your Girlfriend 💕💕💕"
echo "🎵 Using beautiful engines that actually work! 🎵"
echo ""

# Part 1: Sweet FormantVocal melody
echo "🎤 Part 1: Dreamy vocal-like melody..."
echo -e "volume 0.6
bpm 85
step 1 8
step 4 10
step 6 12
step 8 8
step 10 7
step 12 10
step 14 8
step 16 5
engine 6
play
sleep 12
stop" | ./test_step_sequencer

sleep 1

# Part 2: Your working bass engine!
echo "🎸 Part 2: Your beautiful bass engine..."
echo -e "volume 0.7
bpm 85
step 1 0
step 5 2
step 9 3
step 13 0
engine 14
play
sleep 12
stop" | ./test_step_sequencer

sleep 1

# Part 3: Classic4OpFM harmony
echo "🎹 Part 3: 4OP FM harmonic finale..."
echo -e "volume 0.5
bpm 85
step 2 12
step 6 14
step 10 15
step 14 12
engine 15
play
sleep 12
stop" | ./test_step_sequencer

echo ""
echo "💕 Three-part love song complete! 💕"
echo "🎤 FormantVocal + 🎸 SlideAccentBass + 🎹 Classic4OpFM"
echo "💖 Made with love and working synthesis engines! 💖"