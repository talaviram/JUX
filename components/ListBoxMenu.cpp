/*
  ==============================================================================

    MIT License

    Copyright (c) 2020 Tal Aviram

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

#include "ListBoxMenu.h"

using namespace juce;

namespace jux
{
typedef std::vector<ListBoxMenu::Item> List;

Path ListBoxMenu::getArrowPath (Rectangle<float> arrowZone, const int direction, bool filled, const Justification justification)
{
    auto w = jmin (arrowZone.getWidth(), (direction == 0 || direction == 2) ? 8.0f : filled ? 5.0f : 8.0f);
    auto h = jmin (arrowZone.getHeight(), (direction == 0 || direction == 2) ? 5.0f : filled ? 8.0f : 5.0f);

    if (justification == Justification::centred)
    {
        arrowZone.reduce ((arrowZone.getWidth() - w) / 2, (arrowZone.getHeight() - h) / 2);
    }
    else if (justification == Justification::centredRight)
    {
        arrowZone.removeFromLeft (arrowZone.getWidth() - w);
        arrowZone.reduce (0, (arrowZone.getHeight() - h) / 2);
    }
    else if (justification == Justification::centredLeft)
    {
        arrowZone.removeFromRight (arrowZone.getWidth() - w);
        arrowZone.reduce (0, (arrowZone.getHeight() - h) / 2);
    }
    else
    {
        jassertfalse; // currently only supports centred justifications
    }

    Path path;
    path.startNewSubPath (arrowZone.getX(), arrowZone.getBottom());
    path.lineTo (arrowZone.getCentreX(), arrowZone.getY());
    path.lineTo (arrowZone.getRight(), arrowZone.getBottom());

    if (filled)
        path.closeSubPath();

    path.applyTransform (AffineTransform::rotation (direction * MathConstants<float>::halfPi,
                                                    arrowZone.getCentreX(),
                                                    arrowZone.getCentreY()));

    return path;
}

ListBoxMenu::Item::Item() = default;
ListBoxMenu::Item::Item (String t) : text (std::move (t)), itemID (-1) {}

ListBoxMenu::Item::Item (Item&&) = default;
ListBoxMenu::Item& ListBoxMenu::Item::operator= (Item&&) = default;

ListBoxMenu::Item::Item (const Item& other)
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

ListBoxMenu::Item& ListBoxMenu::Item::operator= (const Item& other)
{
    text = other.text;
    itemID = other.itemID;
    action = other.action;
    parentItem = other.parentItem;
    subMenu.reset (createCopyIfNotNull (other.subMenu.get()));
    image = other.image != nullptr ? other.image->createCopy() : std::unique_ptr<Drawable>();
    jassert (other.customComponent == nullptr);
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

ListBoxMenu::Item& ListBoxMenu::Item::setTicked (bool shouldBeTicked) & noexcept
{
    isTicked = shouldBeTicked;
    return *this;
}

ListBoxMenu::Item& ListBoxMenu::Item::setEnabled (bool shouldBeEnabled) & noexcept
{
    isEnabled = shouldBeEnabled;
    return *this;
}

ListBoxMenu::Item& ListBoxMenu::Item::setAction (std::function<void()> newAction) & noexcept
{
    action = std::move (newAction);
    return *this;
}

ListBoxMenu::Item& ListBoxMenu::Item::setID (int newID) & noexcept
{
    itemID = newID;
    return *this;
}

ListBoxMenu::Item& ListBoxMenu::Item::setColour (Colour newColour) & noexcept
{
    colour = newColour;
    return *this;
}

ListBoxMenu::Item& ListBoxMenu::Item::setCustomComponent (ReferenceCountedObjectPtr<PopupMenu::CustomComponent> comp) & noexcept
{
    customComponent = comp;
    return *this;
}

ListBoxMenu::Item& ListBoxMenu::Item::setImage (std::unique_ptr<Drawable> newImage) & noexcept
{
    image = std::move (newImage);
    return *this;
}

ListBoxMenu::Item&& ListBoxMenu::Item::setTicked (bool shouldBeTicked) && noexcept
{
    isTicked = shouldBeTicked;
    return std::move (*this);
}

ListBoxMenu::Item&& ListBoxMenu::Item::setEnabled (bool shouldBeEnabled) && noexcept
{
    isEnabled = shouldBeEnabled;
    return std::move (*this);
}

ListBoxMenu::Item&& ListBoxMenu::Item::setAction (std::function<void()> newAction) && noexcept
{
    action = std::move (newAction);
    return std::move (*this);
}

ListBoxMenu::Item&& ListBoxMenu::Item::setID (int newID) && noexcept
{
    itemID = newID;
    return std::move (*this);
}

ListBoxMenu::Item&& ListBoxMenu::Item::setColour (Colour newColour) && noexcept
{
    colour = newColour;
    return std::move (*this);
}

ListBoxMenu::Item&& ListBoxMenu::Item::setCustomComponent (ReferenceCountedObjectPtr<PopupMenu::CustomComponent> comp) && noexcept
{
    customComponent = comp;
    return std::move (*this);
}

ListBoxMenu::Item&& ListBoxMenu::Item::setImage (std::unique_ptr<Drawable> newImage) && noexcept
{
    image = std::move (newImage);
    return std::move (*this);
}

ListBoxMenu::ListBoxMenu() : lastSelectedRow (-1), currentRoot (nullptr)
{
    addDefaultColourIdIfNotSet (ColourIds::backgroundColour, Colours::darkgrey);
    addDefaultColourIdIfNotSet (ColourIds::headerBackgroundColour, Colours::black.withAlpha (0.7f));
    addDefaultColourIdIfNotSet (ListBox::ColourIds::backgroundColourId, findColour (ColourIds::backgroundColour));
    list.setMouseMoveSelectsRows (false);
    list.setModel (this);
    list.setHeaderComponent (std::unique_ptr<Component> (new ListBoxMenu::ListMenuToolbar()));
    setRowHeight (30);
    addAndMakeVisible (list);
    getToolbar()->backButton.onClick = [&]() { backToParent(); };
}

void ListBoxMenu::addSelectionListener (juce::Value::Listener* listener)
{
    selectedId.addListener (listener);
}

void ListBoxMenu::removeSelectionListener (juce::Value::Listener* listener)
{
    selectedId.removeListener (listener);
}

void ListBoxMenu::resized()
{
    auto bounds = getLocalBounds();
    list.setBounds (bounds);
}

void ListBoxMenu::mouseDown (const juce::MouseEvent& e)
{
    if (e.getMouseDownY() > list.getDefaultRowHeight()) // header
    {
        mouseDownRow = list.getRowContainingPosition (e.getPosition().getX(), e.getPosition().getY());
        list.repaintRow (mouseDownRow);
    }
    if (auto cmp = list.getComponentAt (e.getPosition()))
        cmp->mouseDown (e);
}

void ListBoxMenu::mouseUp (const juce::MouseEvent& e)
{
    if (e.getMouseDownY() > list.getDefaultRowHeight()) // header
    {
        mouseDownRow = -1;
        list.repaintRow (list.getRowContainingPosition (e.getMouseDownX(), e.getMouseDownY()));
    }
    if (auto cmp = list.getComponentAt (e.getMouseDownPosition()))
        cmp->mouseUp (e);
}

int ListBoxMenu::getRowHeight (const int rowNumber) const
{
    if (currentRoot != nullptr)
    {
        if (rowNumber >= currentRoot->subMenu->size())
        {
            jassertfalse;
            return list.getDefaultRowHeight();
        }
        auto root = currentRoot->subMenu->at (rowNumber);
        if (root.customComponent)
        {
            int width = 0;
            int height = 0;
            root.customComponent->getIdealSize (width, height);
            return height;
        }
    }

    return list.getDefaultRowHeight();
}

void ListBoxMenu::setRowHeight (const int newSize)
{
    getToolbar()->setBounds (getToolbar()->getBounds().withHeight (newSize));
    list.setDefaultRowHeight (newSize);
}

int ListBoxMenu::getNumRows()
{
    int val = currentRoot && currentRoot->subMenu ? static_cast<int> (currentRoot->subMenu->size()) : 0;
    return val;
}

void ListBoxMenu::setShouldCloseOnItemClick (const bool shouldClose)
{
    shouldCloseOnItemClick = shouldClose;
}

void ListBoxMenu::setBackButtonShowText (bool showText)
{
    getToolbar()->backButton.setIsNameVisible (showText);
    resized();
}

void ListBoxMenu::animateAndClose (const bool removeComponent)
{
    if (getParentComponent())
    {
        auto* cob = findParentComponentOfClass<CallOutBox>();
        auto& animator = Desktop::getInstance().getAnimator();
        auto bounds = getBounds().translated (-getWidth(), 0);
        animator.animateComponent (this, bounds, 1.0f, 300, true, 0.0, 0.0);
        if (removeComponent)
            getParentComponent()->removeChildComponent (this);
        else
            setVisible (false);
        if (cob != nullptr)
            cob->dismiss();
    }
}

void ListBoxMenu::paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    // we use custom component to capture mouse events
}

Component* ListBoxMenu::refreshComponentForRow (int rowNumber, bool isRowSelected, Component* existingComponentToUpdate)
{
    if (existingComponentToUpdate)
        delete existingComponentToUpdate;
    if (currentRoot && currentRoot->subMenu)
    {
        auto& item = (*currentRoot->subMenu)[rowNumber];
        if (item.customComponent.get())
            return new CustomComponentWrapper (item.customComponent.get());

        auto c = std::make_unique<RowComponent>();
        c->rowNumber = rowNumber;
        c->isRowSelected = isRowSelected;
        c->parent = this;
        return c.release();
    }
    return nullptr;
}

void ListBoxMenu::listBoxItemClicked (int row, const MouseEvent&)
{
    auto* item = &(*currentRoot->subMenu)[row];
    if (item->subMenu != nullptr)
    {
        setCurrentRoot (item);
    }
    else
    {
        if (item->isSectionHeader || item->isSeparator)
        {
            list.selectRow (lastSelectedRow);
            return;
        }
        lastSelectedRow = row;
        selectedId.setValue (item->itemID);
        if (item->action)
            item->action();
        list.repaint();

        if (shouldCloseOnItemClick)
            animateAndClose();
    }
}

void ListBoxMenu::deleteKeyPressed (int)
{
    backToParent();
}

void ListBoxMenu::setMenuFromPopup (juce::PopupMenu& menu, const juce::String rootMenuName)
{
    rootMenu.reset (new Item());
    rootMenu->text = rootMenuName;
    rootMenu->subMenu = convertPopupMenuToList (menu, rootMenu.get());
    setCurrentRoot (rootMenu.get(), true, false);
    list.updateContent();
    list.setDefaultRowHeight (list.getDefaultRowHeight());
}

void ListBoxMenu::setOnRootBackToParent (std::function<void()> func)
{
    onRootBack = func;
    if (currentRoot)
        setCurrentRoot (currentRoot, false, false);
}

void ListBoxMenu::setHideHeaderOnParent (const bool shouldHide)
{
    shouldHideHeaderOnRoot = shouldHide;
    getToolbar()->setBounds (getToolbar()->getBounds().withHeight (rootMenu.get() != nullptr && rootMenu->parentItem == nullptr && shouldHideHeaderOnRoot ? 0 : list.getDefaultRowHeight()));
    list.resized();
}

bool ListBoxMenu::backToParent()
{
    if (isCurrentRootHasParent())
    {
        setCurrentRoot (currentRoot->parentItem);
        return true;
    }
    else if (onRootBack)
    {
        onRootBack();
        return true;
    }
    return false;
}

void ListBoxMenu::setCurrentRoot (Item* newRoot, const bool shouldAnimate, const bool shouldCache)
{
    lastSelectedRow = -1;
    transitionBackground.reset();
    getToolbar()->setBounds (getToolbar()->getBounds().withHeight (newRoot != nullptr && newRoot->parentItem == nullptr && shouldHideHeaderOnRoot ? 0 : list.getDefaultRowHeight()));
    if (isVisible() && shouldAnimate)
    {
        if (shouldCache)
        {
            transitionBackground = std::make_unique<ImageComponent>();
            transitionBackground->setImage (list.createComponentSnapshot (list.getLocalBounds()));
            transitionBackground->setBounds (getLocalBounds());
            addAndMakeVisible (transitionBackground.get());
        }
        getToolbar()->backButton.setVisible (false);
        auto& animator = Desktop::getInstance().getAnimator();
        animator.addChangeListener (this);
        auto finalBounds = list.getLocalBounds();
        list.setBounds (finalBounds.translated (getWidth(), 0));
        if (shouldCache)
        {
            auto curFinalBounds = list.getBounds().translated (-getWidth(), 0);
            list.toFront (false);
            animator.animateComponent (transitionBackground.get(), curFinalBounds, 1.0, 300, false, 0.3, 0.0);
        }
        animator.animateComponent (&list, finalBounds, 1.0, 300, false, 0.3, 0.0);
    }
    list.deselectAllRows(); // this must happen before we change the list structure!
    currentRoot = newRoot;
    getToolbar()->backButton.setText (currentRoot->parentItem != nullptr ? currentRoot->parentItem->text : String());
    getToolbar()->title.setText (currentRoot->text, dontSendNotification);

    if (! shouldAnimate)
    {
        getToolbar()->backButton.setEnabled (onRootBack != nullptr || isCurrentRootHasParent());
        getToolbar()->backButton.setVisible (onRootBack != nullptr || isCurrentRootHasParent());
    }
    list.updateContent();
}

bool ListBoxMenu::isCurrentRootHasParent() const
{
    return currentRoot && currentRoot->parentItem;
}

void ListBoxMenu::changeListenerCallback (ChangeBroadcaster* src)
{
    auto& animator = Desktop::getInstance().getAnimator();
    if (src == &animator && ! animator.isAnimating (&list))
    {
        getToolbar()->backButton.setEnabled (onRootBack != nullptr || isCurrentRootHasParent());
        getToolbar()->backButton.setVisible (onRootBack != nullptr || isCurrentRootHasParent());
        transitionBackground.reset();
        Desktop::getInstance().getAnimator().removeChangeListener (this);
        repaint();
    }
}

std::unique_ptr<ListBoxMenu::Item> ListBoxMenu::convertPopupItem (const PopupMenu::Item& other, ListBoxMenu::Item* parent)
{
    auto dest = std::make_unique<ListBoxMenu::Item>();
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

std::unique_ptr<List> ListBoxMenu::convertPopupMenuToList (const PopupMenu& source, ListBoxMenu::Item* parent)
{
    auto list = std::make_unique<List>();
    PopupMenu::MenuItemIterator it (source);
    while (it.next())
    {
        const auto& other = it.getItem();
        list->push_back (*convertPopupItem (other, parent));
    }
    return list;
}

ListBoxMenu::ListMenuToolbar* ListBoxMenu::getToolbar()
{
    return static_cast<ListBoxMenu::ListMenuToolbar*> (list.getHeaderComponent());
}

ListBoxMenu::ListMenuToolbar::ListMenuToolbar()
{
    title.setJustificationType (Justification::centred);
    addAndMakeVisible (backButton);
    addAndMakeVisible (title);
    title.setInterceptsMouseClicks (false, false);
}

void ListBoxMenu::ListMenuToolbar::paint (Graphics& g)
{
    g.fillAll (findColour (ColourIds::headerBackgroundColour));
}

void ListBoxMenu::ListMenuToolbar::resized()
{
    auto bounds = getLocalBounds();
    title.setBounds (bounds);
    auto backTextArea = backButton.getIsNameVisible() ? 120 : 0;
    backButton.setBounds (bounds.removeFromLeft (bounds.getHeight() + backTextArea));
    bounds.removeFromRight (bounds.getHeight());
}

ListBoxMenu::BackButton::BackButton() : Button ("")
{
    getArrowPath = ListBoxMenu::getArrowPath;
    auto& def = LookAndFeel::getDefaultLookAndFeel();
    if (! def.isColourSpecified (ColourIds::arrowColour))
        def.setColour (ColourIds::arrowColour, Colours::white);
    if (! def.isColourSpecified (ColourIds::arrowColourOver))
        def.setColour (ColourIds::arrowColourOver, Colours::grey);
    if (! def.isColourSpecified (ColourIds::textColour))
        def.setColour (ColourIds::textColour, Colours::white);
    if (! def.isColourSpecified (ColourIds::textColourOver))
        def.setColour (ColourIds::textColourOver, Colours::white.darker());
}

void ListBoxMenu::BackButton::setText (String newText)
{
    text = newText;
}
void ListBoxMenu::BackButton::setIsNameVisible (bool isVisible)
{
    isNameVisible = isVisible;
}
bool ListBoxMenu::BackButton::getIsNameVisible() const
{
    return isNameVisible;
}
void ListBoxMenu::BackButton::paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds();
    auto arrow = bounds.removeFromLeft (bounds.getHeight()).toFloat();

    Colour arrowColour = findColour (shouldDrawButtonAsHighlighted ? ColourIds::arrowColourOver : ColourIds::arrowColour);
    arrowColour = isEnabled() ? arrowColour : arrowColour.darker();
    g.setColour (arrowColour);
    g.strokePath (getArrowPath (arrow, 3, false, Justification::centred), PathStrokeType (1.0f));
    if (text.isNotEmpty())
    {
        Font font (bounds.getHeight() - 4);
        if (font.getStringWidth (text) < bounds.getWidth())
        {
            g.setColour (findColour (shouldDrawButtonAsHighlighted ? ColourIds::textColourOver : ColourIds::textColour));
            g.drawFittedText (text, bounds, Justification::centredLeft, 1);
        }
    }
}

void ListBoxMenu::RowComponent::paint (juce::Graphics& g)
{
    auto& item = (*parent->currentRoot->subMenu)[rowNumber];
    if (item.customComponent == nullptr)
    {
        if (item.isSectionHeader)
            getLookAndFeel().drawPopupMenuSectionHeader (g, getLocalBounds(), item.text);
        else
            getLookAndFeel().drawPopupMenuItem (g, { 0, 0, getWidth(), getHeight() }, item.isSeparator, item.isEnabled, (isRowSelected || isDown) && item.isEnabled, item.isTicked, item.subMenu != nullptr, item.text, item.shortcutKeyDescription, item.image.get(), item.colour.isTransparent() ? nullptr : &item.colour);
    }
}

void ListBoxMenu::RowComponent::mouseDown (const juce::MouseEvent&)
{
    isDown = true;
    repaint();
}
void ListBoxMenu::RowComponent::mouseUp (const juce::MouseEvent& e)
{
    isDown = false;
    repaint();
    if (contains (e.getPosition()))
        parent->listBoxItemClicked (rowNumber, e);
}
} // namespace jux
