Title: Introduction
Slug: Introduction

# Introduction

libpanel is a library of widgets, developed initially for use in GNOME
Builder, to provide movable panels of widgets with GTK 4.0.

It also provides a few other independent widgets like
[class@PanelOmniBar], [class@PanelStatusbar] and [class@ThemeSelector].

A typical UI using libpanel would see in the top level content box a
[class@PanelDock]. You can also add a [class@PanelStatusbar] at the
bottom, indepedently.

## Dock

A dock is made out of 5 areas, start, end, top, bottom and center (the
areas are defined in [enum@PanelArea]). The center area is always
visible and is the main part of the user interface. The start and end
areas are vertical panes on each side of the center, in the reading
order of the locale, which would beft left and right respectively in
English. The top and bottom area are horizontal panes above and below
respectively.

Example builder for a `PanelDock`:

```xml
<object class="PanelDock" id="dock">
    <property name="reveal-end">false</property>
    <property name="vexpand">true</property>
    <child>
        <object class="PanelGrid" id="grid">
            <signal name="create-frame" handler="create_frame_cb"/>
        </object>
    </child>
    <child type="end">
        <object class="PanelPaned">
            <property name="orientation">vertical</property>
                <child>
                    <object class="PanelFrame">
                        <child>
                            <object class="PanelWidget">
                                <property name="title">Color</property>
                                <property name="tooltip">Color</property>
                                <property name="can-maximize">true</property>
                                <property name="icon-name">color-select-symbolic</property>
                                <child>
                                    <object class="GtkColorChooserWidget"></object>
                                </child>
                            </object>
                        </child>
                        <child>
                            <object class="PanelWidget">
                                <property name="title">Swatches</property>
                                <property name="icon-name">preferences-color-symbolic</property>
                                <property name="can-maximize">true</property>
                                <child>
                                    <object class="GtkScrolledWindow">
                                        <child>
                                            <object class="GtkListBox"></object>
                                        </child>
                                    </object>
                                </child>
                            </object>
                        </child>
                  </object>
              </child>
          </object>
    </child>
</object>
```

This builder fragment creates a #PanelDock with a paned on the
end area. [class@PanelPaned] is the widget representing the area. It will
contain frames.

### Frames

[class@PanelFrame] contains stacked [class@PanelWidget] that will be
managed by a switcher widget. These #PanelWidget will be the
containers for your widgets.

A panel frame will have a [class@PanelFrameSwitcher] to manage
switching between the stacked widget. It will also handle the users
moving the widget between frames.

### Showing paned

You can control the visibility (or reveal) of the various paned areas
of the dock. One easy way is to put a [class@PanelToggleButton] that
will be bound to one of the four areas, that will automatically reveal
or hide the paned.

A toggle button for the area in the left (start in the English locale):

<picture>
    <source srcset="toggle-button-dark.png" media="(prefers-color-scheme: dark)">
    <img src="toggle-button.png" alt="toggle-button">
</picture>

### Center area

The center area is the main user interface. Typically you would have
the documents displayed, and when all the paned areas are hidden, it
would remain the only visible part.

Widgets are layout in a [class@PanelGrid] that you add. You connect to
the [signal@PanelGrid::create-frame] signal to be able to create
frames as requested.

## Status bar

A [class@PanelStatusbar] can be added at the end of the window content
box. This is typically where you'd add a [class@PanelToggleButton] for
the area %PANEL_AREA_BOTTOM. The status bar widget can be used
independently from the presence of a panel dok.

## Omni bar

The omni bar is a more complex widget to provide information and
actions, and also display a progress. Typically it would be added to
the window header bar.

<picture>
    <source srcset="omni-bar-dark.png" media="(prefers-color-scheme: dark)">
    <img src="omni-bar.png" alt="omni-bar">
</picture>

The omni bar can be used independently from the presence of a panel
dock.

## Theme selector

The [class@PanelThemeSelector] is a widget that allow to easily
provide a preference for the user to choose dark or light theme for
the application.

It provides three options:

<picture>
    <source srcset="theme-selector-dark.png" media="(prefers-color-scheme: dark)">
    <img src="theme-selector.png" alt="theme-selector">
</picture>

1. follow the system theme: it will use whatever is the global user
   preference.
2. light: always use the light theme.
3. dark: always use the dark theme.

You just specify an action name to be notified. It will do the rest.
