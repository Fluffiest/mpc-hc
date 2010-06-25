#
# This script gets the status of the translations.
#
# Copyright (C) 2007-2010 by Tim Gerundt, Arkadiy Shapkin
# Released under the "GNU General License"
#
# ID line follows -- this is updated by SVN
# $Id: GetTranslationsStatus.vbs 2065 2010-06-20 16:42:14Z kinddragon $

import os, codecs, re, operator

from datetime import datetime
from collections import OrderedDict

SvnWebUrlLanguages = "http://winmerge.svn.sourceforge.net/viewvc/winmerge/trunk/Translations/WinMerge/"

def GetLanguages():
    oLanguages = OrderedDict()

    for oFile in os.listdir("."): #For all files in the current folder...
        ext = os.path.splitext(oFile)[1].lower()
        name = os.path.splitext(oFile)[0]
        if ext == ".po": #If a PO file...
            oLanguages[name] = oFile
    return oLanguages

class CTranslator:
    Name = ""
    Mail = ""
    Maintainer = ""
    
class CStatus:
    def __init__(self):
        self.Count = 0
        self.Translated = 0
        self.Untranslated = 0
        self.Fuzzy = 0
        self.PoRevisionDate = ""
        self.PotCreationDate = ""
        self.Translators = { }

    def AddTranslator(self, sTranslator, bMaintainer):
        self.Translator = CTranslator()
        self.Translator.Maintainer = bMaintainer
        self.Translator.Mail = GetRegExpSubMatch(sTranslator, "<(.*)>").strip()
        if (self.Translator.Mail != ""): #if mail address exists...
            self.Translator.Name = GetRegExpSubMatch(sTranslator, "(.*) <.*>").strip()
        else: #if mail address NOT exists...
            self.Translator.Name = sTranslator        
        self.Translators[len(self.Translators)] = self.Translator

def GetTranslationsStatusFromPoFile(sPoPath):
    reMsgId = re.compile("^msgid \"(.*)\"$", re.IGNORECASE)
    reMsgStr = re.compile("^msgstr \"(.*)\"$", re.IGNORECASE)
    reMsgContinued = re.compile("^\"(.*)\"$", re.IGNORECASE)

    oStatus = CStatus()
    if (os.path.exists(sPoPath)): #if the PO file exists...
        iMsgStarted = 0
        sMsgId = ""
        sMsgStr = ""
        bFuzzy = False
        bMaintainer = False
        oTextFile = codecs.open(sPoPath, 'r', 'utf-8')
        for line in oTextFile: #For all lines...
            sLine = line.strip()
            if sLine != "": #if NOT empty line...
                if sLine[0] != "#": #if NOT comment line...
                    if reMsgId.search(sLine): #if "msgid"...
                        iMsgStarted = 1
                        oMatch = reMsgId.search(sLine).groups()
                        sMsgId = oMatch[0]
                    elif reMsgStr.search(sLine): #if "msgstr"...
                        iMsgStarted = 2
                        oMatch = reMsgStr.search(sLine).groups()
                        sMsgStr = oMatch[0]
                    elif reMsgContinued.search(sLine): #if "msgid" or "msgstr" continued...
                        oMatch = reMsgContinued.search(sLine).groups()
                        if (iMsgStarted == 1):
                            sMsgId = sMsgId + oMatch[0]
                        elif (iMsgStarted == 2):
                            sMsgStr = sMsgStr + oMatch[0]
                else: #if comment line...
                    iMsgStarted = -1
                    if sLine[:2] == "#,": #if "Reference" line...
                        if sLine.find("fuzzy") >= 0: #if "fuzzy"...
                            bFuzzy = True
                        
                    elif sLine == "# Maintainer:": #if maintainer list starts...
                        bMaintainer
                    elif sLine == "# Translators:": #if translators list starts...
                        bMaintainer = False
                    elif sLine[:4] == "# * ": #if translator/maintainer...
                        oStatus.AddTranslator(sLine[:5].strip(), bMaintainer)
            else: #if empty line
                iMsgStarted = 0
            if iMsgStarted == 0: #if NOT inside a translation...
                if sMsgId != "":
                    oStatus.Count = oStatus.Count + 1
                    if not bFuzzy: #if NOT a fuzzy translation...
                        if sMsgStr != "":
                            oStatus.Translated = oStatus.Translated + 1
                        else:
                            oStatus.Untranslated = oStatus.Untranslated + 1
                    else: #if a fuzzy translation...
                        oStatus.Fuzzy = oStatus.Fuzzy + 1
                elif sMsgStr != "":
                    oStatus.PoRevisionDate = GetRegExpSubMatch(sMsgStr, "PO-Revision-Date: ([0-9 :\+\-]+)")
                    oStatus.PotCreationDate = GetRegExpSubMatch(sMsgStr, "POT-Creation-Date: ([0-9 :\+\-]+)")
                sMsgId = ""
                sMsgStr = ""
                bFuzzy = False
        oTextFile.close()
    return oStatus

