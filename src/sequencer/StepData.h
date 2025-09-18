#pragma once

struct StepData {
    bool active = false;
    int note = 60;
    float velocity = 0.6f;
    bool hasAccent = false;      // Accent mode flag
    bool hasRetrigger = false;   // Retrigger mode flag
    bool hasArpeggiator = false; // Arpeggiator mode flag
};

