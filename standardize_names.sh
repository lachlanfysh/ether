#!/bin/bash

echo "Standardizing class names to consistent conventions..."

# Engine class renamings (class names that need Engine suffix)
declare -A ENGINE_RENAMES=(
    ["MacroVA"]="MacroVAEngine"
    ["MacroFM"]="MacroFMEngine"
    ["Classic4OpFM"]="Classic4OpFMEngine"
    ["Noise"]="NoiseEngine"
    ["SerialHPLP"]="SerialHPLPEngine"
    ["SlideAccentBass"]="SlideAccentBassEngine"
    ["MacroChord"]="MacroChordEngine"
    ["MacroHarmonics"]="MacroHarmonicsEngine"
    ["MacroWaveshaper"]="MacroWaveshaperEngine"
    ["MacroWavetable"]="MacroWavetableEngine"
    ["RingsVoice"]="RingsVoiceEngine"
    ["ElementsVoice"]="ElementsVoiceEngine"
    ["TidesOsc"]="TidesOscEngine"
    ["Formant"]="FormantEngine"
)

# Function to rename class in files
rename_class() {
    local old_name="$1"
    local new_name="$2"
    
    echo "Renaming $old_name -> $new_name"
    
    # Find all source files and update class declarations
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/class $old_name\b/class $new_name/g"
    
    # Update class usage (constructors, destructors, etc.)
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/\b$old_name::/\b$new_name::/g"
    
    # Update variable declarations and instantiations
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/\b$old_name\s\+/\b$new_name /g"
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/\b$old_name\*/\b$new_name\*/g"
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/\b$old_name&/\b$new_name\&/g"
    
    # Update template parameters
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/<$old_name>/<$new_name>/g"
    
    # Update forward declarations
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/class $old_name;/class $new_name;/g"
    
    # Update inheritance
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/: public $old_name/: public $new_name/g"
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/: private $old_name/: private $new_name/g"
    
    # Update constructor/destructor calls
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/\b$old_name(/\b$new_name(/g"
    find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' "s/~$old_name(/~$new_name(/g"
}

# Rename all engines
for old_name in "${!ENGINE_RENAMES[@]}"; do
    new_name="${ENGINE_RENAMES[$old_name]}"
    rename_class "$old_name" "$new_name"
done

echo "Class name standardization completed!"

# Note: File renaming will be done separately to avoid breaking includes
echo "Note: File names will be updated in the next step"