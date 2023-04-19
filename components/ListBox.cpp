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

#include "ListBox.h"

namespace jux
{
// private in JUCE so we need to maintain ours!
static bool viewportWouldScrollOnEvent (const juce::Viewport* vp, const juce::MouseInputSource& src) noexcept
{
    using namespace juce;
    if (vp != nullptr)
    {
        switch (vp->getScrollOnDragMode())
        {
            case Viewport::ScrollOnDragMode::all:           return true;
            case Viewport::ScrollOnDragMode::nonHover:      return ! src.canHover();
            case Viewport::ScrollOnDragMode::never:         return false;
        }
    }

    return false;
}

template <typename RowComponentType>
static juce::AccessibilityActions getListRowAccessibilityActions (RowComponentType& rowComponent)
{
    auto onFocus = [&rowComponent]
    {
        rowComponent.getOwner().scrollToEnsureRowIsOnscreen (rowComponent.getRow());
        rowComponent.getOwner().selectRow (rowComponent.getRow());
    };

    auto onPress = [&rowComponent, onFocus]
    {
        onFocus();
        rowComponent.getOwner().keyPressed (juce::KeyPress (juce::KeyPress::returnKey));
    };

    auto onToggle = [&rowComponent]
    {
        rowComponent.getOwner().flipRowSelection (rowComponent.getRow());
    };

    return juce::AccessibilityActions().addAction (juce::AccessibilityActionType::focus,  std::move (onFocus))
                                 .addAction (juce::AccessibilityActionType::press,  std::move (onPress))
                                 .addAction (juce::AccessibilityActionType::toggle, std::move (onToggle));
}

void ListBox::checkModelPtrIsValid() const
{
#if ! JUCE_DISABLE_ASSERTIONS
    // If this is hit, the model was destroyed while the ListBox was still using it.
    // You should ensure that the model remains alive for as long as the ListBox holds a pointer to it.
    // If this assertion is hit in the destructor of a ListBox instance, do one of the following:
    // - Adjust the order in which your destructors run, so that the ListBox destructor runs
    //   before the destructor of your ListBoxModel, or
    // - Call ListBox::setModel (nullptr) before destroying your ListBoxModel.
    jassert ((model == nullptr) == (weakModelPtr.lock() == nullptr));
#endif
}

//==============================================================================
/*  The ListBox and TableListBox rows both have similar mouse behaviours, which are implemented here. */
template <typename Base>
class ComponentWithListRowMouseBehaviours : public juce::Component
{
    auto& getOwner() const { return asBase().getOwner(); }

public:
    void updateRowAndSelection (const int newRow, const bool nowSelected)
    {
        const auto rowChanged       = std::exchange (row,      newRow)      != newRow;
        const auto selectionChanged = std::exchange (selected, nowSelected) != nowSelected;

        if (rowChanged || selectionChanged)
            repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        isDragging = false;
        isDraggingToScroll = false;
        selectRowOnMouseUp = false;

        if (! asBase().isEnabled())
            return;

        const auto select = getOwner().getRowSelectedOnMouseDown()
                            && ! selected
                            && ! viewportWouldScrollOnEvent (getOwner().getViewport(), e.source) ;
        if (select)
            asBase().performSelection (e, false);
        else
            selectRowOnMouseUp = true;
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (asBase().isEnabled() && selectRowOnMouseUp && ! (isDragging || isDraggingToScroll))
            asBase().performSelection (e, true);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (auto* m = getOwner().getModel())
        {
            if (asBase().isEnabled() && e.mouseWasDraggedSinceMouseDown() && ! isDragging)
            {
                juce::SparseSet<int> rowsToDrag;

                if (getOwner().getRowSelectedOnMouseDown() || getOwner().isRowSelected (row))
                    rowsToDrag = getOwner().getSelectedRows();
                else
                    rowsToDrag.addRange (juce::Range<int>::withStartAndLength (row, 1));

                if (! rowsToDrag.isEmpty())
                {
                    auto dragDescription = m->getDragSourceDescription (rowsToDrag);

                    if (! (dragDescription.isVoid() || (dragDescription.isString() && dragDescription.toString().isEmpty())))
                    {
                        isDragging = true;
                        getOwner().startDragAndDrop (e, rowsToDrag, dragDescription, m->mayDragToExternalWindows());
                    }
                }
            }
        }

        if (! isDraggingToScroll)
            if (auto* vp = getOwner().getViewport())
                isDraggingToScroll = vp->isCurrentlyScrollingOnDrag();
    }

    int getRow() const { return row; }
    bool isSelected() const { return selected; }

private:
    const Base& asBase() const { return *static_cast<const Base*> (this); }
    Base& asBase() { return *static_cast<Base*> (this); }

