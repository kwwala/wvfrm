#include "ThemeEngine.h"

#include <algorithm>

namespace wvfrm
{
namespace
{
constexpr auto defaultThemeName = "Default Waveform";

const char* templateThemeText = R"ini(; _TEMPLATE.ini - Waveform module theme
[meta]
name=Default Waveform
author=wvfrm
version=1.0

[module.waveform]
background=#0B0E14
grid=#1F2633
waveform_primary=#36CFC9
waveform_secondary=#FF9F43
band_low=#FF3B30
band_mid=#34C759
band_high=#0A84FF
peak_history_alpha=0.45
text_timecode=#E6EDF7
cursor_readout=#F5F7FA

[module.waveform.colormap]
stop0=0.00,#1D2B53
stop1=0.35,#7E2553
stop2=0.70,#FF004D
stop3=1.00,#FFEC27
)ini";
}

ThemeEngine::ThemeEngine()
    : currentThemeName(defaultThemeName),
      activeTheme(defaultTheme())
{
    ensureThemeTemplateFiles();
    reloadTheme(true);
}

void ThemeEngine::setThemeName(const juce::String& themeName)
{
    const auto sanitized = sanitizeThemeName(themeName);
    if (sanitized == currentThemeName)
        return;

    currentThemeName = sanitized;
    reloadTheme(true);
}

juce::String ThemeEngine::getThemeName() const
{
    return currentThemeName;
}

juce::StringArray ThemeEngine::getAvailableThemeNames() const
{
    juce::StringArray result;
    auto folder = themeDirectory();

    if (! folder.isDirectory())
        return result;

    juce::Array<juce::File> files = folder.findChildFiles(juce::File::findFiles, false, "*.ini");
    for (const auto& file : files)
    {
        const auto name = file.getFileNameWithoutExtension();
        if (! name.startsWith("_"))
            result.add(name);
    }

    result.sort(true);

    if (! result.contains(defaultThemeName))
        result.insert(0, defaultThemeName);

    return result;
}

const ThemePalette& ThemeEngine::getTheme() const noexcept
{
    return activeTheme;
}

void ThemeEngine::setHotReloadEnabled(bool enabled) noexcept
{
    hotReloadEnabled = enabled;
}

void ThemeEngine::pollForChanges()
{
    if (! hotReloadEnabled || currentThemeFile == juce::File() || ! currentThemeFile.existsAsFile())
        return;

    const auto modified = currentThemeFile.getLastModificationTime();
    if (modified != lastModified)
        reloadTheme(true);
}

ThemePalette ThemeEngine::defaultTheme()
{
    ThemePalette palette;
    palette.colorMapStops = {
        { 0.0f, juce::Colour::fromRGB(29, 43, 83) },
        { 0.35f, juce::Colour::fromRGB(126, 37, 83) },
        { 0.7f, juce::Colour::fromRGB(255, 0, 77) },
        { 1.0f, juce::Colour::fromRGB(255, 236, 39) }
    };
    return palette;
}

juce::File ThemeEngine::themeDirectory()
{
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto directory = base.getChildFile("wvfrm").getChildFile("themes");
    directory.createDirectory();
    return directory;
}

juce::File ThemeEngine::themeFileForName(const juce::String& name)
{
    auto sanitized = sanitizeThemeName(name);
    return themeDirectory().getChildFile(sanitized + ".ini");
}

juce::String ThemeEngine::sanitizeThemeName(const juce::String& name)
{
    auto trimmed = name.trim();
    if (trimmed.isEmpty())
        trimmed = defaultThemeName;

    auto cleaned = trimmed.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-");
    if (cleaned.isEmpty())
        cleaned = defaultThemeName;

    return cleaned;
}

juce::Colour ThemeEngine::parseColour(const juce::String& text, juce::Colour fallback)
{
    auto raw = text.trim();
    if (raw.startsWithChar('#'))
        raw = raw.substring(1);

    if (raw.length() != 6 && raw.length() != 8)
        return fallback;

    const auto value = static_cast<uint32_t>(raw.getHexValue32());

    if (raw.length() == 6)
        return juce::Colour::fromRGB(static_cast<uint8_t>((value >> 16) & 0xff),
                                     static_cast<uint8_t>((value >> 8) & 0xff),
                                     static_cast<uint8_t>(value & 0xff));

    return juce::Colour(static_cast<uint8_t>((value >> 24) & 0xff),
                        static_cast<uint8_t>((value >> 16) & 0xff),
                        static_cast<uint8_t>((value >> 8) & 0xff),
                        static_cast<uint8_t>(value & 0xff));
}

