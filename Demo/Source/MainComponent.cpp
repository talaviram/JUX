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

#include "MainComponent.h"

#include "../../components/ListBox.h"
#include "../../components/ListBoxMenu.h"
#include "../../components/SwitchButton.h"

using namespace juce;

enum class MenuOptions
{
    SwitchButtonDemo,
    ListBoxDemo,
    ListBoxMenuDemo
};
template <typename E>
constexpr typename std::underlying_type<E>::type enumAsInt (E e, bool nonZero = false) noexcept
{
    return static_cast<typename std::underlying_type<E>::type> (e) + (nonZero ? 1 : 0);
}

class DemoMenu : public MenuBarModel
{
    friend MainComponent;

public:
    StringArray getMenuBarNames() override
    {
        return { "UI Components" };
    }
    PopupMenu getMenuForIndex (int topLevelMenuIndex, const String& menuName) override
    {
        PopupMenu menu;
        menu.addItem (enumAsInt (MenuOptions::SwitchButtonDemo, true), "jux::SwitchButton");
        menu.addItem (enumAsInt (MenuOptions::ListBoxDemo, true), "jux::ListBox");
        menu.addItem (enumAsInt (MenuOptions::ListBoxMenuDemo, true), "jux::ListBoxMenu");
        return menu;
    }

    void menuItemSelected (int menuItemID, int topLevelMenuIndex) override
    {
        owner->sidePanel.showOrHide (false);
        Component* cmp = nullptr;
        switch (menuItemID)
        {
            case enumAsInt (MenuOptions::SwitchButtonDemo, true):
                cmp = owner->getComponent (enumAsInt (MenuOptions::SwitchButtonDemo));
                break;
            case enumAsInt (MenuOptions::ListBoxDemo, true):
                cmp = owner->getComponent (enumAsInt (MenuOptions::ListBoxDemo));
                break;
            case enumAsInt (MenuOptions::ListBoxMenuDemo, true):
                cmp = owner->getComponent (enumAsInt (MenuOptions::ListBoxMenuDemo));
                break;
            default:
                return;
        }
        cmp->toFront (false);
        cmp->toBehind (&owner->sidePanel);
        cmp->setVisible (true);
    }

    MainComponent* owner;
} demoMenu;

struct ListBoxDemo : public Component, jux::ListBoxModel
{
    static constexpr auto kNumOfItems = 64;
    ListBoxDemo()
    {
        listBox.setModel (this);
        addAndMakeVisible (listBox);
    }

    void resized() override
    {
        listBox.setBounds (getLocalBounds());
    }

    int getNumRows() override { return kNumOfItems; }

    int getRowHeight (int rowNumber) const override
    {
        return rowNumber % 2 ? 30 : 50;
    }

    void paintListBoxItem (int rowNumber,
                           Graphics& g,
                           int width,
                           int height,
                           bool rowIsSelected) override
    {
        g.fillAll (rowIsSelected ? Colours::blue : (rowNumber % 2 ? Colours::darkgrey : Colours::grey));
        g.setColour (Colours::white);
        g.drawFittedText ("Item " + String (rowNumber), 0, 0, width, height, Justification::left, 1);
    }

    jux::ListBox listBox;
};

struct SwitchButtonDemo : public Component
{
    SwitchButtonDemo()
        : darkModeSwitch ("Color", false)
    {
        darkModeTitle.setColour (Label::ColourIds::textColourId, Colours::black);
        darkModeTitle.setText ("Dark Mode", dontSendNotification);
        darkModeSwitch.onClick = [this] {
            darkModeTitle.setColour (Label::ColourIds::textColourId, darkModeSwitch.getToggleState() ? Colours::white : Colours::black);
            repaint();
        };
        addAndMakeVisible (darkModeTitle);
        addAndMakeVisible (darkModeSwitch);
        if (auto* top = getTopLevelComponent())
            top->repaint();
    }

    void resized() override
    {
        auto row = getLocalBounds().removeFromTop (40);
        darkModeSwitch.setBounds (row.removeFromRight (60).reduced (2));
        darkModeTitle.setBounds (row);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (darkModeSwitch.getToggleState() ? Colours::black : Colours::white);
    }

