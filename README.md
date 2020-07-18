# JUX
JUCE User Experience Extension


-----

CAUTION: __work-in-progress__ this will be rebased!

-----

Summary
-------

[JUCE](https://www.juce.com) is a powerful C++ cross-platform framework.
It's one of the most used frameworks for developing Audio Plug-Ins (VST, AAX, Audio-Unit, etc).

**Cross-platform and GUI:**

When it comes to GUI/UI, Cross-platform frameworks can be divided into 2 major types:

* Use native OS components (eg. React Native, Xamrin, etc...)
* Framework drawn components (Qt, JUCE, JavaFX/Swing, etc...)

JUCE is using the second, it makes its own UI components.

The advantage of that approach is that your UI code should be the same on all platforms.

**Mobile Platforms**

With the evolution of mobile platforms, JUCE was quickly to add iOS/iPadOS and Android support, making it possible to build the same UI to even more form-factors.

While it's really cool seeing your software 'running' on mobile. Similar to the web, not all UI translates well between desktop and mobile or touch devices.
(Eg. pop-ups - very useful on desktop, almost impossible to use on touch devices, tablets included).

**JUX**

JUX is aimed at being a complimentary library to improve JUCE UI building blocks.


What's included?
----------------

`jux::ListBox`: Improved ListBox with more capabilities. (currently custom height for each row)

`jux::ListBoxMenu`: A hybrid navigational list component so you could use same code for listbox and popup.

* Limitations: Currently similar to `juce::PopupMenu` it's very hard to update items (eg. tick/untick an item).

`jux::SwitchButton`: Very simple switch button.

License and Contribution
------------------------

JUX is MIT licensed.

(keep in-mind JUCE itself is dual-licensed and should be handled separately).

**Contribution**: Bugs, Feature-Requests, etc are welcomed in issues or as pull-requests :)

**Commercial user(s)**:
If you find it useful in a profitable commercial products. feel free to donate.
