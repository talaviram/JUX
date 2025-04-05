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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

const auto closeButtonSize = 30;

namespace jux
{
//==============================================================================
/**

    Component to show 'tabs' more similar to browsers.
    Unlike the JUCE one. the bar isn't managing anything for you.
    You have to implement your desired behavior.

*/
class TabBar : public juce::Component
{
public:
    enum ColourIds
    {
        highlightColourId = 0xF001700, /**< A colour to use to highlight tab. */
    };

    TabBar();

    Component& getAddButton() { return addButton; }

    std::function<void()> onAddTabClicked;
    std::function<void (int)> onTabClosed;
    std::function<void (int)> onTabMoved;
    std::function<void (int)> onTabSelected;

    void setSelectedTab (int tabId);
    void addTab (juce::String tabName, int tabId = -1);
    void removeTab (int tabId);
    void clearTabs();

    const juce::Value& getSelectedTab() const { return selectedTab; }
    juce::Viewport& getViewport() { return viewport; }
    int getNumOfTabs() const { return tabs.size(); }

    void resized() override;

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawLine (0, getBottom(), getWidth(), getBottom());
    }

private:
    struct CloseButton : public juce::Button
    {
        CloseButton() : Button ("CloseButton") {}
        void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            const auto b = getLocalBounds();
            g.setColour (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted ? juce::Colours::darkgrey : juce::Colours::transparentWhite);
            const auto circleRadius = std::min<int> (b.getWidth(), b.getHeight()) * 0.45f;

            auto rect = juce::Rectangle<int> (
                b.getCentreX() - circleRadius,
                b.getCentreY() - circleRadius,
                circleRadius * 2,
                circleRadius * 2);

            g.fillEllipse (rect.toFloat());
            g.setColour (shouldDrawButtonAsHighlighted ? juce::Colours::white : juce::Colours::black.withAlpha (0.5f));
            const int reduce = 6;
            g.drawLine (rect.getX() + reduce, rect.getY() + reduce, rect.getRight() - reduce, rect.getBottom() - reduce);
            g.drawLine (rect.getRight() - reduce, rect.getY() + reduce, rect.getX() + reduce, rect.getBottom() - reduce);
        }
    };

    class TabButton : public juce::Component, private juce::Value::Listener
    {
    public:
        TabButton (TabBar& owner, int tabIdToSet, juce::String tabName)
            : owner (owner)
        {
            selectedTab.setValue (owner.getSelectedTab().getValue());
            selectedTab.referTo (owner.getSelectedTab());
            tabId = tabIdToSet;
            label.setMinimumHorizontalScale (1.0f);
            label.setText (tabName, juce::dontSendNotification);
            addAndMakeVisible (label);
            addChildComponent (closeButton);
            closeButton.onClick = [&]() {
                jassert (owner.onTabClosed);
                owner.onTabClosed (tabId);
            };
            selectedTab.addListener (this);
            label.setColour (juce::Label::ColourIds::backgroundColourId, juce::Colours::transparentWhite);
            label.setInterceptsMouseClicks (false, false);
            valueChanged (selectedTab);
        }

        int getTabId() { return tabId; }

        void setTabId (int newTabId)
        {
            tabId = newTabId;
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            jassert (owner.onTabSelected);
            selectedTab.setValue (tabId);
            owner.onTabSelected (tabId);
        }

        void paint (juce::Graphics& g) override
        {
            g.setColour (juce::Colours::darkgrey);
            g.fillRect (getLocalBounds().getX(), 0, getWidth(), 1);
            g.fillRect (getLocalBounds().getRight() - 1, 0, 1, getHeight());
            if (isSelected())
            {
                g.fillAll (juce::Colours::grey);
                g.setColour (findColour (ColourIds::highlightColourId));
                g.fillRect (0, 0, getWidth(), 2);
            }
        }

        void resized() override
        {
            closeButton.setBounds (getLocalBounds().removeFromRight (closeButtonSize).reduced (4));
            label.setBounds (getLocalBounds().withWidth (getWidth() - closeButton.getWidth()));
        }

    private:
        bool isSelected() { return (int) selectedTab.getValue() == tabId; }

        void valueChanged (juce::Value& value) override
        {
            if (isSelected())
            {
                const auto visibleArea = owner.getViewport().getViewArea();
                // move to show tab if not fully shown
                int delta = visibleArea.getX();
                if (getX() < visibleArea.getX())
                {
                    delta = getX();
                }
                else if (visibleArea.getRight() < getRight())
                {
                    delta = visibleArea.getX() + (getRight() - visibleArea.getRight());
                }
                owner.getViewport().setViewPosition (delta, 0);
            }
            closeButton.setVisible (isSelected());
            repaint();
        }

        juce::Label label;
        CloseButton closeButton;
        int tabId;
        juce::Value selectedTab;
        TabBar& owner;
    };

    class ScrollButton : public juce::Button
    {
    public:
        ScrollButton (juce::Viewport& vp, int orientation) : Button ("ScrollButton"), viewport (vp), orientation (orientation)
        {
        }
        void mouseDown (const juce::MouseEvent& e) override
        {
            viewport.beginDragAutoRepeat (50);
        }
        void mouseUp (const juce::MouseEvent& e) override
        {
            viewport.beginDragAutoRepeat (0);
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            const auto relPos = e.getEventRelativeTo (&viewport);
            viewport.autoScroll (relPos.getMouseDownX(), relPos.getMouseDownY(), 10, 10);
        }
        void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    private:
        juce::Viewport& viewport;
        int orientation;
    } leftArrow, rightArrow;

    juce::Component* tabHolder;
    juce::OwnedArray<TabButton> tabs;
    juce::Value selectedTab;
    juce::Viewport viewport;
    juce::TextButton addButton, listTabs;
};
} // namespace jux
