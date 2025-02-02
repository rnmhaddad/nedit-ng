
#include "Settings.h"
#include "Util/Resource.h"

#include <QSettings>
#include <QStandardPaths>
#include <QtDebug>

namespace Settings {

namespace {

bool settingsLoaded_ = false;

const QStringList DEFAULT_INCLUDE_PATHS = {
	QLatin1String("/usr/include/"),
	QLatin1String("/usr/local/include/")};

const auto DEFAULT_DELIMETERS      = QLatin1String(".,/\\`'!|@#%^&*()-=+{}[]\":;<>?");
const auto DEFAULT_BACKLIGHT_CHARS = QLatin1String("0-8,10-31,127:red;9:#dedede;32,160-255:#f0f0f0;128-159:orange");
const auto DEFAULT_TEXT_FONT       = QLatin1String("Courier New,10,-1,5,50,0,0,0,0,0");

#if defined(Q_OS_LINUX)
const auto shellCommandsResource = QLatin1String("DefaultShellCommandsLinux.txt");
#elif defined(Q_OS_FREEBSD)
const auto shellCommandsResource = QLatin1String("DefaultShellCommandsFreeBSD.txt");
#elif defined(Q_OS_UNIX)
const auto shellCommandsResource = QLatin1String("DefaultShellCommandsUnix.txt");
#elif defined(Q_OS_WIN)
const auto shellCommandsResource = QLatin1String("DefaultShellCommandsWindows.txt");
#endif

template <class T>
using IsEnum = typename std::enable_if<std::is_enum<T>::value>::type;

template <class T, class = IsEnum<T>>
T readEnum(QSettings &settings, const QString &key, const T &defaultValue = T()) {
	return from_integer<T>(settings.value(key, static_cast<int>(defaultValue)).toInt());
}

template <class T, class = IsEnum<T>>
void writeEnum(QSettings &settings, const QString &key, const T &value) {
	settings.setValue(key, static_cast<int>(value));
}

}

bool showResizeNotification;
bool appendLF;
bool autoSave;
bool autoScroll;
bool backlightChars;
bool beepOnSearchWrap;
bool globalTabNavigate;
bool highlightSyntax;
bool insertTabs;
bool iSearchLine;
bool lineNumbers;
bool matchSyntaxBased;
bool openInTab;
bool pathInWindowsMenu;
bool prefFileRead;
bool repositionDialogs;
bool retainSearchDialogs;
bool saveOldVersion;
bool searchDialogs;
bool searchWraps;
bool smartTags;
bool sortOpenPrevMenu;
bool sortTabs;
bool statisticsLine;
bool tabBar;
bool tabBarHideOne;
bool toolTips;
bool warnExit;
bool warnFileMods;
bool warnRealFileMods;
bool smartHome;
int fileVersion;
IndentStyle autoIndent;
WrapStyle autoWrap;
int emulateTabs;
SearchType searchMethod;
ShowMatchingStyle showMatching;
int tabDistance;
int textCols;
int textRows;
int wrapMargin;
QString bgMenuCommands;
QString colors[9];
QString geometry;
QString highlightPatterns;
QString languageModes;
QString macroCommands;
QString serverName;
QString shell;
QString shellCommands;
QString smartIndentInit;
QString smartIndentInitCommon;
QString fontName;
QString titleFormat;

bool autoWrapPastedText;
bool colorizeHighlightedText;
bool heavyCursor;
bool alwaysCheckRelativeTagsSpecs;
bool findReplaceUsesSelection;
bool focusOnRaise;
bool forceOSConversion;
bool honorSymlinks;
bool stickyCaseSenseButton;
bool typingHidesPointer;
bool undoModifiesSelection;
bool splitHorizontally;
int truncateLongNamesInTabs;
int autoScrollVPadding;
int maxPrevOpenFiles;
TruncSubstitution truncSubstitution;
QString backlightCharTypes;
QString tagFile;
QString wordDelimiters;
QStringList includePaths;
QFont font;

/**
 * @brief themeFile
 * @return
 */
QString themeFile() {
	static const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
	static const auto configFile   = tr("%1/%2/%3").arg(configDir, tr("nedit-ng"), tr("theme.xml"));
	return configFile;
}

/**
 * @brief configFile
 * @return
 */
QString configFile() {
	static const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
	static const auto configFile   = tr("%1/%2/%3").arg(configDir, tr("nedit-ng"), tr("config.ini"));
	return configFile;
}

/**
 * @brief historyFile
 * @return
 */
QString historyFile() {
	static const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
	static const auto configFile   = tr("%1/%2/%3").arg(configDir, tr("nedit-ng"), tr("history"));
	return configFile;
}

/**
 * @brief autoLoadMacroFile
 * @return
 */
QString autoLoadMacroFile() {
	static const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
	static const auto configFile   = tr("%1/%2/%3").arg(configDir, tr("nedit-ng"), tr("autoload.nm"));
	return configFile;
}

/**
 * @brief styleFile
 * @return
 */
QString styleFile() {
	static const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
	static const auto configFile   = tr("%1/%2/%3").arg(configDir, tr("nedit-ng"), tr("style.qss"));
	return configFile;
}

/**
 * @brief loadPreferences
 */
void loadPreferences() {

	QString filename = configFile();
	QSettings settings(filename, QSettings::IniFormat);

	fileVersion = settings.value(tr("nedit.fileVersion"), 1).toInt();

	shellCommands         = settings.value(tr("nedit.shellCommands"), loadResource(shellCommandsResource)).toString();
	macroCommands         = settings.value(tr("nedit.macroCommands"), loadResource(QLatin1String("DefaultMacroCommands.txt"))).toString();
	bgMenuCommands        = settings.value(tr("nedit.bgMenuCommands"), loadResource(QLatin1String("DefaultBackgroundMenuCommands.txt"))).toString();
	highlightPatterns     = settings.value(tr("nedit.highlightPatterns"), loadResource(QLatin1String("DefaultHighlightPatterns.txt"))).toString();
	languageModes         = settings.value(tr("nedit.languageModes"), loadResource(QLatin1String("DefaultLanguageModes.txt"))).toString();
	smartIndentInit       = settings.value(tr("nedit.smartIndentInit"), loadResource(QLatin1String("DefaultSmartIndentInit.txt"))).toString();
	smartIndentInitCommon = settings.value(tr("nedit.smartIndentInitCommon"), loadResource(QLatin1String("DefaultSmartIndentInitCommon.txt"))).toString();

	autoWrap          = readEnum(settings, tr("nedit.autoWrap"), WrapStyle::Continuous);
	autoIndent        = readEnum(settings, tr("nedit.autoIndent"), IndentStyle::Auto);
	showMatching      = readEnum(settings, tr("nedit.showMatching"), ShowMatchingStyle::Delimiter);
	searchMethod      = readEnum(settings, tr("nedit.searchMethod"), SearchType::Literal);
	truncSubstitution = readEnum(settings, tr("nedit.truncSubstitution"), TruncSubstitution::Fail);

	wrapMargin             = settings.value(tr("nedit.wrapMargin"), 0).toInt();
	autoSave               = settings.value(tr("nedit.autoSave"), true).toBool();
	openInTab              = settings.value(tr("nedit.openInTab"), true).toBool();
	saveOldVersion         = settings.value(tr("nedit.saveOldVersion"), false).toBool();
	matchSyntaxBased       = settings.value(tr("nedit.matchSyntaxBased"), true).toBool();
	highlightSyntax        = settings.value(tr("nedit.highlightSyntax"), true).toBool();
	backlightChars         = settings.value(tr("nedit.backlightChars"), false).toBool();
	backlightCharTypes     = settings.value(tr("nedit.backlightCharTypes"), DEFAULT_BACKLIGHT_CHARS).toString();
	searchDialogs          = settings.value(tr("nedit.searchDialogs"), false).toBool();
	beepOnSearchWrap       = settings.value(tr("nedit.beepOnSearchWrap"), false).toBool();
	retainSearchDialogs    = settings.value(tr("nedit.retainSearchDialogs"), false).toBool();
	searchWraps            = settings.value(tr("nedit.searchWraps"), true).toBool();
	stickyCaseSenseButton  = settings.value(tr("nedit.stickyCaseSenseButton"), true).toBool();
	repositionDialogs      = settings.value(tr("nedit.repositionDialogs"), false).toBool();
	autoScroll             = settings.value(tr("nedit.autoScroll"), false).toBool();
	autoScrollVPadding     = settings.value(tr("nedit.autoScrollVPadding"), 4).toInt();
	appendLF               = settings.value(tr("nedit.appendLF"), true).toBool();
	sortOpenPrevMenu       = settings.value(tr("nedit.sortOpenPrevMenu"), true).toBool();
	statisticsLine         = settings.value(tr("nedit.statisticsLine"), false).toBool();
	iSearchLine            = settings.value(tr("nedit.iSearchLine"), false).toBool();
	sortTabs               = settings.value(tr("nedit.sortTabs"), false).toBool();
	tabBar                 = settings.value(tr("nedit.tabBar"), true).toBool();
	tabBarHideOne          = settings.value(tr("nedit.tabBarHideOne"), true).toBool();
	toolTips               = settings.value(tr("nedit.toolTips"), true).toBool();
	globalTabNavigate      = settings.value(tr("nedit.globalTabNavigate"), false).toBool();
	lineNumbers            = settings.value(tr("nedit.lineNumbers"), false).toBool();
	pathInWindowsMenu      = settings.value(tr("nedit.pathInWindowsMenu"), true).toBool();
	warnFileMods           = settings.value(tr("nedit.warnFileMods"), true).toBool();
	warnRealFileMods       = settings.value(tr("nedit.warnRealFileMods"), true).toBool();
	warnExit               = settings.value(tr("nedit.warnExit"), true).toBool();
	showResizeNotification = settings.value(tr("nedit.showResizeNotification"), false).toBool();
	smartHome              = settings.value(tr("nedit.smartHome"), false).toBool();

	textRows                     = settings.value(tr("nedit.textRows"), 24).toInt();
	textCols                     = settings.value(tr("nedit.textCols"), 80).toInt();
	tabDistance                  = settings.value(tr("nedit.tabDistance"), 8).toInt();
	emulateTabs                  = settings.value(tr("nedit.emulateTabs"), 0).toInt();
	insertTabs                   = settings.value(tr("nedit.insertTabs"), true).toBool();
	fontName                     = settings.value(tr("nedit.textFont"), DEFAULT_TEXT_FONT).toString();
	shell                        = settings.value(tr("nedit.shell"), QLatin1String("DEFAULT")).toString();
	geometry                     = settings.value(tr("nedit.geometry"), QString()).toString();
	tagFile                      = settings.value(tr("nedit.tagFile"), QString()).toString();
	wordDelimiters               = settings.value(tr("nedit.wordDelimiters"), DEFAULT_DELIMETERS).toString();
	includePaths                 = settings.value(tr("nedit.includePaths"), DEFAULT_INCLUDE_PATHS).toStringList();
	serverName                   = settings.value(tr("nedit.serverName"), QString()).toString();
	maxPrevOpenFiles             = settings.value(tr("nedit.maxPrevOpenFiles"), 30).toInt();
	smartTags                    = settings.value(tr("nedit.smartTags"), true).toBool();
	typingHidesPointer           = settings.value(tr("nedit.typingHidesPointer"), false).toBool();
	alwaysCheckRelativeTagsSpecs = settings.value(tr("nedit.alwaysCheckRelativeTagsSpecs"), true).toBool();
	colorizeHighlightedText      = settings.value(tr("nedit.colorizeHighlightedText"), false).toBool();
	autoWrapPastedText           = settings.value(tr("nedit.autoWrapPastedText"), false).toBool();
	heavyCursor                  = settings.value(tr("nedit.heavyCursor"), false).toBool();
	prefFileRead                 = settings.value(tr("nedit.prefFileRead"), false).toBool();
	findReplaceUsesSelection     = settings.value(tr("nedit.findReplaceUsesSelection"), false).toBool();
	titleFormat                  = settings.value(tr("nedit.titleFormat"), QLatin1String("{%c} [%s] %f (%S) - %d")).toString();
	undoModifiesSelection        = settings.value(tr("nedit.undoModifiesSelection"), true).toBool();
	splitHorizontally            = settings.value(tr("nedit.splitHorizontally"), false).toBool();
	truncateLongNamesInTabs      = settings.value(tr("nedit.truncateLongNamesInTabs"), 0).toInt();
	focusOnRaise                 = settings.value(tr("nedit.focusOnRaise"), false).toBool();
	forceOSConversion            = settings.value(tr("nedit.forceOSConversion"), true).toBool();
	honorSymlinks                = settings.value(tr("nedit.honorSymlinks"), true).toBool();

	if (includePaths.isEmpty()) {
		includePaths.append(DEFAULT_INCLUDE_PATHS[0]);
	}

	settingsLoaded_ = true;
}

/**
 * @brief importSettings
 * @param filename
 */
void importSettings(const QString &filename) {
	if (!settingsLoaded_) {
		qWarning("NEdit: Warning, importing while no previous settings loaded!");
	}

	QSettings settings(filename, QSettings::IniFormat);

	fileVersion           = settings.value(tr("nedit.fileVersion"), fileVersion).toInt();
	shellCommands         = settings.value(tr("nedit.shellCommands"), shellCommands).toString();
	macroCommands         = settings.value(tr("nedit.macroCommands"), macroCommands).toString();
	bgMenuCommands        = settings.value(tr("nedit.bgMenuCommands"), bgMenuCommands).toString();
	highlightPatterns     = settings.value(tr("nedit.highlightPatterns"), highlightPatterns).toString();
	languageModes         = settings.value(tr("nedit.languageModes"), languageModes).toString();
	smartIndentInit       = settings.value(tr("nedit.smartIndentInit"), smartIndentInit).toString();
	smartIndentInitCommon = settings.value(tr("nedit.smartIndentInitCommon"), smartIndentInitCommon).toString();

	autoWrap          = readEnum(settings, tr("nedit.autoWrap"), autoWrap);
	autoIndent        = readEnum(settings, tr("nedit.autoIndent"), autoIndent);
	showMatching      = readEnum(settings, tr("nedit.showMatching"), showMatching);
	searchMethod      = readEnum(settings, tr("nedit.searchMethod"), searchMethod);
	truncSubstitution = readEnum(settings, tr("nedit.truncSubstitution"), truncSubstitution);

	wrapMargin            = settings.value(tr("nedit.wrapMargin"), wrapMargin).toInt();
	autoSave              = settings.value(tr("nedit.autoSave"), autoSave).toBool();
	openInTab             = settings.value(tr("nedit.openInTab"), openInTab).toBool();
	saveOldVersion        = settings.value(tr("nedit.saveOldVersion"), saveOldVersion).toBool();
	matchSyntaxBased      = settings.value(tr("nedit.matchSyntaxBased"), matchSyntaxBased).toBool();
	highlightSyntax       = settings.value(tr("nedit.highlightSyntax"), highlightSyntax).toBool();
	backlightChars        = settings.value(tr("nedit.backlightChars"), backlightChars).toBool();
	backlightCharTypes    = settings.value(tr("nedit.backlightCharTypes"), backlightCharTypes).toString();
	searchDialogs         = settings.value(tr("nedit.searchDialogs"), searchDialogs).toBool();
	beepOnSearchWrap      = settings.value(tr("nedit.beepOnSearchWrap"), beepOnSearchWrap).toBool();
	retainSearchDialogs   = settings.value(tr("nedit.retainSearchDialogs"), retainSearchDialogs).toBool();
	searchWraps           = settings.value(tr("nedit.searchWraps"), searchWraps).toBool();
	stickyCaseSenseButton = settings.value(tr("nedit.stickyCaseSenseButton"), stickyCaseSenseButton).toBool();
	repositionDialogs     = settings.value(tr("nedit.repositionDialogs"), repositionDialogs).toBool();
	autoScroll            = settings.value(tr("nedit.autoScroll"), autoScroll).toBool();
	autoScrollVPadding    = settings.value(tr("nedit.autoScrollVPadding"), autoScrollVPadding).toInt();
	appendLF              = settings.value(tr("nedit.appendLF"), appendLF).toBool();
	sortOpenPrevMenu      = settings.value(tr("nedit.sortOpenPrevMenu"), sortOpenPrevMenu).toBool();
	statisticsLine        = settings.value(tr("nedit.statisticsLine"), statisticsLine).toBool();
	iSearchLine           = settings.value(tr("nedit.iSearchLine"), iSearchLine).toBool();
	sortTabs              = settings.value(tr("nedit.sortTabs"), sortTabs).toBool();
	tabBar                = settings.value(tr("nedit.tabBar"), tabBar).toBool();
	tabBarHideOne         = settings.value(tr("nedit.tabBarHideOne"), tabBarHideOne).toBool();
	toolTips              = settings.value(tr("nedit.toolTips"), toolTips).toBool();
	globalTabNavigate     = settings.value(tr("nedit.globalTabNavigate"), globalTabNavigate).toBool();
	lineNumbers           = settings.value(tr("nedit.lineNumbers"), lineNumbers).toBool();
	pathInWindowsMenu     = settings.value(tr("nedit.pathInWindowsMenu"), pathInWindowsMenu).toBool();
	warnFileMods          = settings.value(tr("nedit.warnFileMods"), warnFileMods).toBool();
	warnRealFileMods      = settings.value(tr("nedit.warnRealFileMods"), warnRealFileMods).toBool();
	warnExit              = settings.value(tr("nedit.warnExit"), warnExit).toBool();
	smartHome             = settings.value(tr("nedit.smartHome"), smartHome).toBool();

	textRows                     = settings.value(tr("nedit.textRows"), textRows).toInt();
	textCols                     = settings.value(tr("nedit.textCols"), textCols).toInt();
	tabDistance                  = settings.value(tr("nedit.tabDistance"), tabDistance).toInt();
	emulateTabs                  = settings.value(tr("nedit.emulateTabs"), emulateTabs).toInt();
	insertTabs                   = settings.value(tr("nedit.insertTabs"), insertTabs).toBool();
	fontName                     = settings.value(tr("nedit.textFont"), fontName).toString();
	shell                        = settings.value(tr("nedit.shell"), shell).toString();
	geometry                     = settings.value(tr("nedit.geometry"), geometry).toString();
	tagFile                      = settings.value(tr("nedit.tagFile"), tagFile).toString();
	wordDelimiters               = settings.value(tr("nedit.wordDelimiters"), wordDelimiters).toString();
	includePaths                 = settings.value(tr("nedit.includePaths"), includePaths).toStringList();
	serverName                   = settings.value(tr("nedit.serverName"), serverName).toString();
	maxPrevOpenFiles             = settings.value(tr("nedit.maxPrevOpenFiles"), maxPrevOpenFiles).toInt();
	smartTags                    = settings.value(tr("nedit.smartTags"), smartTags).toBool();
	typingHidesPointer           = settings.value(tr("nedit.typingHidesPointer"), typingHidesPointer).toBool();
	alwaysCheckRelativeTagsSpecs = settings.value(tr("nedit.alwaysCheckRelativeTagsSpecs"), alwaysCheckRelativeTagsSpecs).toBool();
	heavyCursor                  = settings.value(tr("nedit.heavyCursor"), heavyCursor).toBool();
	autoWrapPastedText           = settings.value(tr("nedit.autoWrapPastedText"), autoWrapPastedText).toBool();
	colorizeHighlightedText      = settings.value(tr("nedit.colorizeHighlightedText"), colorizeHighlightedText).toBool();
	prefFileRead                 = settings.value(tr("nedit.prefFileRead"), prefFileRead).toBool();
	findReplaceUsesSelection     = settings.value(tr("nedit.findReplaceUsesSelection"), findReplaceUsesSelection).toBool();
	titleFormat                  = settings.value(tr("nedit.titleFormat"), titleFormat).toString();
	undoModifiesSelection        = settings.value(tr("nedit.undoModifiesSelection"), undoModifiesSelection).toBool();
	splitHorizontally            = settings.value(tr("nedit.splitHorizontally"), splitHorizontally).toBool();
	truncateLongNamesInTabs      = settings.value(tr("nedit.truncateLongNamesInTabs"), truncateLongNamesInTabs).toInt();
	focusOnRaise                 = settings.value(tr("nedit.focusOnRaise"), focusOnRaise).toBool();
	forceOSConversion            = settings.value(tr("nedit.forceOSConversion"), forceOSConversion).toBool();
	honorSymlinks                = settings.value(tr("nedit.honorSymlinks"), honorSymlinks).toBool();
}

/**
 * @brief savePreferences
 * @return
 */
bool savePreferences() {
	QString filename = configFile();
	QSettings settings(filename, QSettings::IniFormat);

	settings.setValue(tr("nedit.fileVersion"), fileVersion);
	settings.setValue(tr("nedit.shellCommands"), shellCommands);
	settings.setValue(tr("nedit.macroCommands"), macroCommands);
	settings.setValue(tr("nedit.bgMenuCommands"), bgMenuCommands);
	settings.setValue(tr("nedit.highlightPatterns"), highlightPatterns);
	settings.setValue(tr("nedit.languageModes"), languageModes);
	settings.setValue(tr("nedit.smartIndentInit"), smartIndentInit);
	settings.setValue(tr("nedit.smartIndentInitCommon"), smartIndentInitCommon);

	writeEnum(settings, tr("nedit.autoWrap"), autoWrap);
	writeEnum(settings, tr("nedit.autoIndent"), autoIndent);
	writeEnum(settings, tr("nedit.showMatching"), showMatching);
	writeEnum(settings, tr("nedit.searchMethod"), searchMethod);
	writeEnum(settings, tr("nedit.truncSubstitution"), truncSubstitution);

	settings.setValue(tr("nedit.wrapMargin"), wrapMargin);
	settings.setValue(tr("nedit.autoSave"), autoSave);
	settings.setValue(tr("nedit.openInTab"), openInTab);
	settings.setValue(tr("nedit.saveOldVersion"), saveOldVersion);
	settings.setValue(tr("nedit.matchSyntaxBased"), matchSyntaxBased);
	settings.setValue(tr("nedit.highlightSyntax"), highlightSyntax);
	settings.setValue(tr("nedit.backlightChars"), backlightChars);
	settings.setValue(tr("nedit.backlightCharTypes"), backlightCharTypes);
	settings.setValue(tr("nedit.searchDialogs"), searchDialogs);
	settings.setValue(tr("nedit.beepOnSearchWrap"), beepOnSearchWrap);
	settings.setValue(tr("nedit.retainSearchDialogs"), retainSearchDialogs);
	settings.setValue(tr("nedit.searchWraps"), searchWraps);
	settings.setValue(tr("nedit.stickyCaseSenseButton"), stickyCaseSenseButton);
	settings.setValue(tr("nedit.repositionDialogs"), repositionDialogs);
	settings.setValue(tr("nedit.autoScroll"), autoScroll);
	settings.setValue(tr("nedit.autoScrollVPadding"), autoScrollVPadding);
	settings.setValue(tr("nedit.appendLF"), appendLF);
	settings.setValue(tr("nedit.sortOpenPrevMenu"), sortOpenPrevMenu);
	settings.setValue(tr("nedit.statisticsLine"), statisticsLine);
	settings.setValue(tr("nedit.iSearchLine"), iSearchLine);
	settings.setValue(tr("nedit.sortTabs"), sortTabs);
	settings.setValue(tr("nedit.tabBar"), tabBar);
	settings.setValue(tr("nedit.tabBarHideOne"), tabBarHideOne);
	settings.setValue(tr("nedit.toolTips"), toolTips);
	settings.setValue(tr("nedit.globalTabNavigate"), globalTabNavigate);
	settings.setValue(tr("nedit.lineNumbers"), lineNumbers);
	settings.setValue(tr("nedit.pathInWindowsMenu"), pathInWindowsMenu);
	settings.setValue(tr("nedit.warnFileMods"), warnFileMods);
	settings.setValue(tr("nedit.warnRealFileMods"), warnRealFileMods);
	settings.setValue(tr("nedit.warnExit"), warnExit);
	settings.setValue(tr("nedit.smartHome"), smartHome);

	settings.setValue(tr("nedit.textRows"), textRows);
	settings.setValue(tr("nedit.textCols"), textCols);
	settings.setValue(tr("nedit.tabDistance"), tabDistance);
	settings.setValue(tr("nedit.emulateTabs"), emulateTabs);
	settings.setValue(tr("nedit.insertTabs"), insertTabs);
	settings.setValue(tr("nedit.textFont"), fontName);
	settings.setValue(tr("nedit.shell"), shell);
	settings.setValue(tr("nedit.geometry"), geometry);
	settings.setValue(tr("nedit.tagFile"), tagFile);
	settings.setValue(tr("nedit.wordDelimiters"), wordDelimiters);
	settings.setValue(tr("nedit.includePaths"), includePaths);
	settings.setValue(tr("nedit.serverName"), serverName);
	settings.setValue(tr("nedit.maxPrevOpenFiles"), maxPrevOpenFiles);
	settings.setValue(tr("nedit.smartTags"), smartTags);
	settings.setValue(tr("nedit.typingHidesPointer"), typingHidesPointer);
	settings.setValue(tr("nedit.autoWrapPastedText"), autoWrapPastedText);
	settings.setValue(tr("nedit.heavyCursor"), heavyCursor);
	settings.setValue(tr("nedit.alwaysCheckRelativeTagsSpecs"), alwaysCheckRelativeTagsSpecs);
	settings.setValue(tr("nedit.colorizeHighlightedText"), colorizeHighlightedText);
	settings.setValue(tr("nedit.prefFileRead"), prefFileRead);
	settings.setValue(tr("nedit.findReplaceUsesSelection"), findReplaceUsesSelection);
	settings.setValue(tr("nedit.titleFormat"), titleFormat);
	settings.setValue(tr("nedit.undoModifiesSelection"), undoModifiesSelection);
	settings.setValue(tr("nedit.splitHorizontally"), splitHorizontally);
	settings.setValue(tr("nedit.truncateLongNamesInTabs"), truncateLongNamesInTabs);
	settings.setValue(tr("nedit.focusOnRaise"), focusOnRaise);
	settings.setValue(tr("nedit.forceOSConversion"), forceOSConversion);
	settings.setValue(tr("nedit.honorSymlinks"), honorSymlinks);

	settings.sync();
	return settings.status() == QSettings::NoError;
}

}