def CreateTranslationsStatusHtmlFile(sHtmlPath, sortedKeys, oTranslationsStatus):
    oHtmlFile = open(sHtmlPath, "w")

    oHtmlFile.write("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n")
    oHtmlFile.write("  \"http://www.w3.org/TR/html4/loose.dtd\">\n")
    oHtmlFile.write("<html>\n")
    oHtmlFile.write("<head>\n")
    oHtmlFile.write("  <title>Translations Status</title>\n")
    oHtmlFile.write("  <meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n")
    oHtmlFile.write("  <style type=\"text/css\">\n")
    oHtmlFile.write("  <!--\n")
    oHtmlFile.write("    body {\n")
    oHtmlFile.write("      font-family: Verdana,Helvetica,Arial,sans-serif;\n")
    oHtmlFile.write("      font-size: small;\n")
    oHtmlFile.write("    }\n")
    oHtmlFile.write("    code,pre {\n")
    oHtmlFile.write("      font-family: \"Courier New\",Courier,monospace;\n")
    oHtmlFile.write("      font-size: 1em;\n")
    oHtmlFile.write("    }\n")
    oHtmlFile.write("    .status {\n")
    oHtmlFile.write("      border-collapse: collapse;\n")
    oHtmlFile.write("      border: 1px solid #aaaaaa;\n")
    oHtmlFile.write("    }\n")
    oHtmlFile.write("    .status th {\n")
    oHtmlFile.write("      padding: 3px;\n")
    oHtmlFile.write("      background: #f2f2f2;\n")
    oHtmlFile.write("      border: 1px solid #aaaaaa;\n")
    oHtmlFile.write("    }\n")
    oHtmlFile.write("    .status td {\n")
    oHtmlFile.write("      padding: 3px;\n")
    oHtmlFile.write("      background: #f9f9f9;\n")
    oHtmlFile.write("      border: 1px solid #aaaaaa;\n")
    oHtmlFile.write("    }\n")
    oHtmlFile.write("    .left { text-align: left; }\n")
    oHtmlFile.write("    .center { text-align: center; }\n")
    oHtmlFile.write("    .right { text-align: right; }\n")
    oHtmlFile.write("  -->\n")
    oHtmlFile.write("  </style>\n")
    oHtmlFile.write("</head>\n")
    oHtmlFile.write("<body>\n")
    oHtmlFile.write("<h1>Translations Status</h1>\n")
    oHtmlFile.write("<p>Status from <strong>" + GetCreationDate() + "</strong>:</p>\n")
    oHtmlFile.write("<table class=\"status\">\n")
    oHtmlFile.write("  <tr>\n")
    oHtmlFile.write("    <th class=\"left\">Language</th>\n")
    oHtmlFile.write("    <th class=\"right\">Total</th>\n")
    oHtmlFile.write("    <th class=\"right\">Translated</th>\n")
    oHtmlFile.write("    <th class=\"right\">Fuzzy</th>\n")
    oHtmlFile.write("    <th class=\"right\">Untranslated</th>\n")
    oHtmlFile.write("    <th class=\"center\">Last Update</th>\n")
    oHtmlFile.write("  </tr>\n")
    for sLanguage in sortedKeys: #For all languages...
        if sLanguage != "English": #if NOT English...
            oLanguageStatus = oTranslationsStatus[sLanguage]
            oHtmlFile.write("  <tr>\n")
            oHtmlFile.write("    <td class=\"left\">" + sLanguage + "</td>\n")
            oHtmlFile.write("    <td class=\"right\">" + str(oLanguageStatus.Count) + "</td>\n")
            oHtmlFile.write("    <td class=\"right\">" + str(oLanguageStatus.Translated) + "</td>\n")
            oHtmlFile.write("    <td class=\"right\">" + str(oLanguageStatus.Fuzzy) + "</td>\n")
            oHtmlFile.write("    <td class=\"right\">" + str(oLanguageStatus.Untranslated) + "</td>\n")
            oHtmlFile.write("    <td class=\"center\">" + oLanguageStatus.PoRevisionDate[:10] + "</td>\n")
            oHtmlFile.write("  </tr>\n")
    oLanguageStatus = oTranslationsStatus["English"]
    oHtmlFile.write("  <tr>\n")
    oHtmlFile.write("    <td class=\"left\">English</td>\n")
    oHtmlFile.write("    <td class=\"right\">" + str(oLanguageStatus.Count) + "</td>\n")
    oHtmlFile.write("    <td class=\"right\">" + str(oLanguageStatus.Count) + "</td>\n")
    oHtmlFile.write("    <td class=\"right\">0</td>\n")
    oHtmlFile.write("    <td class=\"right\">0</td>\n")
    oHtmlFile.write("    <td class=\"center\">" + oLanguageStatus.PotCreationDate[:10] + "</td>\n")
    oHtmlFile.write("  </tr>\n")
    oHtmlFile.write("</table>\n")
    oHtmlFile.write("<h2>Translators</h2>\n")
    oHtmlFile.write("<table class=\"status\">\n")
    oHtmlFile.write("  <tr>\n")
    oHtmlFile.write("    <th class=\"left\">Language</th>\n")
    oHtmlFile.write("    <th class=\"left\">Translator(s)</th>\n")
    oHtmlFile.write("  </tr>\n")
    for sLanguage in sortedKeys: #For all languages...
        if (sLanguage != "English"): #if NOT English...
            oLanguageStatus = oTranslationsStatus[sLanguage]
            oHtmlFile.write("  <tr>\n")
            oHtmlFile.write("    <td>" + sLanguage + "</td>\n")
            oHtmlFile.write("    <td>\n")
            for i in range(0, len(oLanguageStatus.Translators) - 1): #For all translators...
                sMaintainerStart = ""
                sMaintainerEnd = ""
                status = oLanguageStatus.Translators[i]
                if (status.Maintainer): #if maintainer...
                    sMaintainerStart = "<strong title=\"Maintainer\">"
                    sMaintainerEnd = "</strong>"                
                if (status.Mail != ""): #if mail address exists...
                    oHtmlFile.write("      " + sMaintainerStart + "<a href=\"mailto:" + status.Mail + "\">" + status.Name + "</a>" + sMaintainerEnd + "<br>\n")
                else: #if mail address NOT exists...
                    oHtmlFile.write("      " + sMaintainerStart + status.Name + sMaintainerEnd + "<br>\n")
            oHtmlFile.write("    </td>\n")
            oHtmlFile.write("  </tr>\n")
    oHtmlFile.write("</table>\n")
    oHtmlFile.write("</body>\n")
    oHtmlFile.write("</html>\n")
    oHtmlFile.close()

