#!/bin/bash

# Collaborative Work Monitor Script
# Provides timing structure for work batches

BATCH_SIZE=3
WORK_PERIOD=1800    # 30 minutes for Claude to complete 3 tasks
SLEEP_DURATION=3600 # 1 hour sleep between batches

echo "🔧 Collaborative Work Monitor Started"
echo "📋 Batch size: $BATCH_SIZE tasks"
echo "⚙️ Work period: $WORK_PERIOD seconds (30 minutes)"
echo "⏰ Sleep duration: $SLEEP_DURATION seconds (1 hour)"
echo "🕐 Started at: $(date)"
echo ""

# Create log file for tracking
LOG_FILE="/Users/lachlanfysh/Desktop/ether/work_log.txt"
echo "=== Work Session Started at $(date) ===" >> "$LOG_FILE"

batch_number=1

while true; do
    echo "📦 BATCH $batch_number - Work period starting"
    echo "🏁 Batch $batch_number work period started at $(date)" >> "$LOG_FILE"
    
    echo "⏳ Claude has $WORK_PERIOD seconds to complete $BATCH_SIZE tasks..."
    echo "🔧 Work period: $(date) (30 minutes)"
    
    # Give Claude time to work (30 minutes should be plenty for 3 tasks)
    sleep $WORK_PERIOD
    
    echo "✅ Work period for batch $batch_number complete at $(date)"
    echo "✅ Batch $batch_number work period ended at $(date)" >> "$LOG_FILE"
    
    echo ""
    echo "😴 Starting 1-hour sleep period..."
    echo "🕐 Sleep started at: $(date)"
    echo "🕐 Will wake up in 1 hour at: $(date -v +1H)"
    echo "😴 Sleep period $batch_number started at $(date)" >> "$LOG_FILE"
    
    # Sleep for 1 hour
    sleep $SLEEP_DURATION
    
    echo ""
    echo "⏰ Sleep period complete!"
    echo "🕐 Woke up at: $(date)"
    echo "⏰ Sleep period $batch_number completed at $(date)" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    batch_number=$((batch_number + 1))
    echo ""
done