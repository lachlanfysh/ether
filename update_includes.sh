#!/bin/bash

echo "Updating #include paths for directory reorganization..."

# Update includes for modulation -> control/modulation
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]modulation/|#include "control/modulation/|g'
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]../modulation/|#include "../control/modulation/|g'

# Update includes for velocity -> control/velocity  
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]velocity/|#include "control/velocity/|g'
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]../velocity/|#include "../control/velocity/|g'

# Update includes for ui -> interface/ui
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]ui/|#include "interface/ui/|g'
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]../ui/|#include "../interface/ui/|g'

# Update includes for effects -> processing/effects
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]effects/|#include "processing/effects/|g'
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]../effects/|#include "../processing/effects/|g'

# Update includes for hardware -> platform/hardware
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]hardware/|#include "platform/hardware/|g'
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]../hardware/|#include "../platform/hardware/|g'

# Update includes for sampler -> storage/sampling
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]sampler/|#include "storage/sampling/|g'
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]../sampler/|#include "../storage/sampling/|g'

# Update includes for presets -> storage/presets
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]presets/|#include "storage/presets/|g'
find src -name "*.h" -o -name "*.cpp" | xargs sed -i '' 's|#include.*["<]../presets/|#include "../storage/presets/|g'

# Update root level test includes
find . -maxdepth 1 -name "test_*.cpp" | xargs sed -i '' 's|#include "modulation/|#include "src/control/modulation/|g'
find . -maxdepth 1 -name "test_*.cpp" | xargs sed -i '' 's|#include "ui/|#include "src/interface/ui/|g'
find . -maxdepth 1 -name "test_*.cpp" | xargs sed -i '' 's|#include "effects/|#include "src/processing/effects/|g'
find . -maxdepth 1 -name "test_*.cpp" | xargs sed -i '' 's|#include "velocity/|#include "src/control/velocity/|g'
find . -maxdepth 1 -name "test_*.cpp" | xargs sed -i '' 's|#include "sampler/|#include "src/storage/sampling/|g'
find . -maxdepth 1 -name "test_*.cpp" | xargs sed -i '' 's|#include "sequencer/|#include "src/sequencer/|g'
find . -maxdepth 1 -name "test_*.cpp" | xargs sed -i '' 's|#include "audio/|#include "src/audio/|g'

echo "Include path updates completed!"