    int row = -1;
    bool selected = false, isDragging = false, isDraggingToScroll = false, selectRowOnMouseUp = false;
};

class ListBox::RowComponent : public juce::TooltipClient,
                              public ComponentWithListRowMouseBehaviours<RowComponent>
{
public:
    explicit RowComponent (ListBox& lb) : owner (lb) {}

    void paint (juce::Graphics& g) override
    {
        if (auto* m = owner.getModel())
            m->paintListBoxItem (getRow(), g, getWidth(), getHeight(), isSelected());
    }

    void update (const int newRow, const bool nowSelected)
    {
        updateRowAndSelection (newRow, nowSelected);

        if (auto* m = owner.getModel())
        {
            setMouseCursor (m->getMouseCursorForRow (getRow()));

            customComponent.reset (m->refreshComponentForRow (newRow, nowSelected, customComponent.release()));

            if (customComponent != nullptr)
            {
                addAndMakeVisible (customComponent.get());
                customComponent->setBounds (getLocalBounds());

                setFocusContainerType (FocusContainerType::focusContainer);
            }
            else
            {
                setFocusContainerType (FocusContainerType::none);
            }
        }
    }

    void performSelection (const juce::MouseEvent& e, bool isMouseUp)
    {
        owner.selectRowsBasedOnModifierKeys (getRow(), e.mods, isMouseUp);

        if (auto* m = owner.getModel())
            m->listBoxItemClicked (getRow(), e);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (isEnabled())
            if (auto* m = owner.getModel())
                m->listBoxItemDoubleClicked (getRow(), e);
    }

    void resized() override
    {
        if (customComponent != nullptr)
            customComponent->setBounds (getLocalBounds());
    }

    juce::String getTooltip() override
    {
        if (auto* m = owner.getModel())
            return m->getTooltipForRow (getRow());

        return {};
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<RowAccessibilityHandler> (*this);
    }

    ListBox& getOwner() const { return owner; }

    Component* getCustomComponent() const { return customComponent.get(); }

private:
    //==============================================================================
    class RowAccessibilityHandler  : public juce::AccessibilityHandler
    {
    public:
        explicit RowAccessibilityHandler (RowComponent& rowComponentToWrap)
            : AccessibilityHandler (rowComponentToWrap,
                                    juce::AccessibilityRole::listItem,
                                    getListRowAccessibilityActions (rowComponentToWrap),
                                    { std::make_unique<RowCellInterface> (*this) }),
              rowComponent (rowComponentToWrap)
        {
        }

        juce::String getTitle() const override
        {
            if (auto* m = rowComponent.owner.getModel())
                return m->getNameForRow (rowComponent.getRow());

            return {};
        }

        juce::String getHelp() const override  { return rowComponent.getTooltip(); }

        juce::AccessibleState getCurrentState() const override
        {
            if (auto* m = rowComponent.owner.getModel())
                if (rowComponent.getRow() >= m->getNumRows())
                    return juce::AccessibleState().withIgnored();

            auto state = AccessibilityHandler::getCurrentState().withAccessibleOffscreen();

            if (rowComponent.owner.multipleSelection)
                state = state.withMultiSelectable();
            else
                state = state.withSelectable();

            if (rowComponent.isSelected())
                state = state.withSelected();

            return state;
        }

    private:
        class RowCellInterface  : public juce::AccessibilityCellInterface
        {
        public:
            explicit RowCellInterface (RowAccessibilityHandler& h)  : handler (h)  {}

            int getDisclosureLevel() const override  { return 0; }

            const AccessibilityHandler* getTableHandler() const override
            {
                return handler.rowComponent.owner.getAccessibilityHandler();
            }

        private:
            RowAccessibilityHandler& handler;
        };

        RowComponent& rowComponent;
    };

    //==============================================================================
    ListBox& owner;
    std::unique_ptr<Component> customComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RowComponent)
};

