#!/bin/bash

# 💕 Romantic Looping Tune - A gentle, sweet melody 💕

echo "💕 Playing a romantic tune for your girlfriend! 💕"
echo "🎵 Using FormantVocal engine - sounds dreamy and sweet! 🎵"

# Create a sweet melodic pattern that loops
echo -e "volume 0.5
bpm 95
step 1 8
step 3 10  
step 5 12
step 7 8
step 9 10
step 11 7
step 13 8
step 15 5
engine 6
play
sleep 10
stop
quit" | ./test_step_sequencer

echo "💕 Hope she loved the dreamy FormantVocal melody! 💕"