    Label darkModeTitle;
    jux::SwitchButton darkModeSwitch;
};

struct ListBoxMenuDemo : public Component
{
public:
    class CustomTextEditor : public PopupMenu::CustomComponent
    {
    public:
        CustomTextEditor()
        {
            editor.setMultiLine (true);
            editor.setText ("Hello ListBox!\n\n\n\nThis is a text box....\nYou can edit it!");
            addAndMakeVisible (editor);
        }

        void getIdealSize (int& idealWidth, int& idealHeight) override
        {
            idealWidth = editor.getTextWidth();
            idealHeight = editor.getTextHeight();
        }

        void resized() override
        {
            editor.setBounds (getLocalBounds());
        }

    private:
        TextEditor editor;
    };

    ListBoxMenuDemo()
    {
        openAsPopup.setButtonText ("Pop-up...");
        openAsListBoxMenu.setButtonText ("ListBoxMenu (close on click)...");
        openAsListBoxMenuInteractive.setButtonText ("ListBoxMenu (interactive)...");
        openAsCallOut.setButtonText ("CallOut...");
        addAndMakeVisible (openAsPopup);
        addAndMakeVisible (openAsListBoxMenu);
        addAndMakeVisible (openAsListBoxMenuInteractive);
        addAndMakeVisible (openAsCallOut);

        openAsPopup.onClick = [this] {
            jucePopupMenu.showAt (&openAsPopup);
        };

        openAsListBoxMenu.onClick = [this] {
            isInteractive = false;
            if (listBoxMenu == nullptr || ! listBoxMenu->isShowing())
                setupListBoxMenuFromPopup();
            else if (listBoxMenu->isShowing())
                listBoxMenu->animateAndClose();
        };

        openAsListBoxMenuInteractive.onClick = [this] {
            isInteractive = true;
            if (listBoxMenu == nullptr || ! listBoxMenu->isShowing())
                setupListBoxMenuFromPopup();
            else if (listBoxMenu->isShowing())
                listBoxMenu->animateAndClose();
        };

        openAsCallOut.onClick = [this] {
            auto content = std::make_unique<jux::ListBoxMenu>();
            PopupMenu menu;
            for (auto i = 0; i < 20; i++)
                menu.addItem ("Item " + String (i), nullptr);
            content->setMenuFromPopup (std::move (menu));
            content->setSize (300, 300);
            content->setHideHeaderOnParent (true);
            content->setShouldCloseOnItemClick (true);
            CallOutBox::launchAsynchronously (std::move (content), openAsCallOut.getBounds(), getParentComponent());
        };

        // JUCE PopupMenu converted to ListBoxMenu
        // Can be useful for preset selection or basic popup when
        // clicking should close UI
        jucePopupMenu.addSectionHeader ("Section Header");
        for (auto i = 0; i < 10; i++)
        {
            jucePopupMenu.addItem ("Item " + String (i), Random().nextBool(), Random().nextBool(), [this, i] {
                if (isInteractive)
                {
                    auto& item = listBoxMenu->getCurrentRootItem()->subMenu->at (i + 1);
                    if (item.isSeparator || item.isSectionHeader || ! item.isEnabled)
                        return;

                    item.setTicked (! item.isTicked);
                }
                else
                {
                    NativeMessageBox::showMessageBoxAsync (AlertWindow::InfoIcon, "ListBoxMenu", "Item " + String (i) + " clicked.");
                }
            });
        }
        jucePopupMenu.addSeparator();

        PopupMenu subMenu;
        for (auto i = 0; i < 10; i++)
            subMenu.addColouredItem (200 + i, "Sub " + String (i), Colour (static_cast<uint32> (Random().nextInt())), true, false, nullptr);
        jucePopupMenu.addSubMenu ("SubMenu", subMenu);
        jucePopupMenu.addSeparator();
        jucePopupMenu.addCustomItem (400, std::make_unique<CustomTextEditor>());
    }

