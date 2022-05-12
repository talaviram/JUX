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
        rowComponent.owner.scrollToEnsureRowIsOnscreen (rowComponent.row);
        rowComponent.owner.selectRow (rowComponent.row);
    };

    auto onPress = [&rowComponent, onFocus]
    {
        onFocus();
        rowComponent.owner.keyPressed (juce::KeyPress (juce::KeyPress::returnKey));
    };

    auto onToggle = [&rowComponent]
    {
        rowComponent.owner.flipRowSelection (rowComponent.row);
    };

    return juce::AccessibilityActions().addAction (juce::AccessibilityActionType::focus,  std::move (onFocus))
                                 .addAction (juce::AccessibilityActionType::press,  std::move (onPress))
                                 .addAction (juce::AccessibilityActionType::toggle, std::move (onToggle));
}

class ListBox::RowComponent : public juce::Component, public juce::TooltipClient
{
public:
    RowComponent (ListBox& lb) : owner (lb) {}

    void paint (juce::Graphics& g) override
    {
        if (auto* m = owner.getModel())
            m->paintListBoxItem (row, g, getWidth(), getHeight(), isSelected);
    }

    void update (const int newRow, const bool nowSelected)
    {
        if (row != newRow || isSelected != nowSelected)
        {
            repaint();
            row = newRow;
            isSelected = nowSelected;
        }

        if (auto* m = owner.getModel())
        {
            if (newRow > m->getNumRows())
                return;

            setMouseCursor (m->getMouseCursorForRow (row));

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
        owner.selectRowsBasedOnModifierKeys (row, e.mods, isMouseUp);

        if (auto* m = owner.getModel())
            m->listBoxItemClicked (row, e);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        isDragging = false;
        isDraggingToScroll = false;
        selectRowOnMouseUp = false;

        if (isEnabled())
        {
            if (owner.selectOnMouseDown && ! isSelected && ! viewportWouldScrollOnEvent (owner.getViewport(), e.source))
                performSelection (e, false);
            else
                selectRowOnMouseUp = true;
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (isEnabled() && selectRowOnMouseUp && ! (isDragging || isDraggingToScroll))
            performSelection (e, true);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (isEnabled())
            if (auto* m = owner.getModel())
                m->listBoxItemDoubleClicked (row, e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (auto* m = owner.getModel())
        {
            if (isEnabled() && e.mouseWasDraggedSinceMouseDown() && ! isDragging)
            {
                juce::SparseSet<int> rowsToDrag;

                if (owner.selectOnMouseDown || owner.isRowSelected (row))
                    rowsToDrag = owner.getSelectedRows();
                else
                    rowsToDrag.addRange (juce::Range<int>::withStartAndLength (row, 1));

                if (rowsToDrag.size() > 0)
                {
                    auto dragDescription = m->getDragSourceDescription (rowsToDrag);

                    if (! (dragDescription.isVoid() || (dragDescription.isString() && dragDescription.toString().isEmpty())))
                    {
                        isDragging = true;
                        owner.startDragAndDrop (e, rowsToDrag, dragDescription, true);
                    }
                }
            }
        }

        if (! isDraggingToScroll)
            if (auto* vp = owner.getViewport())
                isDraggingToScroll = vp->isCurrentlyScrollingOnDrag();
    }

    void resized() override
    {
        if (customComponent != nullptr)
            customComponent->setBounds (getLocalBounds());
    }

    juce::String getTooltip() override
    {
        if (auto* m = owner.getModel())
            return m->getTooltipForRow (row);

        return {};
    }

        //==============================================================================
    class RowAccessibilityHandler  : public juce::AccessibilityHandler
    {
    public:
        explicit RowAccessibilityHandler (RowComponent& rowComponentToWrap)
            : juce::AccessibilityHandler (rowComponentToWrap,
                                    juce::AccessibilityRole::listItem,
                                    getListRowAccessibilityActions (rowComponentToWrap),
                                    { std::make_unique<RowCellInterface> (*this) }),
              rowComponent (rowComponentToWrap)
        {
        }

        juce::String getTitle() const override
        {
            if (auto* m = rowComponent.owner.getModel())
                return m->getNameForRow (rowComponent.row);

            return {};
        }

        juce::String getHelp() const override  { return rowComponent.getTooltip(); }

        juce::AccessibleState getCurrentState() const override
        {
            if (auto* m = rowComponent.owner.getModel())
                if (rowComponent.row >= m->getNumRows())
                    return juce::AccessibleState().withIgnored();

            auto state = AccessibilityHandler::getCurrentState().withAccessibleOffscreen();

            if (rowComponent.owner.multipleSelection)
                state = state.withMultiSelectable();
            else
                state = state.withSelectable();

            if (rowComponent.isSelected)
                state = state.withSelected();

            return state;
        }

    private:
        class RowCellInterface  : public juce::AccessibilityCellInterface
        {
        public:
            explicit RowCellInterface (RowAccessibilityHandler& h)  : handler (h)  {}

            int getColumnIndex() const override      { return 0; }
            int getColumnSpan() const override       { return 1; }

            int getRowIndex() const override
            {
                const auto index = handler.rowComponent.row;

                if (handler.rowComponent.owner.hasAccessibleHeaderComponent())
                    return index + 1;

                return index;
            }

            int getRowSpan() const override          { return 1; }

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

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<RowAccessibilityHandler> (*this);
    }
    //==============================================================================

    ListBox& owner;
    std::unique_ptr<Component> customComponent;
    int row = -1;
    bool isSelected = false, isDragging = false, isDraggingToScroll = false, selectRowOnMouseUp = false;

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
        auto content = std::make_unique<Component>();
        content->setWantsKeyboardFocus (false);
        setViewedComponent (content.release());
    }

    RowComponent* getComponentForRow (int row) const noexcept
    {
        if (juce::isPositiveAndBelow (row, rows.size()))
            return rows[row];

        return nullptr;
    }

    RowComponent* getComponentForRowWrapped (int row) const noexcept
    {
        return rows[row % std::max<size_t> (1, rows.size())];
    }

    RowComponent* getComponentForRowIfOnscreen (const int row) const noexcept
    {
        return (row >= firstIndex && row < firstIndex + rows.size())
                   ? getComponentForRow (row)
                   : nullptr;
    }

    int getRowNumberOfComponent (Component* const rowComponent) const noexcept
    {
        const int index = getViewedComponent()->getIndexOfChildComponent (rowComponent);
        const int num = rows.size();

        for (int i = num; --i >= 0;)
            if (((firstIndex + i) % std::max<int> (1, num)) == index)
                return firstIndex + i;

        return -1;
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
            lastWholeIndex = owner.itemHeightSum.at (previousLastIndex) <= y + getMaximumVisibleHeight() ? previousLastIndex : static_cast<int> (lastIndex);

            const int numNeeded = juce::jmin (owner.totalItems, 2 + static_cast<int> (lastIndex - firstWholeIndex));
            rows.removeRange (numNeeded, rows.size());

            while (numNeeded > rows.size())
            {
                auto newRow = new RowComponent (owner);
                rows.add (newRow);
                content.addAndMakeVisible (newRow);
            }

            for (int i = 0; i < numNeeded; ++i)
            {
                const int row = i + firstIndex;

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
            return owner.itemHeightSum[row - 1];
    }

    void selectRow (const int row, const int rowH, const bool dontScroll, const int lastSelectedRow, const int totalRows, const bool isMouseClick)
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
                setViewPosition (getViewPositionX(), getViewPositionY() + (owner.itemHeightSum.at (row) - bottom));
            }
        }

        if (! hasUpdated)
            updateContents();
    }

    void scrollToEnsureRowIsOnscreen (const int row)
    {
        if (row < firstWholeIndex)
        {
            setViewPosition (getViewPositionX(), getRowY (row));
        }
        else if (row >= lastWholeIndex)
        {
            auto bottom = getViewPositionY() + getMaximumVisibleHeight();
            setViewPosition (getViewPositionX(),
                             std::max<int> (0, getViewPositionY() + (owner.itemHeightSum.at (row + 1) - bottom)));
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
    juce::OwnedArray<RowComponent> rows;
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
    : Component (name), model (m)
{
    viewport.reset (new ListViewport (*this));
    addAndMakeVisible (viewport.get());

    ListBox::setWantsKeyboardFocus (true);
    setFocusContainerType (FocusContainerType::focusContainer);
    ListBox::colourChanged();
}

ListBox::~ListBox()
{
    headerComponent.reset();
    viewport.reset();
}

void ListBox::setModel (ListBoxModel* const newModel)
{
    if (model != newModel)
    {
        model = newModel;
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

    if (selectionChanged && model != nullptr)
    {
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
        return listRowComp->customComponent.get();

    return nullptr;
}

int ListBox::getRowNumberOfComponent (juce::Component* const rowComponent) const noexcept
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
    viewport->scrollToEnsureRowIsOnscreen (row);
}

//==============================================================================
bool ListBox::keyPressed (const juce::KeyPress& key)
{
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

    auto rowHeight = model->getRowHeight (rowNumber);
    return rowHeight > 0 ? rowHeight : getDefaultRowHeight();
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
            if (listBox.model == nullptr)
                return 0;

            const auto numRows = listBox.model->getNumRows();

            if (listBox.hasAccessibleHeaderComponent())
                return numRows + 1;

            return numRows;
        }

        int getNumColumns() const override
        {
            return 1;
        }

        const juce::AccessibilityHandler* getCellHandler (int row, int) const override
        {
            if (auto* headerHandler = getHeaderHandler())
            {
                if (row == 0)
                    return headerHandler;

                --row;
            }

            if (auto* rowComponent = listBox.viewport->getComponentForRow (row))
                return rowComponent->getAccessibilityHandler();

            return nullptr;
        }

    private:
        const juce::AccessibilityHandler* getHeaderHandler() const
        {
            if (listBox.hasAccessibleHeaderComponent())
                return listBox.headerComponent->getAccessibilityHandler();

            return nullptr;
        }

        ListBox& listBox;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableInterface)
    };

    return std::make_unique<juce::AccessibilityHandler> (*this,
                                                   juce::AccessibilityRole::list,
                                                   juce::AccessibilityActions{},
                                                   juce::AccessibilityHandler::Interfaces { std::make_unique<TableInterface> (*this) });
}

//==============================================================================
juce::Component* ListBoxModel::refreshComponentForRow (int, bool, juce::Component* existingComponentToUpdate)
{
    ignoreUnused (existingComponentToUpdate);
    jassert (existingComponentToUpdate == nullptr); // indicates a failure in the code that recycles the components
    return nullptr;
}

int ListBoxModel::getRowHeight (int rowNumber) const
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