//==============================================================================
class ListBox::ListViewport : public juce::Viewport
                            , private juce::Timer
{
public:
    ListViewport (ListBox& lb) : owner (lb)
    {
        setWantsKeyboardFocus (false);
        struct IgnoredComponent : Component
        {
            std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
            {
                return createIgnoredAccessibilityHandler (*this);
            }
        };

        auto content = std::make_unique<IgnoredComponent>();
        content->setWantsKeyboardFocus (false);

        setViewedComponent (content.release());
    }

    int getIndexOfFirstVisibleRow() const { return std::max (0, firstIndex - 1); }

    RowComponent* getComponentForRow (int row) const noexcept
    {
        const auto circularRow = (size_t) row % std::max<size_t> (1, rows.size());
        if (juce::isPositiveAndBelow (circularRow, rows.size()))
            return rows[circularRow].get();

        return nullptr;
    }

    RowComponent* getComponentForRowIfOnscreen (const int row) const noexcept
    {
        const auto startIndex = getIndexOfFirstVisibleRow();
        return (row >= startIndex && row < startIndex + static_cast<int>(rows.size()))
                   ? getComponentForRow (row)
                   : nullptr;
    }

    int getRowNumberOfComponent (const juce::Component* const rowComponent) const noexcept
    {
        const auto iter = std::find_if (rows.begin(), rows.end(), [=] (auto& ptr)
                                        { return ptr.get() == rowComponent; });

        if (iter == rows.end())
            return -1;

        const auto index = (int) std::distance (rows.begin(), iter);
        const auto mod = std::max (1, (int) rows.size());
        const auto startIndex = getIndexOfFirstVisibleRow();

        return index + mod * ((startIndex / mod) + (index < (startIndex % mod) ? 1 : 0));
    }

    void visibleAreaChanged (const juce::Rectangle<int>&) override
    {
        updateVisibleArea (true);

        if (auto* m = owner.getModel())
            m->listWasScrolled();

        startTimer (50);
    }

    void updateVisibleArea (const bool makeSureItUpdatesContent)
    {
        hasUpdated = false;

        auto& content = *getViewedComponent();
        auto newX = content.getX();
        auto newY = content.getY();
        auto newW = std::max<int> (owner.minimumRowWidth, getMaximumVisibleWidth());
        int newH = 0;
        for (auto i = 0; i < owner.totalItems; i++)
        {
            jassert (owner.model != nullptr);
            newH += owner.getRowHeight (i);
        }

        if (newY + newH < getMaximumVisibleHeight() && newH > getMaximumVisibleHeight())
            newY = getMaximumVisibleHeight() - newH;

        content.setBounds (newX, newY, newW, newH);

        if (makeSureItUpdatesContent && ! hasUpdated)
            updateContents();
    }

    void updateContents()
    {
        if (getMaximumVisibleHeight() > 0)
            hasUpdated = true;

        auto& content = *getViewedComponent();

        if (owner.totalItems > 0 && owner.itemHeightSum.back() > 0)
        {
            auto y = getViewPositionY();
            auto w = content.getWidth();

            const auto first = std::lower_bound (owner.itemHeightSum.begin(), owner.itemHeightSum.end(), y);
            firstIndex = static_cast<int> (first - owner.itemHeightSum.begin());
            firstWholeIndex = *first > y ? firstIndex + 1 : firstWholeIndex;

            const auto last = std::lower_bound (first, owner.itemHeightSum.end(), juce::jmax (1, y + getMaximumVisibleHeight() - 1));
            const auto lastIndex = last - owner.itemHeightSum.begin();

            const auto previousLastIndex = juce::jmax (0, static_cast<int> (lastIndex - 1));
            lastWholeIndex = owner.itemHeightSum.at ((size_t) previousLastIndex) <= y + getMaximumVisibleHeight() ? previousLastIndex : static_cast<int> (lastIndex);

            const size_t numNeeded = static_cast<size_t> (std::min (owner.totalItems, 2 + static_cast<int> (lastIndex - firstWholeIndex)));
            rows.resize (std::min (numNeeded, rows.size()));

            while (numNeeded > rows.size())
            {
                rows.push_back (std::make_unique<RowComponent> (owner));
                content.addAndMakeVisible (rows.back().get());
            }

            for (size_t i = 0; i < numNeeded; ++i)
            {
                const auto row = static_cast<int> (i) + firstIndex;
                if (auto* rowComp = getComponentForRow (row))
                {
                    auto height = owner.getRowHeight (row);
                    rowComp->setBounds (0, getRowY (row), w, height);
                    rowComp->update (row, owner.isRowSelected (row));
                }
            }
        }

        if (owner.headerComponent != nullptr)
            owner.headerComponent->setBounds (owner.outlineThickness + content.getX(),
                                              owner.outlineThickness,
                                              juce::jmax (owner.getWidth() - owner.outlineThickness * 2,
                                                          content.getWidth()),
                                              owner.headerComponent->getHeight());
    }

    int getRowY (const int row)
    {
        if (row == 0)
            return 0;
        else if (row >= owner.totalItems)
            return owner.itemHeightSum.back() + owner.getDefaultRowHeight() * (1 + row - owner.totalItems);
        else
            return owner.itemHeightSum[static_cast<size_t> (row - 1)];
    }

    void selectRow (const int row, const int /*rowH*/, const bool dontScroll, const int lastSelectedRow, const int totalRows, const bool isMouseClick)
    {
        hasUpdated = false;

        if (row < firstWholeIndex && ! dontScroll)
        {
            setViewPosition (getViewPositionX(), getRowY (row));
        }
        else if (row >= lastWholeIndex && ! dontScroll)
        {
            const int rowsOnScreen = lastWholeIndex - firstWholeIndex;

            if (row >= lastSelectedRow + rowsOnScreen
                && rowsOnScreen < totalRows - 1
                && ! isMouseClick)
            {
                setViewPosition (getViewPositionX(),
                                 getRowY (juce::jlimit (0, juce::jmax (0, totalRows - rowsOnScreen), row)));
            }
            else
            {
                auto bottom = getViewPositionY() + getMaximumVisibleHeight();
                jassert (row >= 0);
                setViewPosition (getViewPositionX(), getViewPositionY() + (owner.itemHeightSum.at ((size_t) row) - bottom));
            }
        }

        if (! hasUpdated)
            updateContents();
    }

    void scrollToEnsureRowIsOnscreen (const int row)
    {
        jassert (row >= 0);
        if (row < firstWholeIndex)
        {
            setViewPosition (getViewPositionX(), getRowY (row));
        }
        else if (row >= lastWholeIndex)
        {
            auto bottom = getViewPositionY() + getMaximumVisibleHeight();
            setViewPosition (getViewPositionX(),
                             std::max<int> (0, getViewPositionY() + (owner.itemHeightSum.at (size_t (row) + 1) - bottom)));
        }
    }

    void paint (juce::Graphics& g) override
    {
        if (isOpaque())
            g.fillAll (owner.findColour (ListBox::backgroundColourId));
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (Viewport::respondsToKey (key))
        {
            const int allowableMods = owner.multipleSelection ? juce::ModifierKeys::shiftModifier : 0;

            if ((key.getModifiers().getRawFlags() & ~allowableMods) == 0)
            {
                // we want to avoid these keypresses going to the viewport, and instead allow
                // them to pass up to our listbox..
                return false;
            }
        }

        return Viewport::keyPressed (key);
    }

private:
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return createIgnoredAccessibilityHandler (*this);
    }

    void timerCallback() override
    {
        stopTimer();

        if (auto* handler = owner.getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::structureChanged);
    }

    ListBox& owner;
    std::vector<std::unique_ptr<RowComponent>> rows;
    int firstIndex = 0, firstWholeIndex = 0, lastWholeIndex = 0;
    bool hasUpdated = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ListViewport)
};

