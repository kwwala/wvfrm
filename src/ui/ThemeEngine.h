#pragma once

#include "../JuceIncludes.h"

#include <vector>

namespace wvfrm
{

struct ThemePalette
{
    juce::Colour background = juce::Colour::fromRGB(11, 14, 20);
    juce::Colour grid = juce::Colour::fromRGB(31, 38, 51);
    juce::Colour waveformPrimary = juce::Colour::fromRGB(54, 207, 201);
    juce::Colour waveformSecondary = juce::Colour::fromRGB(255, 159, 67);
    juce::Colour bandLow = juce::Colour::fromRGB(255, 59, 48);
    juce::Colour bandMid = juce::Colour::fromRGB(52, 199, 89);
    juce::Colour bandHigh = juce::Colour::fromRGB(10, 132, 255);
    float peakHistoryAlpha = 0.45f;
    juce::Colour textTimecode = juce::Colour::fromRGB(230, 237, 247);
    juce::Colour cursorReadout = juce::Colour::fromRGB(245, 247, 250);
    std::vector<std::pair<float, juce::Colour>> colorMapStops;
};

class ThemeEngine
{
public:
    ThemeEngine();

    void setThemeName(const juce::String& themeName);
    juce::String getThemeName() const;
    juce::StringArray getAvailableThemeNames() const;

    const ThemePalette& getTheme() const noexcept;

    void setHotReloadEnabled(bool enabled) noexcept;
    void pollForChanges();

private:
    static ThemePalette defaultTheme();
    static juce::File themeDirectory();
    static juce::File themeFileForName(const juce::String& name);
    static juce::String sanitizeThemeName(const juce::String& name);
    static juce::Colour parseColour(const juce::String& text, juce::Colour fallback);
    static bool parseThemeFile(const juce::File& file, ThemePalette& out);
    static void ensureThemeTemplateFiles();

    void reloadTheme(bool force);

    juce::String currentThemeName;
    juce::File currentThemeFile;
    juce::Time lastModified;
    ThemePalette activeTheme;
    bool hotReloadEnabled = true;
};

} // namespace wvfrm
