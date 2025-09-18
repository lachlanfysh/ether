#!/bin/bash

# 🎵 Celebration Tune - A sweet melodic loop for your girlfriend! 🎵
# This creates a gentle, melodic pattern using different engines

echo "🎉 Playing a celebration tune! 🎶"

# Main melodic pattern with MacroVA (warm and sweet)
(echo -e "volume 0.4
bpm 110
step 1 8
step 3 10
step 5 12
step 7 8
step 9 10
step 11 7
step 13 8
step 15 5
engine 0
play
sleep 8
stop") | ./test_step_sequencer &

sleep 2

# Bass line with our newly working SlideAccentBass! 
(echo -e "volume 0.3
bpm 110
step 1 0
step 5 3
step 9 0
step 13 2
engine 14
play
sleep 8
stop") | ./test_step_sequencer &

sleep 2

# High melodic counter-melody with FormantVocal (dreamy)
(echo -e "volume 0.25
bpm 110
step 2 12
step 6 14
step 10 15
step 14 12
engine 6
play
sleep 8
stop") | ./test_step_sequencer &

echo "🎶 Three-part harmony celebrating the bass engine! 🎶"
echo "🎤 Main melody (MacroVA) + Bass (SlideAccentBass) + High melody (FormantVocal)"
echo "💕 A sweet tune for your girlfriend! 💕"

# Wait for all parts to finish
wait

echo "🎉 Celebration complete! The bass engine is ALIVE! 🎉"