def CreateTranslationsStatusWikiFile(sWikiPath, sortedKeys, oTranslationsStatus):
    oWikiFile = open(sWikiPath, "w")
    
    oWikiFile.write("== Translations Status ==\n")
    oWikiFile.write("Status from #''" + GetCreationDate() + "''':\n")
    oWikiFile.write("{| class=\"wikitable\" border=\"1\"\n")
    oWikiFile.write("! Language\n")
    oWikiFile.write("! Total\n")
    oWikiFile.write("! Translated\n")
    oWikiFile.write("! Fuzzy\n")
    oWikiFile.write("! Untranslated\n")
    oWikiFile.write("! Last Update\n")
    for sLanguage in sortedKeys: #For all languages...
        if (sLanguage != "English"): #if NOT English...
            oLanguageStatus = oTranslationsStatus[sLanguage]
            oWikiFile.write("|-\n")
            oWikiFile.write("|align=\"left\"| " + sLanguage + "\n")
            oWikiFile.write("|align=\"right\"| " + str(oLanguageStatus.Count) + "\n")
            oWikiFile.write("|align=\"right\"| " + str(oLanguageStatus.Translated) + "\n")
            oWikiFile.write("|align=\"right\"| " + str(oLanguageStatus.Fuzzy) + "\n")
            oWikiFile.write("|align=\"right\"| " + str(oLanguageStatus.Untranslated) + "\n")
            oWikiFile.write("|align=\"center\"| " + oLanguageStatus.PoRevisionDate[:10] + "\n")
    oLanguageStatus = oTranslationsStatus["English"]
    oWikiFile.write("|-\n")
    oWikiFile.write("|align=\"left\"| English\n")
    oWikiFile.write("|align=\"right\"| " + str(oLanguageStatus.Count) + "\n")
    oWikiFile.write("|align=\"right\"| " + str(oLanguageStatus.Count) + "\n")
    oWikiFile.write("|align=\"right\"| 0\n")
    oWikiFile.write("|align=\"right\"| 0\n")
    oWikiFile.write("|align=\"center\"| " + oLanguageStatus.PotCreationDate[:10] + "\n")
    oWikiFile.write("|}\n")
    oWikiFile.close()

