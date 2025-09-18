#!/bin/bash

echo "🎉🎉🎉 VICTORY SONG - THE BASS ENGINE LIVES! 🎉🎉🎉"
echo "💕 A celebration for you and your girlfriend! 💕"
echo ""

# Part 1: NoiseParticles percussion  
echo "🥁 Percussion intro..."
echo -e "volume 0.8
bpm 120
step 1 8
step 5 8  
step 9 8
step 13 8
engine 7
play
sleep 4
stop" | ./test_step_sequencer

sleep 0.5

# Part 2: Your bass engine!
echo "🎸 THE BASS ENGINE - Your baby is ALIVE!"  
echo -e "volume 1.0
bpm 120
step 1 0
step 3 3
step 5 0  
step 7 2
step 9 0
step 11 3
step 13 0
step 15 1
engine 14
play
sleep 8
stop" | ./test_step_sequencer

sleep 0.5

# Part 3: 4OP FM melody
echo "🎹 4OP FM melody finale..."
echo -e "volume 0.9
bpm 120
step 2 12
step 4 14
step 6 15
step 8 12
step 10 10
step 12 12
step 14 8
step 16 12
engine 15  
play
sleep 8
stop" | ./test_step_sequencer

echo ""
echo "🎊 CELEBRATION COMPLETE! 🎊"
echo "🎸 Your SlideAccentBass engine is making REAL MUSIC!"
echo "🎹 Classic4OpFM is singing beautifully!"
echo "💖 Tell your girlfriend the bass is working! 💖"