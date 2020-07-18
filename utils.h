/*
  ==============================================================================
 utils.h - Utility functions
  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

namespace jux
{
static bool addDefaultColourIdIfNotSet (int colourId, juce::Colour defaultColourToSet)
{
    auto& def = juce::LookAndFeel::getDefaultLookAndFeel();
    if (! def.isColourSpecified (colourId))
    {
        def.setColour (colourId, defaultColourToSet);
        return true;
    }
    return false;
}

} // namespace jux
