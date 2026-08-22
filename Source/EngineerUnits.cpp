#include "EngineerUnits.h"

namespace EngineerUnits
{
float parseLengthMeters(const juce::String& text, float fallbackMeters)
{
    const auto trimmed = text.trim();
    if (trimmed.isEmpty())
        return fallbackMeters;

    int index = 0;
    const auto length = trimmed.length();
    if (index < length && (trimmed[index] == '+' || trimmed[index] == '-'))
        ++index;

    bool sawDigitOrDot = false;
    while (index < length && (juce::CharacterFunctions::isDigit(trimmed[index]) || trimmed[index] == '.'))
    {
        sawDigitOrDot = true;
        ++index;
    }

    if (!sawDigitOrDot)
        return fallbackMeters;

    const auto numericPart = trimmed.substring(0, index);
    const auto suffix = trimmed.substring(index).trim().toLowerCase();
    const auto value = numericPart.getFloatValue();

    if (suffix.isEmpty() || suffix == "m")
        return value;
    if (suffix == "mm")
        return value / 1000.0f;
    if (suffix == "cm")
        return value / 100.0f;
    if (suffix == "in" || suffix == "\"")
        return value * 0.0254f;

    // Unrecognized suffix -- treat the numeric part as bare meters rather
    // than silently discarding what the user typed.
    return value;
}

juce::String formatLengthMeters(float meters)
{
    return juce::String(meters, 3);
}
}
