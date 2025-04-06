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

#include <juce_gui_basics/juce_gui_basics.h>

namespace jux
{

//==============================================================================
/**
    MenuItem represents a selectable item in a menu.
    It can be used by various menu components and is interchangable with juce::PopupMenu::Item.
*/
class MenuItem
{
public:
    typedef std::vector<MenuItem> List;

    MenuItem();
    MenuItem (juce::String text);

    MenuItem (const MenuItem&);
    MenuItem& operator= (const MenuItem&);
    MenuItem (MenuItem&&);
    MenuItem& operator= (MenuItem&&);

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
    MenuItem* parentItem = nullptr;

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
    MenuItem& setTicked (bool shouldBeTicked = true) & noexcept;
    /** Sets the isEnabled flag (and returns a reference to this item to allow chaining). */
    MenuItem& setEnabled (bool shouldBeEnabled) & noexcept;
    /** Sets the action property (and returns a reference to this item to allow chaining). */
    MenuItem& setAction (std::function<void()> action) & noexcept;
    /** Sets the itemID property (and returns a reference to this item to allow chaining). */
    MenuItem& setID (int newID) & noexcept;
    /** Sets the colour property (and returns a reference to this item to allow chaining). */
    MenuItem& setColour (juce::Colour) & noexcept;
    /** Sets the customComponent property (and returns a reference to this item to allow chaining). */
    MenuItem& setCustomComponent (juce::ReferenceCountedObjectPtr<juce::PopupMenu::CustomComponent> customComponent) & noexcept;
    /** Sets the image property (and returns a reference to this item to allow chaining). */
    MenuItem& setImage (std::unique_ptr<juce::Drawable>) & noexcept;

    /** Sets the isTicked flag (and returns a reference to this item to allow chaining). */
    MenuItem&& setTicked (bool shouldBeTicked = true) && noexcept;
    /** Sets the isEnabled flag (and returns a reference to this item to allow chaining). */
    MenuItem&& setEnabled (bool shouldBeEnabled) && noexcept;
    /** Sets the action property (and returns a reference to this item to allow chaining). */
    MenuItem&& setAction (std::function<void()> action) && noexcept;
    /** Sets the itemID property (and returns a reference to this item to allow chaining). */
    MenuItem&& setID (int newID) && noexcept;
    /** Sets the colour property (and returns a reference to this item to allow chaining). */
    MenuItem&& setColour (juce::Colour) && noexcept;
    /** Sets the customComponent property (and returns a reference to this item to allow chaining). */
    MenuItem&& setCustomComponent (juce::ReferenceCountedObjectPtr<juce::PopupMenu::CustomComponent> customComponent) && noexcept;
    /** Sets the image property (and returns a reference to this item to allow chaining). */
    MenuItem&& setImage (std::unique_ptr<juce::Drawable>) && noexcept;

    static std::unique_ptr<MenuItem::List> convertPopupMenuToList (const juce::PopupMenu& source, MenuItem* parent = nullptr);
    static std::unique_ptr<MenuItem> convertPopupItem (const juce::PopupMenu::Item&, MenuItem* parent = nullptr);
};

} // namespace jux