//==============================================================================
struct ListBoxMouseMoveSelector : public juce::MouseListener
{
    ListBoxMouseMoveSelector (ListBox& lb) : owner (lb)
    {
        owner.addMouseListener (this, true);
    }

    ~ListBoxMouseMoveSelector() override
    {
        owner.removeMouseListener (this);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        auto pos = e.getEventRelativeTo (&owner).position.toInt();
        owner.selectRow (owner.getRowContainingPosition (pos.x, pos.y), true);
    }

    void mouseExit (const juce::MouseEvent& e) override
    {
        mouseMove (e);
    }

    ListBox& owner;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ListBoxMouseMoveSelector)
};

//==============================================================================
ListBox::ListBox (const juce::String& name, ListBoxModel* const m)
    : Component (name)
{
    viewport.reset (new ListViewport (*this));
    addAndMakeVisible (viewport.get());

    setWantsKeyboardFocus (true);
    setFocusContainerType (FocusContainerType::focusContainer);
    colourChanged();

    assignModelPtr (m);
}

ListBox::~ListBox()
{
    headerComponent.reset();
    viewport.reset();
}

void ListBox::assignModelPtr (ListBoxModel* const newModel)
{
    model = newModel;

#if ! JUCE_DISABLE_ASSERTIONS
    weakModelPtr = model != nullptr ? model->sharedState : nullptr;
#endif
}

void ListBox::setModel (ListBoxModel* const newModel)
{
    if (model != newModel)
    {
        assignModelPtr (newModel);
        repaint();
        updateContent();
    }
}

void ListBox::setMultipleSelectionEnabled (bool b) noexcept { multipleSelection = b; }
void ListBox::setClickingTogglesRowSelection (bool b) noexcept { alwaysFlipSelection = b; }
void ListBox::setRowSelectedOnMouseDown (bool b) noexcept { selectOnMouseDown = b; }

void ListBox::setMouseMoveSelectsRows (bool b)
{
    if (b)
    {
        if (mouseMoveSelector == nullptr)
            mouseMoveSelector.reset (new ListBoxMouseMoveSelector (*this));
    }
    else
    {
        mouseMoveSelector.reset();
    }
}

//==============================================================================
void ListBox::paint (juce::Graphics& g)
{
    if (! hasDoneInitialUpdate)
        updateContent();

    g.fillAll (findColour (backgroundColourId));
}

void ListBox::paintOverChildren (juce::Graphics& g)
{
    if (outlineThickness > 0)
    {
        g.setColour (findColour (outlineColourId));
        g.drawRect (getLocalBounds(), outlineThickness);
    }
}

void ListBox::resized()
{
    viewport->setBoundsInset (juce::BorderSize<int> (outlineThickness + (headerComponent != nullptr ? headerComponent->getHeight() : 0),
                                                     outlineThickness,
                                                     outlineThickness,
                                                     outlineThickness));

    viewport->setSingleStepSizes (20, getDefaultRowHeight());

    viewport->updateVisibleArea (false);
}

void ListBox::visibilityChanged()
{
    viewport->updateVisibleArea (true);
}

juce::Viewport* ListBox::getViewport() const noexcept
{
    return viewport.get();
}

