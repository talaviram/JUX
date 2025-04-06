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

#include "MenuItem.h"

using namespace juce;

namespace jux
{

MenuItem::MenuItem() = default;
MenuItem::MenuItem (String t) : text (std::move (t)), itemID (-1) {}

MenuItem::MenuItem (MenuItem&&) = default;
MenuItem& MenuItem::operator= (MenuItem&&) = default;

MenuItem::MenuItem (const MenuItem& other)
    : text (other.text),
      itemID (other.itemID),
      action (other.action),
      parentItem (other.parentItem),
      subMenu (createCopyIfNotNull (other.subMenu.get())),
      image (other.image != nullptr ? other.image->createCopy() : nullptr),
      customComponent (other.customComponent),
      customCallback (other.customCallback),
      commandManager (other.commandManager),
      shortcutKeyDescription (other.shortcutKeyDescription),
      colour (other.colour),
      isEnabled (other.isEnabled),
      isTicked (other.isTicked),
      isSeparator (other.isSeparator),
      isSectionHeader (other.isSectionHeader)
{
}

MenuItem& MenuItem::operator= (const MenuItem& other)
{
    text = other.text;
    itemID = other.itemID;
    action = other.action;
    parentItem = other.parentItem;
    subMenu.reset (createCopyIfNotNull (other.subMenu.get()));
    image = other.image != nullptr ? other.image->createCopy() : std::unique_ptr<Drawable>();
    customComponent = other.customComponent;
    customCallback = other.customCallback;
    commandManager = other.commandManager;
    shortcutKeyDescription = other.shortcutKeyDescription;
    colour = other.colour;
    isEnabled = other.isEnabled;
    isTicked = other.isTicked;
    isSeparator = other.isSeparator;
    isSectionHeader = other.isSectionHeader;
    return *this;
}

MenuItem& MenuItem::setTicked (bool shouldBeTicked) & noexcept
{
    isTicked = shouldBeTicked;
    return *this;
}

MenuItem& MenuItem::setEnabled (bool shouldBeEnabled) & noexcept
{
    isEnabled = shouldBeEnabled;
    return *this;
}

MenuItem& MenuItem::setAction (std::function<void()> newAction) & noexcept
{
    action = std::move (newAction);
    return *this;
}

MenuItem& MenuItem::setID (int newID) & noexcept
{
    itemID = newID;
    return *this;
}

MenuItem& MenuItem::setColour (Colour newColour) & noexcept
{
    colour = newColour;
    return *this;
}

MenuItem& MenuItem::setCustomComponent (ReferenceCountedObjectPtr<PopupMenu::CustomComponent> comp) & noexcept
{
    customComponent = comp;
    return *this;
}

MenuItem& MenuItem::setImage (std::unique_ptr<Drawable> newImage) & noexcept
{
    image = std::move (newImage);
    return *this;
}

MenuItem&& MenuItem::setTicked (bool shouldBeTicked) && noexcept
{
    isTicked = shouldBeTicked;
    return std::move (*this);
}

MenuItem&& MenuItem::setEnabled (bool shouldBeEnabled) && noexcept
{
    isEnabled = shouldBeEnabled;
    return std::move (*this);
}

MenuItem&& MenuItem::setAction (std::function<void()> newAction) && noexcept
{
    action = std::move (newAction);
    return std::move (*this);
}

MenuItem&& MenuItem::setID (int newID) && noexcept
{
    itemID = newID;
    return std::move (*this);
}

MenuItem&& MenuItem::setColour (Colour newColour) && noexcept
{
    colour = newColour;
    return std::move (*this);
}

MenuItem&& MenuItem::setCustomComponent (ReferenceCountedObjectPtr<PopupMenu::CustomComponent> comp) && noexcept
{
    customComponent = comp;
    return std::move (*this);
}

MenuItem&& MenuItem::setImage (std::unique_ptr<Drawable> newImage) && noexcept
{
    image = std::move (newImage);
    return std::move (*this);
}

std::unique_ptr<MenuItem> MenuItem::convertPopupItem (const PopupMenu::Item& other, MenuItem* parent)
{
    auto dest = std::make_unique<MenuItem>();
    dest->text = other.text;
    dest->itemID = other.itemID;
    dest->action = other.action;
    if (other.subMenu)
        dest->subMenu = convertPopupMenuToList (*other.subMenu, dest.get());

    dest->parentItem = parent;
    dest->image = other.image != nullptr ? other.image->createCopy() : nullptr;
    dest->customComponent = other.customComponent;
    dest->customCallback = other.customCallback;
    dest->commandManager = other.commandManager;
    dest->shortcutKeyDescription = other.shortcutKeyDescription;
    dest->colour = other.colour;
    dest->isEnabled = other.isEnabled;
    dest->isTicked = other.isTicked;
    dest->isSeparator = other.isSeparator;
    dest->isSectionHeader = other.isSectionHeader;
    return dest;
}

std::unique_ptr<MenuItem::List> MenuItem::convertPopupMenuToList (const PopupMenu& source, MenuItem* parent)
{
    auto popupAsList = std::make_unique<List>();
    PopupMenu::MenuItemIterator it (source);
    while (it.next())
    {
        const auto& other = it.getItem();
        popupAsList->push_back (*convertPopupItem (other, parent));
    }
    return popupAsList;
}

} // namespace jux