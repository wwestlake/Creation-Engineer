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

float parseDegrees(const juce::String& text, float fallbackDegrees)
{
    const auto trimmed = text.trim();
    if (trimmed.isEmpty())
        return fallbackDegrees;

    // getFloatValue() itself already returns 0 for genuinely unparseable
    // text (no leading digit at all) rather than signalling failure, so a
    // deliberately-typed "0" and an accidental empty-after-trim both need
    // the same fallback -- checked above -- but anything with at least a
    // leading digit/sign is trusted to getFloatValue() directly, same as
    // every plain-float editor elsewhere in this app before unit-aware
    // parsing existed.
    return trimmed.getFloatValue();
}

juce::String formatDegrees(float degrees)
{
    return juce::String(degrees, 2);
}
}