def CreateTranslationsStatusXmlFile(sXmlPath, sortedKeys, oTranslationsStatus):
    oXmlFile = open(sXmlPath, "w")

    oXmlFile.write("<translations>\n")
    oXmlFile.write("  <update>" + GetCreationDate() + "</update>\n")
    for sLanguage in sortedKeys: #For all languages...
        if (sLanguage != "English"): #if NOT English...
            oLanguageStatus = oTranslationsStatus[sLanguage]
            oXmlFile.write("  <translation>\n")
            oXmlFile.write("    <language>" + sLanguage + "</language>\n")
            oXmlFile.write("    <file>" + sLanguage + ".po</file>\n")
            oXmlFile.write("    <update>" + oLanguageStatus.PoRevisionDate[:10] + "</update>\n")
            oXmlFile.write("    <strings>\n")
            oXmlFile.write("      <count>"          + str(oLanguageStatus.Count)        + "</count>\n")
            oXmlFile.write("      <translated>"     + str(oLanguageStatus.Translated)   + "</translated>\n")
            oXmlFile.write("      <fuzzy>"          + str(oLanguageStatus.Fuzzy)        + "</fuzzy>\n")
            oXmlFile.write("      <untranslated>"   + str(oLanguageStatus.Untranslated) + "</untranslated>\n")
            oXmlFile.write("    </strings>\n")
            oXmlFile.write("    <translators>\n")
            for i in range(0, len(oLanguageStatus.Translators) - 1): #For all translators...
                status = oLanguageStatus.Translators[i]
                if (status.Maintainer): #if maintainer...
                    oXmlFile.write("      <translator maintainer=\"1\">\n")
                else: #if NOT maintainer...
                    oXmlFile.write("      <translator>\n")
                oXmlFile.write("        <name>" + status.Name + "</name>\n")
                if (status.Mail != ""): #if mail address exists...
                    oXmlFile.write("        <mail>" + status.Mail + "</mail>\n")
                oXmlFile.write("      </translator>\n")
            oXmlFile.write("    </translators>\n")
            oXmlFile.write("  </translation>\n")
    oLanguageStatus = oTranslationsStatus["English"]
    oXmlFile.write("  <translation>\n")
    oXmlFile.write("    <language>English</language>\n")
    oXmlFile.write("    <file>English.pot</file>\n")
    oXmlFile.write("    <update>" + oLanguageStatus.PotCreationDate[:10] + "</update>\n")
    oXmlFile.write("    <strings>\n")
    oXmlFile.write("      <count>" + str(oLanguageStatus.Count) + "</count>\n")
    oXmlFile.write("      <translated>" + str(oLanguageStatus.Count) + "</translated>\n")
    oXmlFile.write("      <fuzzy>0</fuzzy>\n")
    oXmlFile.write("      <untranslated>0</untranslated>\n")
    oXmlFile.write("    </strings>\n")
    oXmlFile.write("  </translation>\n")
    oXmlFile.write("</translations>\n")
    oXmlFile.close()

