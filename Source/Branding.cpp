#include "Branding.h"
#include <creation/ui/CreationSuiteLogos.h>

namespace branding
{
juce::Image CreateLogoImage(int size)
{
    auto source = creation::ui::getSuiteLogoImage(creation::ui::SuiteLogoId::engineer);
    if (! source.isValid())
        return {};

    return source.rescaled(size, size, juce::Graphics::highResamplingQuality);
}
}

namespace creation_engineer::branding
{
juce::Colour backgroundColour() noexcept { return juce::Colour(0xff0d1118); }
juce::Colour panelColour() noexcept { return juce::Colour(0xff182230); }
juce::Colour accentColour() noexcept { return juce::Colour(0xff5ec8ff); }
}