//==============================================================================
void ListBox::updateContent()
{
    checkModelPtrIsValid();
    hasDoneInitialUpdate = true;
    totalItems = (model != nullptr) ? model->getNumRows() : 0;

    if (model != nullptr)
    {
        // update item height sums
        itemHeightSum.clear();
        auto sumToRow = 0;
        for (auto i = 0; i < totalItems; i++)
        {
            jassert (i < totalItems);
            sumToRow += getRowHeight (i);
            itemHeightSum.push_back (sumToRow);
        }
    }

    bool selectionChanged = false;

    if (selected.size() > 0 && selected[selected.size() - 1] >= totalItems)
    {
        selected.removeRange ({ totalItems, std::numeric_limits<int>::max() });
        lastRowSelected = getSelectedRow (0);
        selectionChanged = true;
    }

    viewport->updateVisibleArea (isVisible());
    viewport->resized();

    if (selectionChanged)
    {
        if (model != nullptr)
            model->selectedRowsChanged (lastRowSelected);
        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::rowSelectionChanged);
    }
}

//==============================================================================
void ListBox::selectRow (int row, bool dontScroll, bool deselectOthersFirst)
{
    selectRowInternal (row, dontScroll, deselectOthersFirst, false);
}

void ListBox::selectRowInternal (const int row,
                                 bool dontScroll,
                                 bool deselectOthersFirst,
                                 bool isMouseClick)
{
    checkModelPtrIsValid();

    if (! multipleSelection)
        deselectOthersFirst = true;

    if ((! isRowSelected (row))
        || (deselectOthersFirst && getNumSelectedRows() > 1))
    {
        if (juce::isPositiveAndBelow (row, totalItems))
        {
            if (deselectOthersFirst)
                selected.clear();

            selected.addRange ({ row, row + 1 });

            if (getHeight() == 0 || getWidth() == 0)
                dontScroll = true;

            viewport->selectRow (row, getRowHeight (row), dontScroll, lastRowSelected, totalItems, isMouseClick);

            lastRowSelected = row;
            model->selectedRowsChanged (row);

            if (auto* handler = getAccessibilityHandler())
                handler->notifyAccessibilityEvent (juce::AccessibilityEvent::rowSelectionChanged);
        }
        else
        {
            if (deselectOthersFirst)
                deselectAllRows();
        }
    }
}

void ListBox::deselectRow (const int row)
{
    checkModelPtrIsValid();

    if (selected.contains (row))
    {
        selected.removeRange ({ row, row + 1 });

        if (row == lastRowSelected)
            lastRowSelected = getSelectedRow (0);

        viewport->updateContents();
        model->selectedRowsChanged (lastRowSelected);

        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::rowSelectionChanged);
    }
}

void ListBox::setSelectedRows (const juce::SparseSet<int>& setOfRowsToBeSelected,
                               const juce::NotificationType sendNotificationEventToModel)
{
    checkModelPtrIsValid();

    selected = setOfRowsToBeSelected;
    selected.removeRange ({ totalItems, std::numeric_limits<int>::max() });

    if (! isRowSelected (lastRowSelected))
        lastRowSelected = getSelectedRow (0);

    viewport->updateContents();

    if (model != nullptr && sendNotificationEventToModel == juce::sendNotification)
        model->selectedRowsChanged (lastRowSelected);

    if (auto* handler = getAccessibilityHandler())
        handler->notifyAccessibilityEvent (juce::AccessibilityEvent::rowSelectionChanged);
}

juce::SparseSet<int> ListBox::getSelectedRows() const
{
    return selected;
}

void ListBox::selectRangeOfRows (int firstRow, int lastRow, bool dontScrollToShowThisRange)
{
    if (multipleSelection && (firstRow != lastRow))
    {
        const int numRows = totalItems - 1;
        firstRow = juce::jlimit (0, std::max<int> (0, numRows), firstRow);
        lastRow = juce::jlimit (0, std::max<int> (0, numRows), lastRow);

        selected.addRange ({ std::min<int> (firstRow, lastRow),
                             std::max<int> (firstRow, lastRow) + 1 });

        selected.removeRange ({ lastRow, lastRow + 1 });
    }

    selectRowInternal (lastRow, dontScrollToShowThisRange, false, true);
}

void ListBox::flipRowSelection (const int row)
{
    if (isRowSelected (row))
        deselectRow (row);
    else
        selectRowInternal (row, false, false, true);
}

void ListBox::deselectAllRows()
{
    checkModelPtrIsValid();

    if (! selected.isEmpty())
    {
        selected.clear();
        lastRowSelected = -1;

        viewport->updateContents();

        if (model != nullptr)
            model->selectedRowsChanged (lastRowSelected);

        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::rowSelectionChanged);
    }
}

void ListBox::selectRowsBasedOnModifierKeys (const int row,
                                             juce::ModifierKeys mods,
                                             const bool isMouseUpEvent)
{
    if (multipleSelection && (mods.isCommandDown() || alwaysFlipSelection))
    {
        flipRowSelection (row);
    }
    else if (multipleSelection && mods.isShiftDown() && lastRowSelected >= 0)
    {
        selectRangeOfRows (lastRowSelected, row);
    }
    else if ((! mods.isPopupMenu()) || ! isRowSelected (row))
    {
        selectRowInternal (row, false, ! (multipleSelection && (! isMouseUpEvent) && isRowSelected (row)), true);
    }
}