def CreateTranslatorsListFile(sHtmlPath, sortedKeys, oTranslationsStatus):
    oHtmlFile = open(sHtmlPath, "w")

    oHtmlFile.write("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n")
    oHtmlFile.write("  \"http://www.w3.org/TR/html4/loose.dtd\">\n")
    oHtmlFile.write("<html>\n")
    oHtmlFile.write("<head>\n")
    oHtmlFile.write("  <title>WinMerge Translators</title>\n")
    oHtmlFile.write("  <meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n")
    oHtmlFile.write("  <style type=\"text/css\">\n")
    oHtmlFile.write("  <!--\n")
    oHtmlFile.write("    body {\n")
    oHtmlFile.write("      font-family: Verdana,Helvetica,Arial,sans-serif;\n")
    oHtmlFile.write("      font-size: small;\n")
    oHtmlFile.write("    }\n")
    oHtmlFile.write("    dl {\n")
    oHtmlFile.write("      width: 30em;\n")
    oHtmlFile.write("      margin: 1em;\n")
    oHtmlFile.write("      padding: .5em;\n")
    oHtmlFile.write("      background: #F9F9F9;\n")
    oHtmlFile.write("      border: 1px solid #AAAAAA;\n")
    oHtmlFile.write("    }\n")
    oHtmlFile.write("    dt {\n")
    oHtmlFile.write("      margin-top: .5em;\n")
    oHtmlFile.write("      font-weight: bold;\n")
    oHtmlFile.write("    }\n")
    oHtmlFile.write("  -->\n")
    oHtmlFile.write("  </style>\n")
    oHtmlFile.write("</head>\n")
    oHtmlFile.write("<body>\n")
    oHtmlFile.write("<h1>WinMerge Translators</h1>\n")
    oHtmlFile.write("<ul>\n")
    for sLanguage in sortedKeys: #For all languages...
        if (sLanguage != "English"): #if NOT English...
            oHtmlFile.write("  <li><a href=\"#" + sLanguage + "\">" + sLanguage + "</a></li>\n")    
    oHtmlFile.write("</ul>\n")
    for sLanguage in sortedKeys: #For all languages...
        if (sLanguage != "English"): #if NOT English...
            sMaintainer = ""
            sTranslators = ""
            oLanguageStatus = oTranslationsStatus[sLanguage]
            oHtmlFile.write("<h2><a name=\"" + sLanguage + "\">" + sLanguage + "</a></h2>\n")
            oHtmlFile.write("<dl>\n")
            for i in range(0, len(oLanguageStatus.Translators) - 1): #For all translators...
                status = oLanguageStatus.Translators[i]
                if (status.Maintainer): #if maintainer...
                    if (status.Mail != ""): #if mail address exists...
                        sMaintainer = "<dd><strong><a href=\"mailto:" + status.Mail + "\">" + status.Name + "</a></strong></dd>"
                    else: #if mail address NOT exists...
                        sMaintainer = "<dd><strong>" + status.Name + "</strong></dd>"
                else: #if NO maintainer...
                    if (status.Mail != ""): #if mail address exists...
                        sTranslators = sTranslators + "<dd><a href=\"mailto:" + status.Mail + "\">" + status.Name + "</a></dd>"
                    else: #if mail address NOT exists...
                        sTranslators = sTranslators + "<dd>" + status.Name + "</dd>"
            if (sMaintainer != ""): #if maintainer exists...
                oHtmlFile.write("  <dt>Maintainer:</dt>\n")
                oHtmlFile.write("  " + sMaintainer + "\n")
            if (sTranslators != ""): #if translators exists...
                oHtmlFile.write("  <dt>Translators:</dt>\n")
                oHtmlFile.write("  " + sTranslators + "\n")
            sLastUpdated = oLanguageStatus.PoRevisionDate[:10]
            if (sLastUpdated != ""): #if PO revision date exists...
                oHtmlFile.write("  <dt>Last Update:</dt>\n")
                oHtmlFile.write("  <dd>" + oLanguageStatus.PoRevisionDate[:10] + "</dd>\n")
            oHtmlFile.write("  <dt>Translation File:</dt>\n")
            oHtmlFile.write("  <dd><a href=\"" + SvnWebUrlLanguages + sLanguage + ".po\" rel=\"nofollow\">" + sLanguage + ".po</a></dd>\n")
            oHtmlFile.write("</dl>\n")    
    oHtmlFile.write("<p>Status from <strong>" + GetCreationDate() + "</strong>. Look at <a href=\"http://winmerge.org/translations/\">winmerge.org</a> for updates.</p>\n")
    oHtmlFile.write("</body>\n")
    oHtmlFile.write("</html>\n")
    oHtmlFile.close()

