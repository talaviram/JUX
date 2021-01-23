/*
  ==============================================================================
 utils.h - Utility functions
  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

namespace jux
{
[[maybe_unused]] static juce::Path getArrowPath (juce::Rectangle<float> arrowZone, const int direction, bool filled, const juce::Justification justification)
{
    auto w = std::min<float> (arrowZone.getWidth(), (direction == 0 || direction == 2) ? 8.0f : filled ? 5.0f : 8.0f);
    auto h = std::min<float> (arrowZone.getHeight(), (direction == 0 || direction == 2) ? 5.0f : filled ? 8.0f : 5.0f);

    if (justification == juce::Justification::centred)
    {
        arrowZone.reduce ((arrowZone.getWidth() - w) / 2, (arrowZone.getHeight() - h) / 2);
    }
    else if (justification == juce::Justification::centredRight)
    {
        arrowZone.removeFromLeft (arrowZone.getWidth() - w);
        arrowZone.reduce (0, (arrowZone.getHeight() - h) / 2);
    }
    else if (justification == juce::Justification::centredLeft)
    {
        arrowZone.removeFromRight (arrowZone.getWidth() - w);
        arrowZone.reduce (0, (arrowZone.getHeight() - h) / 2);
    }
    else
    {
        jassertfalse; // currently only supports centred justifications
    }

    juce::Path path;
    path.startNewSubPath (arrowZone.getX(), arrowZone.getBottom());
    path.lineTo (arrowZone.getCentreX(), arrowZone.getY());
    path.lineTo (arrowZone.getRight(), arrowZone.getBottom());

    if (filled)
        path.closeSubPath();

    path.applyTransform (juce::AffineTransform::rotation (direction * juce::MathConstants<float>::halfPi,
                                                          arrowZone.getCentreX(),
                                                          arrowZone.getCentreY()));

    return path;
}

[[maybe_unused]] static bool addDefaultColourIdIfNotSet (int colourId, juce::Colour defaultColourToSet)
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
