#!/bin/bash

echo "🚀 Importing downloaded models into Ollama..."

# Check if downloads exist
DOWNLOADS_DIR="$HOME/Downloads"
CODER_FILE="$DOWNLOADS_DIR/qwen2.5-coder-7b-instruct-q4_k_m.gguf"
GENERAL_FILE="$DOWNLOADS_DIR/qwen2.5-32b-instruct-q4_k_m.gguf"

# Import Qwen2.5-Coder 7B
if [ -f "$CODER_FILE" ]; then
    echo "📁 Found Qwen2.5-Coder 7B, importing..."
    
    # Create Modelfile for coder
    cat > /tmp/Modelfile.coder << EOF
FROM $CODER_FILE
TEMPLATE "{{- if .System }}<|im_start|>system
{{ .System }}<|im_end|>
{{- end }}<|im_start|>user
{{ .Prompt }}<|im_end|>
<|im_start|>assistant
"
PARAMETER stop <|im_start|>
PARAMETER stop <|im_end|>
EOF
    
    ollama create qwen2.5-coder:7b -f /tmp/Modelfile.coder
    echo "✅ Qwen2.5-Coder 7B imported!"
else
    echo "❌ Qwen2.5-Coder 7B not found at: $CODER_FILE"
fi

# Import Qwen2.5 32B
if [ -f "$GENERAL_FILE" ]; then
    echo "📁 Found Qwen2.5 32B, importing..."
    
    # Create Modelfile for general
    cat > /tmp/Modelfile.general << EOF
FROM $GENERAL_FILE
TEMPLATE "{{- if .System }}<|im_start|>system
{{ .System }}<|im_end|>
{{- end }}<|im_start|>user
{{ .Prompt }}<|im_end|>
<|im_start|>assistant
"
PARAMETER stop <|im_start|>
PARAMETER stop <|im_end|>
EOF
    
    ollama create qwen2.5:32b -f /tmp/Modelfile.general  
    echo "✅ Qwen2.5 32B imported!"
else
    echo "❌ Qwen2.5 32B not found at: $GENERAL_FILE"
fi

echo "🎉 Model import complete! Available models:"
ollama list