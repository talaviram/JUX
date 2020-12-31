/*
  ==============================================================================

    MIT License

    Copyright (c) 2020-2021 Tal Aviram

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

  ==============================================================================
*/

#include "TabBar.h"
#include "../utils.h"

namespace jux
{
TabBar::TabBar()
    : leftArrow (viewport, 3),
      rightArrow (viewport, 1)
{
    if (! getLookAndFeel().isColourSpecified (highlightColourId))
        getLookAndFeel().setColour (ColourIds::highlightColourId, juce::Colours::white);
    addAndMakeVisible (viewport);
    viewport.setViewedComponent (tabHolder = new Component());
    viewport.setScrollBarsShown (false, false, false, true);
    addAndMakeVisible (leftArrow);
    addAndMakeVisible (rightArrow);
    addAndMakeVisible (addButton);

    addButton.setButtonText ("+");
    viewport.setSingleStepSizes (50, 0);
    addButton.onClick = [=]() {
        onAddTabClicked();
    };
}

void TabBar::setSelectedTab (int tabId)
{
    selectedTab.setValue (juce::jlimit (0, tabs.size() - 1, tabId));
}

void TabBar::addTab (juce::String tabName, int tabId)
{
    tabId = tabId == -1 ? tabs.size() : tabId;
    DBG ("Adding -> " << juce::String (tabId));
    auto* l = new TabButton (*this, tabId, tabName);
    tabs.add (l);
    tabHolder->addAndMakeVisible (l);
    resized();
    // select tab
    selectedTab.setValue (juce::jmin (tabId, tabs.size() - 1));
    DBG ("Tabs size: " << juce::String (tabs.size()));
}

void TabBar::removeTab (int tabId)
{
    tabHolder->removeChildComponent (tabs[tabId]);
    tabs.remove (tabId);
    if (tabId < tabs.size())
    {
        // update indexs
        for (auto idx = tabId; idx < tabs.size(); idx++)
        {
            tabs[idx]->setTabId (idx);
        }
    }
    resized();
    if (tabs.size() > 0)
    {
        // select next tab
        selectedTab.setValue (juce::jmin (tabId, tabs.size() - 1));
    }
    else
    {
        selectedTab.setValue (-1);
    }
    onTabSelected (selectedTab.getValue());
}

void TabBar::resized()
{
    const int switchButtonSize = juce::roundToInt (getHeight() / 2);
    constexpr auto maxTabSize = 200;
    constexpr auto minTabSize = 80;
    const auto tabCalc = juce::roundToInt ((1.0 * getWidth() - switchButtonSize * 3) / std::min<int> (1, tabs.size()));
    const auto tabSize = juce::jlimit (minTabSize, maxTabSize, tabCalc);
    auto viewportBounds = getLocalBounds();
    tabHolder->setBounds (viewportBounds.withWidth (tabSize * tabs.size()));
    for (auto i = 0; i < tabs.size(); i++)
    {
        tabs[i]->setBounds (tabSize * i, 0, tabSize, tabHolder->getHeight());
    }
    addButton.setBounds (viewportBounds.removeFromRight (getHeight()));
    leftArrow.setBounds (viewportBounds.removeFromLeft (switchButtonSize));
    rightArrow.setBounds (viewportBounds.removeFromRight (switchButtonSize));
    viewport.setBounds (viewportBounds);
}

void TabBar::clearTabs()
{
    tabHolder->removeAllChildren();
    tabs.clear();
    resized();
}

void TabBar::ScrollButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    g.fillAll (shouldDrawButtonAsHighlighted ? juce::Colours::grey : juce::Colours::white.withAlpha (0.2f));
    g.setColour (juce::Colours::white);
    auto bounds = juce::Rectangle<float> (0, 0, getWidth(), getHeight());
    auto path = jux::getArrowPath (bounds, orientation, false, juce::Justification::centred);
    g.strokePath (path, juce::PathStrokeType (1.5f));
}

} // namespace jux
