#pragma once

#include <JuceHeader.h>

namespace branding
{
juce::Image CreateLogoImage(int size);
}

namespace creation_engineer::branding
{
juce::Colour backgroundColour() noexcept;
juce::Colour panelColour() noexcept;
juce::Colour accentColour() noexcept;
}