int ListBox::getNumSelectedRows() const
{
    return selected.size();
}

int ListBox::getSelectedRow (const int index) const
{
    return (juce::isPositiveAndBelow (index, selected.size()))
               ? selected[index]
               : -1;
}

bool ListBox::isRowSelected (const int row) const
{
    return selected.contains (row);
}

int ListBox::getLastRowSelected() const
{
    return isRowSelected (lastRowSelected) ? lastRowSelected : -1;
}

//==============================================================================
int ListBox::getRowContainingPosition (const int x, const int y) const noexcept
{
    if (juce::isPositiveAndBelow (x, getWidth()))
    {
        const auto absoluteY = viewport->getViewPositionY() + y;
        const auto row = *std::lower_bound (itemHeightSum.begin(), itemHeightSum.end(), absoluteY);

        if (juce::isPositiveAndBelow (row, itemHeightSum.back()))
            return row;
    }

    return -1;
}

int ListBox::getInsertionIndexForPosition (const int x, const int y) const noexcept
{
    if (juce::isPositiveAndBelow (x, getWidth()))
        return juce::jlimit (0, totalItems, (viewport->getViewPositionY() + y + rowHeight / 2 - viewport->getY()) / rowHeight);

    return -1;
}

juce::Component* ListBox::getComponentForRowNumber (const int row) const noexcept
{
    if (auto* listRowComp = viewport->getComponentForRowIfOnscreen (row))
        return listRowComp->getCustomComponent();

    return nullptr;
}

int ListBox::getRowNumberOfComponent (const juce::Component* const rowComponent) const noexcept
{
    return viewport->getRowNumberOfComponent (rowComponent);
}

juce::Rectangle<int> ListBox::getRowPosition (int rowNumber, bool relativeToComponentTopLeft) const noexcept
{
    auto y = viewport->getY() + rowHeight * rowNumber;

    if (relativeToComponentTopLeft)
        y -= viewport->getViewPositionY();

    return { viewport->getX(), y, viewport->getViewedComponent()->getWidth(), rowHeight };
}

void ListBox::setVerticalPosition (const double proportion)
{
    auto offscreen = viewport->getViewedComponent()->getHeight() - viewport->getHeight();

    viewport->setViewPosition (viewport->getViewPositionX(),
                               std::max<int> (0, juce::roundToInt (proportion * offscreen)));
}

double ListBox::getVerticalPosition() const
{
    auto offscreen = viewport->getViewedComponent()->getHeight() - viewport->getHeight();

    return offscreen > 0 ? viewport->getViewPositionY() / (double) offscreen
                         : 0;
}

int ListBox::getVisibleRowWidth() const noexcept
{
    return viewport->getViewWidth();
}

void ListBox::scrollToEnsureRowIsOnscreen (const int row)
{
    viewport->scrollToEnsureRowIsOnscreen (row); // viewport->scrollToEnsureRowIsOnscreen (row, getRowHeight());
}

//==============================================================================
bool ListBox::keyPressed (const juce::KeyPress& key)
{
    checkModelPtrIsValid();
    //    const int numVisibleRows = 0; //viewport->getHeight() / getRowHeight();

    const bool multiple = multipleSelection
                          && lastRowSelected >= 0
                          && key.getModifiers().isShiftDown();

    if (key.isKeyCode (juce::KeyPress::upKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, lastRowSelected - 1);
        else
            selectRow (std::max<int> (0, lastRowSelected - 1));
    }
    else if (key.isKeyCode (juce::KeyPress::downKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, lastRowSelected + 1);
        else
            selectRow (std::min<int> (totalItems - 1, std::max<int> (0, lastRowSelected + 1)));
    }

    else if (key.isKeyCode (juce::KeyPress::homeKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, 0);
        else
            selectRow (0);
    }
    else if (key.isKeyCode (juce::KeyPress::endKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, totalItems - 1);
        else
            selectRow (totalItems - 1);
    }
    else if (key.isKeyCode (juce::KeyPress::returnKey) && isRowSelected (lastRowSelected))
    {
        if (model != nullptr)
            model->returnKeyPressed (lastRowSelected);
    }
    else if ((key.isKeyCode (juce::KeyPress::deleteKey) || key.isKeyCode (juce::KeyPress::backspaceKey))
             && isRowSelected (lastRowSelected))
    {
        if (model != nullptr)
            model->deleteKeyPressed (lastRowSelected);
    }
    else if (multipleSelection && key == juce::KeyPress ('a', juce::ModifierKeys::commandModifier, 0))
    {
        selectRangeOfRows (0, std::numeric_limits<int>::max());
    }
    else
    {
        return false;
    }

    return true;
}

