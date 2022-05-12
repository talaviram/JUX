/*
  ==============================================================================

    MIT License

    Copyright (c) 2020-2022 Tal Aviram

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
#include "../utils.h"

using namespace juce;

namespace jux
{
typedef std::vector<ListBoxMenu::Item> List;

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
    jux::addDefaultColourIdIfNotSet (ColourIds::backgroundColour, Colours::darkgrey);
    jux::addDefaultColourIdIfNotSet (ColourIds::headerBackgroundColour, Colours::black.withAlpha (0.7f));
    jux::addDefaultColourIdIfNotSet (ListBox::ColourIds::backgroundColourId, findColour (ColourIds::backgroundColour));
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

void ListBoxMenu::setShouldCloseOnItemClick (const bool shouldClose, std::function<void()> onMenuClosed)
{
    shouldCloseOnItemClick = shouldClose;
    onMenuClose = onMenuClosed;
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
    if (onMenuClose)
        onMenuClose();
}

void ListBoxMenu::paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    // we use custom component to capture mouse events
}

juce::Component* ListBoxMenu::getCustomComponentIfValid (int rowNumber)
{
    if (currentRoot == nullptr || currentRoot->subMenu == nullptr)
        return nullptr;
    if (rowNumber >= currentRoot->subMenu->size())
        return nullptr;

    return (*currentRoot->subMenu)[rowNumber].customComponent.get();
}

Component* ListBoxMenu::refreshComponentForRow (int rowNumber, bool isRowSelected, Component* existingComponentToUpdate)
{
    juce::Component* customComponent = getCustomComponentIfValid (rowNumber);

    RowComponent* c = nullptr;
    if (existingComponentToUpdate == nullptr)
    {
        if (customComponent)
        {
            return new CustomComponentWrapper (customComponent);
        }
        c = new RowComponent (*this);
    }
    else
    {
        if (customComponent)
        {
            auto customWrapper = dynamic_cast<CustomComponentWrapper*> (existingComponentToUpdate);
            if (customWrapper != nullptr)
            {
                customWrapper->updateComponent (customComponent);
                return customWrapper;
            }
            else
            {
                return new CustomComponentWrapper (customComponent);
            }
        }
        else
        {
            c = dynamic_cast<RowComponent*> (existingComponentToUpdate);
            if (c == nullptr)
            {
                delete existingComponentToUpdate;
                c = new RowComponent (*this);
            }
        }
    }

    if (c != nullptr)
    {
        c->rowNumber = rowNumber;
        c->isRowSelected = isRowSelected;
        c->parent = this;
        return c;
    }

    if (existingComponentToUpdate != nullptr)
        jassertfalse;

    return existingComponentToUpdate;
}

void ListBoxMenu::invokeItemEventsIfNeeded (Item& item)
{
    if (! item.isEnabled)
        return;

    if (item.action != nullptr)
    {
        MessageManager::callAsync (item.action);
        return;
    }

    if (item.customCallback != nullptr)
        if (! item.customCallback->menuItemTriggered())
            return;

    if (item.commandManager != nullptr)
    {
        ApplicationCommandTarget::InvocationInfo info (item.itemID);
        info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;

        item.commandManager->invoke (info, true);
    }
}

void ListBoxMenu::listBoxItemClicked (int row, const MouseEvent& e)
{
    listBoxItemClicked (row, e.mods.isRightButtonDown());
}

void ListBoxMenu::listBoxItemClicked (int row, const bool isSecondaryClick)
{
    if (onSecondaryClick != nullptr && isSecondaryClick)
        return;

    auto* item = &(*currentRoot->subMenu)[row];
    if (item && ! item->isEnabled)
        return;

    if (item->subMenu != nullptr)
    {
        invokeItemEventsIfNeeded (*item);
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
        invokeItemEventsIfNeeded (*item);
        list.repaint();

        if (shouldCloseOnItemClick)
            animateAndClose();
    }
}

void ListBoxMenu::deleteKeyPressed (int)
{
    backToParent();
}

void ListBoxMenu::setMenu (std::unique_ptr<Item> menu)
{
    rootMenu = std::move (menu);
    setCurrentRoot (rootMenu.get(), true, false);
    list.updateContent();
    list.setDefaultRowHeight (list.getDefaultRowHeight());
}

void ListBoxMenu::setMenuFromPopup (juce::PopupMenu&& menu, const juce::String rootMenuName)
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

void ListBoxMenu::setSecondaryClickAction (std::function<void (Item&)> func)
{
    onSecondaryClick = func;
}

void ListBoxMenu::setHideHeaderOnParent (const bool shouldHide)
{
    shouldHideHeaderOnRoot = shouldHide;
    getToolbar()->setBounds (getToolbar()->getBounds().withHeight (shouldShowHeaderForItem (rootMenu.get()) ? list.getDefaultRowHeight() : 0));
    list.resized();
}

void ListBoxMenu::setShouldShowHeader (bool isVisible)
{
    shouldShowHeader = isVisible;
    getToolbar()->setBounds (getToolbar()->getBounds().withHeight (shouldShowHeaderForItem (rootMenu.get()) ? list.getDefaultRowHeight() : 0));
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
    getToolbar()->setBounds (getToolbar()->getBounds().withHeight (shouldShowHeaderForItem (newRoot) ? list.getDefaultRowHeight() : 0));
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

bool ListBoxMenu::shouldShowHeaderForItem (Item* rootItem)
{
    if (shouldShowHeader)
        return true;

    return rootItem != nullptr && rootItem->parentItem != nullptr && shouldHideHeaderOnRoot;
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
    setFocusContainerType (FocusContainerType::focusContainer);
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

ListBoxMenu::BackButton::BackButton() : Button ("Back Button")
{
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
    setTitle (getName() + (getIsNameVisible() ? newText : ""));
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
    g.strokePath (jux::getArrowPath (arrow, 3, false, Justification::centred), PathStrokeType (1.0f));
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

juce::Rectangle<int> ListBoxMenu::getSelectedBounds() const
{
    const auto row = lastSelectedRow;
    if (row == -1)
        return Rectangle<int>();

    return list.getRowPosition (row, true);
}

ListBoxMenu::RowComponent::RowComponent (ListBoxMenu& owner) : owner (owner)
{
}

juce::AccessibilityActions ListBoxMenu::RowComponent::getItemAccessibilityActions()
{
    auto& parent = owner;
    auto& list = owner.list;
    auto row = rowNumber;

    auto onFocus = [row, &list]
    {
        list.scrollToEnsureRowIsOnscreen (row);
        list.selectRow (row);
    };

    auto onPressOrToggle = [row, &parent, onFocus]
    {
        onFocus();
        parent.listBoxItemClicked (row, false);
    };

    return juce::AccessibilityActions().addAction (juce::AccessibilityActionType::focus, std::move (onFocus)).addAction (juce::AccessibilityActionType::press, onPressOrToggle).addAction (juce::AccessibilityActionType::toggle, onPressOrToggle);
}

class RowAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit RowAccessibilityHandler (ListBoxMenu::RowComponent& rowComponentToWrap)
        : juce::AccessibilityHandler (rowComponentToWrap,
                                      juce::AccessibilityRole::listItem,
                                      rowComponentToWrap.getItemAccessibilityActions(),
                                      { std::make_unique<RowCellInterface> (*this) }),
          rowComponent (rowComponentToWrap)
    {
    }

    juce::String getTitle() const override
    {
        if (auto* m = rowComponent.parent->currentRoot->subMenu.get())
        {
            return (*m)[rowComponent.rowNumber].text;
        }

        return {};
    }

    juce::AccessibleState getCurrentState() const override
    {
        if (auto* m = rowComponent.parent->list.getModel())
            if (rowComponent.rowNumber >= m->getNumRows())
                return juce::AccessibleState().withIgnored();

        auto state = AccessibilityHandler::getCurrentState().withAccessibleOffscreen();
        if (auto* m = rowComponent.parent->currentRoot->subMenu.get())
        {
            auto& item = (*m)[rowComponent.rowNumber];
            if (item.isEnabled)
            {
                state = state.withSelectable();
            }
            if (rowComponent.isRowSelected)
                state = state.withSelected();
        }
        return state;
    }

    int getRow() const
    {
        return rowComponent.rowNumber;
    }

private:
    class RowCellInterface : public juce::AccessibilityCellInterface
    {
    public:
        explicit RowCellInterface (RowAccessibilityHandler& h) : handler (h) {}

        int getColumnIndex() const override { return 0; }
        int getColumnSpan() const override { return 1; }

        int getRowIndex() const override
        {
            return handler.getRow();
        }

        int getRowSpan() const override { return 1; }

        int getDisclosureLevel() const override { return 0; }

        const AccessibilityHandler* getTableHandler() const override
        {
            return handler.rowComponent.owner.getAccessibilityHandler();
        }

    private:
        RowAccessibilityHandler& handler;
    };
    ListBoxMenu::RowComponent& rowComponent;
};

std::unique_ptr<juce::AccessibilityHandler> ListBoxMenu::RowComponent::createAccessibilityHandler()
{
    return std::make_unique<RowAccessibilityHandler> (*this);
}

void ListBoxMenu::RowComponent::paint (juce::Graphics& g)
{
    auto& item = (*parent->currentRoot->subMenu)[rowNumber];
    if (item.customComponent == nullptr)
    {
        if (item.isSectionHeader)
            getLookAndFeel().drawPopupMenuSectionHeader (g, getLocalBounds(), item.text);
        else
            getLookAndFeel().drawPopupMenuItem (g, { 0, 0, getWidth(), getHeight() }, item.isSeparator, item.isEnabled, (isRowSelected || isDown || isSecondary) && item.isEnabled, item.isTicked, item.subMenu != nullptr, item.text, item.shortcutKeyDescription, item.image.get(), item.colour.isTransparent() ? nullptr : &item.colour);
    }
}

bool ListBoxMenu::RowComponent::isSecondaryClick (const juce::MouseEvent& e) const
{
    bool secondaryOption =
#if JUCE_MAC
        e.mods.isCommandDown();
#else
        false;
#endif
    return ! e.mods.isLeftButtonDown() && (secondaryOption || e.mods.isRightButtonDown());
}

void ListBoxMenu::RowComponent::mouseDown (const juce::MouseEvent& e)
{
    isDragging = false;
    isDown = true;
    // TODO mobile long press;
    isSecondary = isSecondaryClick (e);
    repaint();
}
void ListBoxMenu::RowComponent::mouseUp (const juce::MouseEvent& e)
{
    isDown = false;
    repaint();
    if (contains (e.getPosition()))
    {
        if (! isDragging && (! isSecondary || parent->onSecondaryClick != nullptr))
            parent->listBoxItemClicked (rowNumber, e.mods.isRightButtonDown());
        else if (parent->getCurrentRootItem() != nullptr)
        {
            parent->lastSelectedRow = rowNumber;
            if (isSecondary && parent->onSecondaryClick == nullptr)
                parent->onSecondaryClick (parent->getCurrentRootItem()->subMenu->at (rowNumber));
            isSecondary = false;
            repaint();
        }
    }
}

void ListBoxMenu::RowComponent::mouseDrag (const juce::MouseEvent& e)
{
    getParentComponent()->mouseDrag (e.getEventRelativeTo (getParentComponent()));
    isDragging = owner.list.getViewport()->isCurrentlyScrollingOnDrag();
}

void ListBoxMenu::RowComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& d)
{
    owner.list.mouseWheelMove (e, d);
}

} // namespace jux
