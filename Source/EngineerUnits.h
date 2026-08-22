#pragma once

#include <JuceHeader.h>

// Small, focused unit-parsing helpers so dimension fields across Engineer's
// panels accept real-world units instead of raw, unlabeled numbers. Bare
// numbers are meters (the app's default unit); mm/cm/in are converted.
// Internal storage and display stay in meters everywhere -- only *input*
// is unit-flexible, per the drawing/layers/vertex-editing plan's Part L.
namespace EngineerUnits
{
// Parses a length typed by the user: a leading number (optionally signed,
// with a decimal point) followed by an optional case-insensitive unit
// suffix -- "150mm", "25 mm", "6in", "6\"", "0.2m", or a bare "0.2" (meters).
// Returns fallbackMeters for empty or unparseable text, matching the
// "don't crash on bad text" contract juce::TextEditor::getFloatValue()
// callers already relied on implicitly before this existed.
float parseLengthMeters(const juce::String& text, float fallbackMeters);

// Always formats in meters, 3 decimals -- the display convention every
// dimension field in this app already used before unit-aware input existed;
// this just centralizes it in one place.
juce::String formatLengthMeters(float meters);
}