bool ThemeEngine::parseThemeFile(const juce::File& file, ThemePalette& out)
{
    if (! file.existsAsFile())
        return false;

    auto parsed = defaultTheme();
    auto lines = juce::StringArray::fromLines(file.loadFileAsString());
    juce::String currentSection;
    std::vector<std::pair<float, juce::Colour>> mapStops;

    for (auto line : lines)
    {
        line = line.trim();
        if (line.isEmpty() || line.startsWithChar(';') || line.startsWithChar('#'))
            continue;

        if (line.startsWithChar('[') && line.endsWithChar(']'))
        {
            currentSection = line.substring(1, line.length() - 1).trim().toLowerCase();
            continue;
        }

        const auto eq = line.indexOfChar('=');
        if (eq <= 0)
            continue;

        const auto key = line.substring(0, eq).trim().toLowerCase();
        const auto value = line.substring(eq + 1).trim();

        if (currentSection == "module.waveform")
        {
            if (key == "background") parsed.background = parseColour(value, parsed.background);
            else if (key == "grid") parsed.grid = parseColour(value, parsed.grid);
            else if (key == "waveform_primary") parsed.waveformPrimary = parseColour(value, parsed.waveformPrimary);
            else if (key == "waveform_secondary") parsed.waveformSecondary = parseColour(value, parsed.waveformSecondary);
            else if (key == "band_low") parsed.bandLow = parseColour(value, parsed.bandLow);
            else if (key == "band_mid") parsed.bandMid = parseColour(value, parsed.bandMid);
            else if (key == "band_high") parsed.bandHigh = parseColour(value, parsed.bandHigh);
            else if (key == "text_timecode") parsed.textTimecode = parseColour(value, parsed.textTimecode);
            else if (key == "cursor_readout") parsed.cursorReadout = parseColour(value, parsed.cursorReadout);
            else if (key == "peak_history_alpha")
                parsed.peakHistoryAlpha = juce::jlimit(0.0f, 1.0f, value.getFloatValue());
        }
        else if (currentSection == "module.waveform.colormap" && key.startsWith("stop"))
        {
            auto parts = juce::StringArray::fromTokens(value, ",", "");
            parts.trim();
            parts.removeEmptyStrings();

            if (parts.size() == 2)
            {
                const auto position = juce::jlimit(0.0f, 1.0f, parts[0].getFloatValue());
                const auto color = parseColour(parts[1], parsed.waveformPrimary);
                mapStops.emplace_back(position, color);
            }
        }
    }

    if (! mapStops.empty())
    {
        std::sort(mapStops.begin(),
                  mapStops.end(),
                  [] (const auto& a, const auto& b) { return a.first < b.first; });
        parsed.colorMapStops = mapStops;
    }

    if (parsed.colorMapStops.empty())
        parsed.colorMapStops = defaultTheme().colorMapStops;

    out = parsed;
    return true;
}

void ThemeEngine::ensureThemeTemplateFiles()
{
    auto folder = themeDirectory();
    auto templateFile = folder.getChildFile("_TEMPLATE.ini");
    if (! templateFile.existsAsFile())
        templateFile.replaceWithText(templateThemeText);

    auto defaultFile = folder.getChildFile("Default Waveform.ini");
    if (! defaultFile.existsAsFile())
        defaultFile.replaceWithText(templateThemeText);
}

void ThemeEngine::reloadTheme(bool force)
{
    auto file = themeFileForName(currentThemeName);
    if (! file.existsAsFile())
        file = themeFileForName(defaultThemeName);

    if (! force && file == currentThemeFile)
        return;

    ThemePalette parsed;
    if (! parseThemeFile(file, parsed))
        parsed = defaultTheme();

    currentThemeFile = file;
    activeTheme = parsed;
    lastModified = file.existsAsFile() ? file.getLastModificationTime() : juce::Time();
}

} // namespace wvfrm