bool ListBox::keyStateChanged (const bool isKeyDown)
{
    using namespace juce;
    return isKeyDown
           && (KeyPress::isKeyCurrentlyDown (KeyPress::upKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::pageUpKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::downKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::pageDownKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::homeKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::endKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::returnKey));
}

void ListBox::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    bool eventWasUsed = false;

    if (wheel.deltaX != 0.0f && getHorizontalScrollBar().isVisible())
    {
        eventWasUsed = true;
        getHorizontalScrollBar().mouseWheelMove (e, wheel);
    }

    if (wheel.deltaY != 0.0f && getVerticalScrollBar().isVisible())
    {
        eventWasUsed = true;
        getVerticalScrollBar().mouseWheelMove (e, wheel);
    }

    if (! eventWasUsed)
        Component::mouseWheelMove (e, wheel);
}

void ListBox::mouseUp (const juce::MouseEvent& e)
{
    checkModelPtrIsValid();

    if (e.mouseWasClicked() && model != nullptr)
        model->backgroundClicked (e);
}

//==============================================================================
void ListBox::setDefaultRowHeight (const int newHeight)
{
    rowHeight = std::max<int> (1, newHeight);
    viewport->setSingleStepSizes (20, rowHeight);
    updateContent();
}

int ListBox::getDefaultRowHeight() const noexcept
{
    return rowHeight;
}

int ListBox::getRowHeight (const int rowNumber) const noexcept
{
    if (model == nullptr || rowNumber >= totalItems)
        return getDefaultRowHeight();

    const auto heightForRow = model->getRowHeight (rowNumber);
    return heightForRow > 0 ? heightForRow : getDefaultRowHeight();
}

int ListBox::getNumRowsOnScreen() const noexcept
{
    const auto* vp = getViewport();
    const auto firstVisibleRowIndex = std::lower_bound (itemHeightSum.begin(), itemHeightSum.end(), vp->getViewPositionY()) - itemHeightSum.begin();
    const auto lastVisibleRowIndex = std::lower_bound (itemHeightSum.begin(), itemHeightSum.end(), vp->getViewPositionY() + vp->getViewHeight()) - itemHeightSum.begin();
    return static_cast<int> (lastVisibleRowIndex - firstVisibleRowIndex);
}

void ListBox::setMinimumContentWidth (const int newMinimumWidth)
{
    minimumRowWidth = newMinimumWidth;
    updateContent();
}

int ListBox::getVisibleContentWidth() const noexcept { return viewport->getMaximumVisibleWidth(); }

juce::ScrollBar& ListBox::getVerticalScrollBar() const noexcept { return viewport->getVerticalScrollBar(); }
juce::ScrollBar& ListBox::getHorizontalScrollBar() const noexcept { return viewport->getHorizontalScrollBar(); }

void ListBox::colourChanged()
{
    setOpaque (findColour (backgroundColourId).isOpaque());
    viewport->setOpaque (isOpaque());
    repaint();
}

void ListBox::parentHierarchyChanged()
{
    colourChanged();
}

void ListBox::setOutlineThickness (int newThickness)
{
    outlineThickness = newThickness;
    resized();
}

void ListBox::setHeaderComponent (std::unique_ptr<Component> newHeaderComponent)
{
    headerComponent = std::move (newHeaderComponent);
    addAndMakeVisible (headerComponent.get());
    ListBox::resized();
    invalidateAccessibilityHandler();
}

bool ListBox::hasAccessibleHeaderComponent() const
{
    return headerComponent != nullptr
            && headerComponent->getAccessibilityHandler() != nullptr;
}

void ListBox::repaintRow (const int rowNumber) noexcept
{
    repaint (getRowPosition (rowNumber, true));
}

juce::ScaledImage ListBox::createSnapshotOfRows (const juce::SparseSet<int>& rows, int& imageX, int& imageY)
{
    juce::Rectangle<int> imageArea;
    auto firstRow = getRowContainingPosition (0, viewport->getY());

    for (int i = getNumRowsOnScreen() + 2; --i >= 0;)
    {
        if (rows.contains (firstRow + i))
        {
            if (auto* rowComp = viewport->getComponentForRowIfOnscreen (firstRow + i))
            {
                auto pos = getLocalPoint (rowComp, juce::Point<int>());

                imageArea = imageArea.getUnion ({ pos.x, pos.y, rowComp->getWidth(), rowComp->getHeight() });
            }
        }
    }

    imageArea = imageArea.getIntersection (getLocalBounds());
    imageX = imageArea.getX();
    imageY = imageArea.getY();

    const auto additionalScale = 2.0f;
    const auto listScale = Component::getApproximateScaleFactorForComponent (this) * additionalScale;
    juce::Image snapshot (juce::Image::ARGB,
                    juce::roundToInt ((float) imageArea.getWidth() * listScale),
                    juce::roundToInt ((float) imageArea.getHeight() * listScale),
                    true);

    for (int i = getNumRowsOnScreen() + 2; --i >= 0;)
    {
        if (rows.contains (firstRow + i))
        {
            if (auto* rowComp = viewport->getComponentForRowIfOnscreen (firstRow + i))
            {
                juce::Graphics g (snapshot);
                g.setOrigin ((getLocalPoint (rowComp, juce::Point<int>()) - imageArea.getPosition()) * additionalScale);

                const auto rowScale = juce::Component::getApproximateScaleFactorForComponent (rowComp) * additionalScale;

                if (g.reduceClipRegion (rowComp->getLocalBounds() * rowScale))
                {
                    g.beginTransparencyLayer (0.6f);
                    g.addTransform (juce::AffineTransform::scale (rowScale));
                    rowComp->paintEntireComponent (g, false);
                    g.endTransparencyLayer();
                }
            }
        }
    }

    return { snapshot, additionalScale };
}

