/*
  ==============================================================================

    MIT License

    Copyright (c) 2020-2025 Tal Aviram

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

#include "../utils.h"
#include "ListBox.h"
#include "MenuItem.h"

namespace jux
{

//==============================================================================
/**

    Component to show list of items for interaction.
    It can be used for making navigational menus common on mobile (iOS/Android)  devices.
    The list supports interchangable structure with juce::PopupMenu::Item.
    This allows having a Popup menu on desktop devices and navigational flow on mobile.


    @see juce::PopupMenu

*/
class ListBoxMenu : public juce::Component, private jux::ListBoxModel, private juce::ChangeListener
{
public:
    using Item = jux::MenuItem;

    ListBoxMenu();

    ListBoxMenu (std::unique_ptr<Item> rootItemToOwn);

    enum ColourIds
    {
        backgroundColour = 0x1B05000,
        headerBackgroundColour = 0x1B05001
    };

    void addSelectionListener (juce::Value::Listener* listener);
    void removeSelectionListener (juce::Value::Listener* listener);

    void resized() override;

    void setMenu (std::unique_ptr<Item>);
    void setMenuFromPopup (juce::PopupMenu&& menu, const juce::String rootMenuName = juce::String());
    bool backToParent();

    // optional function if needed on back of root item.
    void setOnRootBackToParent (std::function<void()> func);

    // Allows setting a right-click (desktop) or long-press (mobile) action.
    void setSecondaryClickAction (std::function<void (Item&)> func);

    void setHideHeaderOnParent (bool shouldHide);
    void setShouldShowHeader (bool isVisible);

    bool isCurrentRootHasParent() const;

    void setRowHeight (int newSize);
    int getRowHeight (int rowNumber) const override;

    /** When set to true, close and remove the ListBoxMenu component.
        @param onMenuClosed - allows additional callback on close (useful when menu is within another object that is affected by ListBoxMenu visibility.
         */
    void setShouldCloseOnItemClick (bool shouldClose, std::function<void()> onMenuClosed = nullptr);
    void setBackButtonShowText (bool showText);

    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForRow (int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void listBoxItemClicked (int row, bool isSecondaryClick);
    void deleteKeyPressed (int lastRowSelected) override;

    juce::Rectangle<int> getSelectedBounds() const;

    /** The current root item.
        This allows updating states like changing isTicked/menu like interfaces.
    */
    Item* const getCurrentRootItem() { return currentRoot; }

    /** Animate component to the left and remove if from screen.
            @param removeComponent - if set to true, removes child. else just set it visibility to false.
         */
    void animateAndClose (bool removeComponent = true);

private:
    friend class RowComponent;
    friend class RowAccessibilityHandler;
    int lastSelectedRow { -1 };

    juce::Component* getCustomComponentIfValid (int rowNumber);
    juce::Value selectedId;
    void updateSelectedId (int newSelection);
    void setCurrentRoot (Item* newRoot, bool shouldAnimate = true, bool shouldCache = true);
    void invokeItemEventsIfNeeded (Item&);
    bool shouldShowHeaderForItem (Item* rootItem);

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    class BackButton : public juce::Button
    {
    public:
        enum ColourIds
        {
            arrowColour = 0x1A05000,
            arrowColourOver = 0x1A05001,
            textColour = 0x1A05002,
            textColourOver = 0x1A05003
        };
        BackButton();
        void setText (juce::String text);
        void setIsNameVisible (bool isVisible);
        bool getIsNameVisible() const;
        void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    private:
        bool isNameVisible { true };
        juce::String text { "" };
    };

    struct ListMenuToolbar : public juce::Component
    {
    public:
        ListMenuToolbar();
        void paint (juce::Graphics&) override;
        void resized() override;
        BackButton backButton;
        juce::Label title;
    };

    ListMenuToolbar* getToolbar();

    class RowComponent : public juce::Component
    {
    public:
        RowComponent (ListBoxMenu&);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

        bool isSecondaryClick (const juce::MouseEvent&) const;

        juce::AccessibilityActions getItemAccessibilityActions();

        ListBoxMenu* parent;
        int rowNumber;
        bool isRowSelected = false, isDown = false, isSecondary = false, isDragging = false;
        bool selectRowOnMouseUp;

        ListBoxMenu& owner;
        friend class RowAccessibilityHandler;
    };

    // workaround for ListBox custom component tricky lifecycle
    class CustomComponentWrapper : public Component
    {
        friend class ListBoxMenu;

    public:
        CustomComponentWrapper (Component* componentToWrap) : nonOwnedComponent (componentToWrap)
        {
            jassert (nonOwnedComponent != nullptr);
            addAndMakeVisible (nonOwnedComponent);
        }
        ~CustomComponentWrapper() override
        {
            removeChildComponent (nonOwnedComponent);
        }
        void resized() override
        {
            nonOwnedComponent->setBounds (getLocalBounds());
        }

    private:
        void updateComponent (Component* componentToUpdate)
        {
            jassert (componentToUpdate != nullptr);
            nonOwnedComponent = componentToUpdate;
            addAndMakeVisible (nonOwnedComponent);
            resized();
        }
        Component* nonOwnedComponent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomComponentWrapper)
    };

    std::unique_ptr<Item::List> convertPopupMenuToList (const juce::PopupMenu& source, ListBoxMenu::Item* parent = nullptr);
    std::unique_ptr<ListBoxMenu::Item> convertPopupItem (const juce::PopupMenu::Item&, Item* parent = nullptr);

    std::unique_ptr<Item> rootMenu;
    std::function<void()> onRootBack {};
    std::function<void (Item&)> onSecondaryClick {};
    std::function<void()> onMenuClose {};
    Item* currentRoot;
    jux::ListBox list;
    bool shouldCloseOnItemClick { false };
    bool shouldHideHeaderOnRoot { false };
    bool shouldShowHeader { true };

    // simplify this component animation
    std::unique_ptr<juce::ImageComponent> transitionBackground;
};
} // namespace jux