def FoundRegExpMatch(sString, sPattern, flags = re.IGNORECASE):
    mo = re.search(sPattern, sString, flags)
    if mo:
        g = mo.groups()
        if g:
            return g
        else:
            return []
    return None

def GetRegExpSubMatch(sString, sPattern):
    oMatch = FoundRegExpMatch(sString, sPattern)
    if oMatch: #if pattern found...
        return oMatch[0]
    return ""

def GetCreationDate():
    oNow = datetime.now()
    sYear = str(oNow.year)
    sMonth = str(oNow.month)
    if oNow.month < 10:
        sMonth = "0" + sMonth
    sDay = str(oNow.day)
    if oNow.day < 10:
        sDay = "0" + sDay
    return sYear + "-" + sMonth + "-" + sDay

if __name__ == '__main__':
    StartTime = datetime.now()
    
    print("Creating translations status files...")
    oTranslationsStatus = OrderedDict()
    print("English")    
    oTranslationsStatus["English"] = GetTranslationsStatusFromPoFile("English.pot")
    oLanguages = GetLanguages()
    for sLanguage in oLanguages.keys(): #For all languages...
        print(sLanguage)        
        lang = oLanguages[sLanguage]
        oTranslationsStatus[sLanguage] = GetTranslationsStatusFromPoFile(lang)
    
    datalist = [(oTranslationsStatus[lang].Translated, lang, oTranslationsStatus[lang]) for lang in oTranslationsStatus.keys()]
    datalist.sort()
    sortedKeys = [data[1] for data in datalist]
    
    CreateTranslationsStatusHtmlFile("TranslationsStatus.html", sortedKeys, oTranslationsStatus)
    CreateTranslationsStatusWikiFile("TranslationsStatus.wiki", sortedKeys, oTranslationsStatus)
    CreateTranslationsStatusXmlFile("TranslationsStatus.xml", sortedKeys, oTranslationsStatus)
    CreateTranslatorsListFile("Translators.html", sortedKeys, oTranslationsStatus)

    EndTime = datetime.now()
    Seconds = str((EndTime-StartTime).seconds)

    print("Translations status files created, after " + Seconds + " second(s).")
