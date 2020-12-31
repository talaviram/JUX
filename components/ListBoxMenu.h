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

#include "../utils.h"
#include "ListBox.h"

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
    struct Item;

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
    void setSsecondaryClickAction (std::function<void (Item&)> func);

    void setHideHeaderOnParent (bool shouldHide);
    void setShouldShowHeader (bool isVisible);

    bool isCurrentRootHasParent() const;

    void setRowHeight (int newSize);
    int getRowHeight (int rowNumber) const override;

    /** When set to true, close and remove the ListBoxMenu component.
         */
    void setShouldCloseOnItemClick (bool shouldClose);
    void setBackButtonShowText (bool showText);

    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForRow (int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void deleteKeyPressed (int lastRowSelected) override;

    juce::Rectangle<int> getSelectedBounds() const;

    struct Item
    {
        typedef std::vector<Item> List;
        /** Creates a null item.
             You'll need to set some fields after creating an Item before you
             can add it to a PopupMenu
         */
        Item();

        /** Creates an item with the given text.
             This constructor also initialises the itemID to -1, which makes it suitable for
             creating lambda-based item actions.
         */
        Item (juce::String text);

        Item (const Item&);
        Item& operator= (const Item&);
        Item (Item&&);
        Item& operator= (Item&&);

        /** The menu item's name. */
        juce::String text;

        /** The menu item's ID.
             This must not be 0 if you want the item to be triggerable, but if you're attaching
             an action callback to the item, you can set the itemID to -1 to indicate that it
             isn't actively needed.
         */
        int itemID = 0;

        /** An optional function which should be invoked when this menu item is triggered. */
        std::function<void()> action;

        /** A parent-item, or nullptr if there isn't one. */
        Item* parentItem = nullptr;

        /** A sub-menu, or nullptr if there isn't one. */
        std::unique_ptr<List> subMenu;

        /** A drawable to use as an icon, or nullptr if there isn't one. */
        std::unique_ptr<juce::Drawable> image;

        /** A custom component for the item to display, or nullptr if there isn't one. */
        juce::ReferenceCountedObjectPtr<juce::PopupMenu::CustomComponent> customComponent = nullptr;

        /** A custom callback for the item to use, or nullptr if there isn't one. */
        juce::ReferenceCountedObjectPtr<juce::PopupMenu::CustomCallback> customCallback = nullptr;

        /** A command manager to use to automatically invoke the command, or nullptr if none is specified. */
        juce::ApplicationCommandManager* commandManager = nullptr;

        /** An optional string describing the shortcut key for this item.
             This is only used for displaying at the right-hand edge of a menu item - the
             menu won't attempt to actually catch or process the key. If you supply a
             commandManager parameter then the menu will attempt to fill-in this field
             automatically.
         */
        juce::String shortcutKeyDescription;

        /** A colour to use to draw the menu text.
             By default this is transparent black, which means that the LookAndFeel should choose the colour.
         */
        juce::Colour colour;

        /** True if this menu item is enabled. */
        bool isEnabled = true;

        /** True if this menu item should have a tick mark next to it. */
        bool isTicked = false;

        /** True if this menu item is a separator line. */
        bool isSeparator = false;

        /** True if this menu item is a section header. */
        bool isSectionHeader = false;

        /** Sets the isTicked flag (and returns a reference to this item to allow chaining). */
        Item& setTicked (bool shouldBeTicked = true) & noexcept;
        /** Sets the isEnabled flag (and returns a reference to this item to allow chaining). */
        Item& setEnabled (bool shouldBeEnabled) & noexcept;
        /** Sets the action property (and returns a reference to this item to allow chaining). */
        Item& setAction (std::function<void()> action) & noexcept;
        /** Sets the itemID property (and returns a reference to this item to allow chaining). */
        Item& setID (int newID) & noexcept;
        /** Sets the colour property (and returns a reference to this item to allow chaining). */
        Item& setColour (juce::Colour) & noexcept;
        /** Sets the customComponent property (and returns a reference to this item to allow chaining). */
        Item& setCustomComponent (juce::ReferenceCountedObjectPtr<juce::PopupMenu::CustomComponent> customComponent) & noexcept;
        /** Sets the image property (and returns a reference to this item to allow chaining). */
        Item& setImage (std::unique_ptr<juce::Drawable>) & noexcept;

        /** Sets the isTicked flag (and returns a reference to this item to allow chaining). */
        Item&& setTicked (bool shouldBeTicked = true) && noexcept;
        /** Sets the isEnabled flag (and returns a reference to this item to allow chaining). */
        Item&& setEnabled (bool shouldBeEnabled) && noexcept;
        /** Sets the action property (and returns a reference to this item to allow chaining). */
        Item&& setAction (std::function<void()> action) && noexcept;
        /** Sets the itemID property (and returns a reference to this item to allow chaining). */
        Item&& setID (int newID) && noexcept;
        /** Sets the colour property (and returns a reference to this item to allow chaining). */
        Item&& setColour (juce::Colour) && noexcept;
        /** Sets the customComponent property (and returns a reference to this item to allow chaining). */
        Item&& setCustomComponent (juce::ReferenceCountedObjectPtr<juce::PopupMenu::CustomComponent> customComponent) && noexcept;
        /** Sets the image property (and returns a reference to this item to allow chaining). */
        Item&& setImage (std::unique_ptr<juce::Drawable>) && noexcept;
    };

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
    int lastSelectedRow { -1 };

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

        bool isSecondaryClick (const juce::MouseEvent&) const;

        ListBoxMenu* parent;
        int rowNumber;
        bool isRowSelected = false, isDown = false, isSecondary = false, isDragging = false;
        bool selectRowOnMouseUp;

        ListBoxMenu& owner;
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
        ~CustomComponentWrapper()
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
    };

    std::unique_ptr<Item::List> convertPopupMenuToList (const juce::PopupMenu& source, ListBoxMenu::Item* parent = nullptr);
    std::unique_ptr<ListBoxMenu::Item> convertPopupItem (const juce::PopupMenu::Item&, Item* parent = nullptr);

    std::unique_ptr<Item> rootMenu;
    std::function<void()> onRootBack {};
    std::function<void (Item&)> onSecondaryClick {};
    Item* currentRoot;
    jux::ListBox list;
    bool shouldCloseOnItemClick { false };
    bool shouldHideHeaderOnRoot { false };
    bool shouldShowHeader { true };

    // simplify this component animation
    std::unique_ptr<juce::ImageComponent> transitionBackground;
};
} // namespace jux