void ListBox::startDragAndDrop (const juce::MouseEvent& e, const juce::SparseSet<int>& rowsToDrag, const juce::var& dragDescription, bool allowDraggingToOtherWindows)
{
    if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor (this))
    {
        int x, y;
        auto dragImage = createSnapshotOfRows (rowsToDrag, x, y);

        auto p = juce::Point<int> (x, y) - e.getEventRelativeTo (this).position.toInt();
        dragContainer->startDragging (dragDescription, this, dragImage, allowDraggingToOtherWindows, &p, &e.source);
    }
    else
    {
        // to be able to do a drag-and-drop operation, the listbox needs to
        // be inside a component which is also a DragAndDropContainer.
        jassertfalse;
    }
}

std::unique_ptr<juce::AccessibilityHandler> ListBox::createAccessibilityHandler()
{
    class TableInterface  : public juce::AccessibilityTableInterface
    {
    public:
        explicit TableInterface (ListBox& listBoxToWrap)
            : listBox (listBoxToWrap)
        {
        }

        int getNumRows() const override
        {
            listBox.checkModelPtrIsValid();

            return listBox.model != nullptr ? listBox.model->getNumRows()
                                            : 0;
        }

        int getNumColumns() const override
        {
            return 1;
        }

        const juce::AccessibilityHandler* getHeaderHandler() const override
        {
            if (listBox.hasAccessibleHeaderComponent())
                return listBox.headerComponent->getAccessibilityHandler();

            return nullptr;
        }

        const juce::AccessibilityHandler* getRowHandler (int row) const override
        {
            if (auto* rowComponent = listBox.viewport->getComponentForRowIfOnscreen (row))
                return rowComponent->getAccessibilityHandler();

            return nullptr;
        }

        const juce::AccessibilityHandler* getCellHandler (int, int) const override
        {
            return nullptr;
        }

        juce::Optional<Span> getRowSpan (const juce::AccessibilityHandler& handler) const override
        {
            const auto rowNumber = listBox.getRowNumberOfComponent (&handler.getComponent());

            return rowNumber != -1 ? makeOptional (Span { rowNumber, 1 })
                                   : juce::nullopt;
        }

        juce::Optional<Span> getColumnSpan (const juce::AccessibilityHandler&) const override
        {
            return Span { 0, 1 };
        }

        void showCell (const juce::AccessibilityHandler& h) const override
        {
            if (const auto row = getRowSpan (h))
                listBox.scrollToEnsureRowIsOnscreen (row->begin);
        }

    private:
        ListBox& listBox;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableInterface)
    };

    return std::make_unique<juce::AccessibilityHandler> (*this,
                                                   juce::AccessibilityRole::list,
                                                   juce::AccessibilityActions{},
                                                   juce::AccessibilityHandler::Interfaces { std::make_unique<TableInterface> (*this) });
}

//==============================================================================
juce::Component* ListBoxModel::refreshComponentForRow (int, bool, [[maybe_unused]] juce::Component* existingComponentToUpdate)
{
    jassert (existingComponentToUpdate == nullptr); // indicates a failure in the code that recycles the components
    return nullptr;
}

int ListBoxModel::getRowHeight (int /*rowNumber*/) const
{
    return -1;
}

juce::String ListBoxModel::getNameForRow (int rowNumber)                      { return "Row " + juce::String (rowNumber + 1); }
void ListBoxModel::listBoxItemClicked (int, const juce::MouseEvent&) {}
void ListBoxModel::listBoxItemDoubleClicked (int, const juce::MouseEvent&) {}
void ListBoxModel::backgroundClicked (const juce::MouseEvent&) {}
void ListBoxModel::selectedRowsChanged (int) {}
void ListBoxModel::deleteKeyPressed (int) {}
void ListBoxModel::returnKeyPressed (int) {}
void ListBoxModel::listWasScrolled() {}
juce::var ListBoxModel::getDragSourceDescription (const juce::SparseSet<int>&) { return {}; }
juce::String ListBoxModel::getTooltipForRow (int) { return {}; }
juce::MouseCursor ListBoxModel::getMouseCursorForRow (int) { return juce::MouseCursor::NormalCursor; }

} // namespace jux