    void setupListBoxMenuFromPopup()
    {
        auto menu = jucePopupMenu;
        listBoxMenu = std::make_unique<jux::ListBoxMenu>();
        listBoxMenu->setMenuFromPopup (std::move (jucePopupMenu));
        listBoxMenu->setShouldCloseOnItemClick (! isInteractive);

        listBoxMenu->setBackButtonShowText (true);
        listBoxMenu->setColour (PopupMenu::ColourIds::backgroundColourId, Colours::grey);
        listBoxMenu->setOnRootBackToParent ([&] {
            listBoxMenu->animateAndClose();
        });
        addAndMakeVisible (listBoxMenu.get());
        listBoxMenu->setBounds (0, 40, getWidth(), getHeight() - 40);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        const auto buttonWidth = roundToInt (getWidth() / 4);
        const auto buttonHeight = 40;
        FlexBox fb;
        fb.items = {
            FlexItem (buttonWidth, buttonHeight, openAsPopup).withMaxHeight (buttonHeight),
            FlexItem (buttonWidth, buttonHeight, openAsListBoxMenu).withMaxHeight (buttonHeight),
            FlexItem (buttonWidth, buttonHeight, openAsListBoxMenuInteractive).withMaxHeight (buttonHeight),
            FlexItem (buttonWidth, buttonHeight, openAsCallOut).withMaxHeight (buttonHeight)
        };
        fb.performLayout (getLocalBounds());
    }

    TextButton openAsPopup, openAsListBoxMenu, openAsListBoxMenuInteractive, openAsCallOut;
    PopupMenu jucePopupMenu;
    std::unique_ptr<jux::ListBoxMenu> listBoxMenu, calloutListBoxMenu;
    bool isInteractive { false };
};

//==============================================================================
MainComponent::MainComponent()
    : sidePanel ("JUX", 400, true), menuButton ("Menu", DrawableButton::ImageRaw)
{
    header.setText ("JUCE User Experience Extension", dontSendNotification);
    header.setJustificationType (Justification::centred);
    addAndMakeVisible (header);

    DrawablePath open, closed;
    open.setPath (jux::getArrowPath ({ 10.0f, 10.0f, 20.0f, 20.0f }, 1, false, Justification::centred));
    open.replaceColour (Colours::black, Colours::white);
    closed.setPath (jux::getArrowPath ({ 10.0f, 10.0f, 20.0f, 20.0f }, 2, false, Justification::centred));
    closed.replaceColour (Colours::black, Colours::white);
    menuButton.setImages (&open, nullptr, nullptr, nullptr, &closed);
    menuButton.setClickingTogglesState (true);
    menuButton.onClick = [this] { sidePanel.showOrHide (! sidePanel.isPanelShowing()); };
    addAndMakeVisible (menuButton);

    components.push_back (std::unique_ptr<Component> (new SwitchButtonDemo()));
    mainArea.addAndMakeVisible (components[enumAsInt (MenuOptions::SwitchButtonDemo)].get());
    components.push_back (std::unique_ptr<Component> (new ListBoxDemo()));
    mainArea.addChildComponent (components[enumAsInt (MenuOptions::ListBoxDemo)].get());
    components.push_back (std::unique_ptr<Component> (new ListBoxMenuDemo()));
    mainArea.addChildComponent (components[enumAsInt (MenuOptions::ListBoxMenuDemo)].get());

    auto side = std::make_unique<BurgerMenuComponent>();
    demoMenu.owner = this;
    side->setModel (&demoMenu);
    sidePanel.setTitleBarHeight (0);
    sidePanel.setContent (side.release());
    mainArea.addAndMakeVisible (sidePanel);

    addAndMakeVisible (mainArea);

    setSize (600, 800);
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    auto* switchButtonDemo = static_cast<SwitchButtonDemo*> (components[enumAsInt (MenuOptions::SwitchButtonDemo)].get());
    header.setColour (Label::ColourIds::backgroundColourId, switchButtonDemo->darkModeSwitch.getToggleState() ? Colours::darkgrey : Colours::black);

    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    {
        auto headerBounds = bounds.removeFromTop (40);
        header.setBounds (headerBounds);
        menuButton.setBounds (headerBounds.removeFromLeft (40));
    }
    mainArea.setBounds (bounds);

    for (auto& cmp : components)
    {
        cmp->setBounds (mainArea.getLocalBounds());
    }
}
