
#include "SmartIndent.h"
#include "DialogSmartIndent.h"
#include "DialogSmartIndentCommon.h"
#include "SmartIndentEntry.h"
#include "Preferences.h"
#include "shift.h"
#include "Util/Input.h"
#include "Util/Resource.h"
#include "Util/Raise.h"

#include <QMessageBox>
#include <QPointer>
#include <QtDebug>
#include <climits>
#include <cstdio>
#include <cstring>

namespace SmartIndent {

QPointer<DialogSmartIndent>   SmartIndentDialog;
std::vector<SmartIndentEntry> SmartIndentSpecs;
QString                       CommonMacros;

namespace {

const auto MacroEndBoundary = QLatin1String("--End-of-Macro--");

// TODO(eteran): 2.0, what would be the best way to move this structure to a resource file and have it be more maintainable?
const SmartIndentEntry DefaultIndentSpecs[] = {
	{
		QLatin1String("C")
		,
		QLatin1String("# C Macros and tuning parameters are shared with C++, and are declared\n"
					  "# in the common section.  Press Common / Shared Initialization above.\n")
		,
		QLatin1String("return cFindSmartIndentDist($1)\n")
		,
		QLatin1String("if ($2 == \"}\" || $2 == \"{\" || $2 == \"#\")\n"
					  "    cBraceOrPound($1, $2)\n")
	},{
		QLatin1String("C++")
		,
		QLatin1String("# C++ Macros and tuning parameters are shared with C, and are declared\n"
					  "# in the common section.  Press Common / Shared Initialization above.\n")
		,
		QLatin1String("return cFindSmartIndentDist($1)\n")
		,
		QLatin1String("if ($2 == \"}\" || $2 == \"{\" || $2 == \"#\")\n"
					  "    cBraceOrPound($1, $2)\n")
	},{
		QLatin1String("Python")
		,
		QLatin1String("# Number of characters in a normal indent level.  May be a number, or the\n"
					  "# string \"default\", meaning, guess the value from the current tab settings.\n"
					  "$pyIndentDist = \"default\"\n")
		,
		QLatin1String("if (get_range($1-1, $1) != \":\")\n"
					  "    return -1\n"
					  "return measureIndent($1) + defaultIndent($pyIndentDist)\n")
		,
		QLatin1String("")
	},{
		QLatin1String("Matlab")
		,
		QLatin1String("# Number of spaces to indent \"case\" statements\n"
					  "$caseDepth = 2\n"
					  "define matlabNewlineMacro\n"
					  "{\n"
					  "   if (!$em_tab_dist)\n"
					  "        tabsize = $tab_dist\n"
					  "   else\n"
					  " 	   tabsize = $em_tab_dist\n"
					  "   startLine = startOfLine($1)\n"
					  "   indentLevel = measureIndent($1)\n"
					  "\n"
					  "   # If this line is continued on next, return default:\n"
					  "   lastLine = get_range(startLine, $1)\n"
					  "   if (search_string(lastLine, \"...\", 0) != -1) {\n"
					  " 	 if ($n_args == 2)\n"
					  " 		return matlabNewlineMacro(startLine - 1, 1)\n"
					  " 	 else {\n"
					  " 		return -1\n"
					  " 	 }\n"
					  "   }\n"
					  "\n"
					  "   # Correct the indentLevel if this was a continued line.\n"
					  "   while (startLine > 1)\n"
					  "   {\n"
					  " 	 endLine = startLine - 1\n"
					  " 	 startLine = startOfLine(endLine)\n"
					  " 	 lastLine = get_range(startLine, endLine)\n"
					  " 	 # No \"...\" means we've found the root\n"
					  " 	 if (search_string(lastLine, \"...\", 0) == -1) {\n"
					  " 		startLine = endLine + 1\n"
					  " 		break\n"
					  " 	 }\n"
					  "   }\n"
					  "   indentLevel = measureIndent(startLine)\n"
					  "\n"
					  "   # Get the first word of the indentLevel line\n"
					  "   FWend = search(\">|\\n\", startLine + indentLevel, \"regex\")\n"
					  "   # This search fails on EOF\n"
					  "   if (FWend == -1)\n"
					  " 	 FWend = $1\n"
					  "\n"
					  "   firstWord = get_range(startLine + indentLevel, FWend)\n"
					  "\n"
					  "   # How shall we change the indent level based on the first word?\n"
					  "   if (search_string(firstWord, \\\n"
					  " 		\"<for>|<function>|<if>|<switch>|<try>|<while>\", 0, \"regex\") == 0) {\n"
					  " 	 return indentLevel + tabsize\n"
					  "   }\n"
					  "   else if ((firstWord == \"end\") || (search_string(firstWord, \\\n"
					  " 		   \"<case>|<catch>|<else>|<elseif>|<otherwise>\", 0, \"regex\") == 0)) {\n"
					  " 	 # Get the last indent level \n"
					  " 	 if (startLine > 0) # avoid infinite loop\n"
					  "    last_indent = matlabNewlineMacro(startLine - 1, 1)\n"
					  " 	 else\n"
					  " 		last_indent = indentLevel\n"
					  "\n"
					  " 	 # Re-indent this line\n"
					  " 	 if ($n_args == 1) {\n"
					  " 		if (firstWord == \"case\" || firstWord == \"otherwise\")\n"
					  " 		   replace_range(startLine, startLine + indentLevel, \\\n"
					  " 						 makeIndentString(last_indent - tabsize + $caseDepth))\n"
					  " 		else\n"
					  " 		   replace_range(startLine, startLine + indentLevel, \\\n"
					  " 						 makeIndentString(last_indent - tabsize))\n"
					  " 	 }\n"
					  "\n"
					  " 	 if (firstWord == \"end\") {\n"
					  " 		return max(last_indent - tabsize, 0)\n"
					  " 	 }\n"
					  " 	 else {\n"
					  " 		return last_indent\n"
					  " 	 }\n"
					  "   } \n"
					  "   else {\n"
					  " 	 return indentLevel\n"
					  "   }\n"
					  "}\n")
		,
		QLatin1String("return matlabNewlineMacro($1)\n")
		,
		QLatin1String("")
	}
};

/**
 * Read a macro (arbitrary text terminated by the macro end boundary string)
 * trim off added tabs and return the string. Returns QString() if the macro
 * end boundary string is not found.
 *
 * @brief readSmartIndentMacro
 * @param in
 * @return
 */
QString readSmartIndentMacro(Input &in) {
	// Strip leading newline
	if (*in == QLatin1Char('\n')) {
		++in;
	}

	// Find the end of the macro
	int macroEnd = in.find(MacroEndBoundary);
	if(macroEnd == -1) {
		return QString();
	}

	// Copy the macro
	QString macroStr = in.mid(macroEnd - in.index());

	// Remove leading tabs added by writer routine
	in += macroEnd - in.index();
	in += MacroEndBoundary.size();

	return ShiftText(macroStr, ShiftDirection::Left, /*tabsAllowed=*/true, /*tabDist=*/8, /*nChars=*/8);
}

/*
** Insert macro text "macro" into buffer "buf" shifted right by 8 characters
** (so it looks nice in the .nedit file), and terminated with a macro-end-
** boundary string.
*/
void insertShiftedMacro(QTextStream &ts, const QString &macro) {

	if (!macro.isNull()) {
		ts << ShiftText(macro, ShiftDirection::Right, /*tabsAllowed=*/true, /*tabDist=*/8, /*nChars*/8);
	}

	ts << QLatin1String("\t");
	ts << MacroEndBoundary;
	ts << QLatin1String("\n");
}

bool isDefaultIndentSpec(const SmartIndentEntry *indentSpec) {

	auto it = std::find_if(std::begin(DefaultIndentSpecs), std::end(DefaultIndentSpecs), [indentSpec](const SmartIndentEntry &entry) {
		return (*indentSpec == entry);
	});

	return it != std::end(DefaultIndentSpecs);
}

bool loadDefaultIndentSpec(const QString &lmName) {

	auto it = std::find_if(std::begin(DefaultIndentSpecs), std::end(DefaultIndentSpecs), [&lmName](const SmartIndentEntry &entry) {
		return entry.languageMode == lmName;
	});

	if(it != std::end(DefaultIndentSpecs)) {
		SmartIndentSpecs.push_back(*it);
		return true;
	}

	return false;
}

}

/**
 * @brief defaultCommonMacros
 * @return
 */
QByteArray defaultCommonMacros() {
	static QByteArray defaultMacros = loadResource(QLatin1String("DefaultCommonMacros.txt"));
	return defaultMacros;
}

/*
** Returns true if there are smart indent macros for a named language
*/
bool SmartIndentMacrosAvailable(const QString &languageModeName) {
	return findIndentSpec(languageModeName) != nullptr;
}

/**
 * @brief LoadSmartIndentStringEx
 * @param string
 * @return
 */
bool LoadSmartIndentString(const QString &string) {

	Input in(&string);
	QString errMsg;

	struct ParseError {
		QString message;
	};

	try {
		for (;;) {
			SmartIndentEntry is;

			// skip over blank space
			in.skipWhitespaceNL();

			// finished
			if (in.atEnd()) {
				return true;
			}

			// read language mode name
			is.languageMode = Preferences::ReadSymbolicField(in);
			if (is.languageMode.isNull()) {
				Raise<ParseError>(tr("language mode name required"));
			}

			if (!Preferences::SkipDelimiter(in, &errMsg)) {
				Raise<ParseError>(errMsg);
			}

			/* look for "Default" keyword, and if it's there, return the default
			   smart indent macros */
			if (in.match(QLatin1String("Default"))) {
				if (!loadDefaultIndentSpec(is.languageMode)) {
					Raise<ParseError>(tr("no default smart indent macros"));
				}
				continue;
			}

			/* read the initialization macro (arbitrary text terminated by the
			   macro end boundary string) */
			is.initMacro = readSmartIndentMacro(in);
			if(is.initMacro.isNull()) {
				Raise<ParseError>(tr("no end boundary to initialization macro"));
			}

			// read the newline macro
			is.newlineMacro = readSmartIndentMacro(in);
			if(is.newlineMacro.isNull()) {
				Raise<ParseError>(tr("no end boundary to newline macro"));
			}

			// read the modify macro
			is.modMacro = readSmartIndentMacro(in);
			if(is.modMacro.isNull()) {
				Raise<ParseError>(tr("no end boundary to modify macro"));
			}

			// if there's no mod macro, make it null so it won't be executed
			if (is.modMacro.isEmpty()) {
				is.modMacro = QString();
			}

			auto it = std::find_if(SmartIndentSpecs.begin(), SmartIndentSpecs.end(), [&is](const SmartIndentEntry &entry) {
				return entry.languageMode == is.languageMode;
			});

			if(it == SmartIndentSpecs.end()) {
				SmartIndentSpecs.push_back(is);
			} else {
				*it = is;
			}
		}
	} catch (const ParseError &e) {
		return Preferences::reportError(
					nullptr,
					*in.string(),
					in.index(),
					tr("smart indent specification"),
					e.message);
	}
}

bool LoadSmartIndentCommonString(const QString &string) {

	Input in(&string);

	// If called from -import, can replace existing ones

	// skip over blank space
	in.skipWhitespaceNL();

	/* look for "Default" keyword, and if it's there, return the default
	   smart common macro */
	if (in.match(QLatin1String("Default"))) {
		QByteArray defaults = defaultCommonMacros();
		CommonMacros = QString::fromLatin1(defaults);
		return true;
	}

	// Remove leading tabs added by writer routine
	CommonMacros = ShiftText(in.mid(), ShiftDirection::Left, /*tabsAllowed=*/true, /*tabDist=*/8, /*nChars*/8);
	return true;
}


QString WriteSmartIndentStringEx() {

	QString s;
	QTextStream ts(&s);

	for(const SmartIndentEntry &sis : SmartIndentSpecs) {

		ts << QLatin1String("\t")
		   << sis.languageMode
		   << QLatin1String(":");

		if (isDefaultIndentSpec(&sis)) {
			ts << QLatin1String("Default\n");
		} else {
			insertShiftedMacro(ts, sis.initMacro);
			insertShiftedMacro(ts, sis.newlineMacro);
			insertShiftedMacro(ts, sis.modMacro);
		}
	}

	// Get the output string, and lop off the trailing newline
	if(!s.isEmpty()) {
		s.chop(1);
	}

	// Protect newlines and backslashes from translation by the resource reader
	return s;
}

QString WriteSmartIndentCommonStringEx() {

	QByteArray defaults = defaultCommonMacros();
	if (CommonMacros == QString::fromLatin1(defaults)) {
		return QLatin1String("Default");
	}

	if(CommonMacros.isNull()) {
		return QLatin1String("");
	}

	// Shift the macro over by a tab to keep .nedit file bright and clean
	QString outStr = ShiftText(CommonMacros, ShiftDirection::Right, /*tabsAllowed=*/true, /*tabDist=*/8, /*nChars*/8);

	/* Protect newlines and backslashes from translation by the resource
	   reader */

	// If there's a trailing escaped newline, remove it
	if(outStr.endsWith(QLatin1String("\\\n"))) {
		outStr.chop(2);
	}

	return outStr;
}

const SmartIndentEntry *findDefaultIndentSpec(const QString &name) {

	if(name.isNull()) {
		return nullptr;
	}

	auto it = std::find_if(std::begin(DefaultIndentSpecs), std::end(DefaultIndentSpecs), [&name](const SmartIndentEntry &entry) {
		return entry.languageMode == name;
	});

	if(it != std::end(DefaultIndentSpecs)) {
		return &*it;
	}

	return nullptr;
}

const SmartIndentEntry *findIndentSpec(const QString &name) {

	if(name.isNull()) {
		return nullptr;
	}

	auto it = std::find_if(std::begin(SmartIndentSpecs), std::end(SmartIndentSpecs), [&name](const SmartIndentEntry &entry) {
		return entry.languageMode == name;
	});

	if(it != std::end(SmartIndentSpecs)) {
		return &*it;
	}

	return nullptr;
}

/*
** Returns true if there are smart indent macros, or potential macros
** not yet committed in the smart indent dialog for a language mode,
*/
bool LMHasSmartIndentMacros(const QString &languageMode) {
	if (findIndentSpec(languageMode) != nullptr) {
		return true;
	}

	return SmartIndentDialog && SmartIndentDialog->languageMode() == languageMode;
}

/*
** Change the language mode name of smart indent macro sets for language
** "oldName" to "newName" in both the stored macro sets, and the pattern set
** currently being edited in the dialog.
*/
void RenameSmartIndentMacros(const QString &oldName, const QString &newName) {

	for(SmartIndentEntry &sis : SmartIndentSpecs) {
		if (sis.languageMode == oldName) {
			sis.languageMode = newName;
		}
	}

	if (SmartIndentDialog) {
		if(SmartIndentDialog->languageMode_ == oldName) {
			SmartIndentDialog->setLanguageMode(newName);
		}
	}
}

/*
** If a smart indent dialog is up, ask to have the option menu for
** chosing language mode updated (via a call to CreateLanguageModeMenu)
*/
void UpdateLangModeMenuSmartIndent() {

	if(SmartIndentDialog) {
		SmartIndentDialog->updateLanguageModes();
	}
}

}
