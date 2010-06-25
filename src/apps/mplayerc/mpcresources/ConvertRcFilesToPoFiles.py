#
# This script converts the language RC files to the language PO files.
#
# Copyright (C) 2007-2010 by Tim Gerundt, Arkadiy Shapkin
# Released under the "GNU General License"
#
# ID line follows -- this is updated by SVN
# $Id: ConvertRcFilesToPoFiles.vbs 2065 2010-06-20 16:42:14Z kinddragon $

import os, sys, codecs, re

from datetime import datetime
from collections import OrderedDict

NO_BLOCK            = -1
MENU_BLOCK          = 0
DIALOGEX_BLOCK      = 1
STRINGTABLE_BLOCK   = 2
VERSIONINFO_BLOCK   = 3
ACCELERATORS_BLOCK  = 4
BEGIN_BLOCK         = 5
END_BLOCK           = 6

PATH_ENGLISH_POT = "English.pot"
PATH_MERGE_RC = "../mplayerc.rc"
PATH_MERGELANG_RC = "MplayercLang.rc"

def GetLanguages():
    oLanguages = OrderedDict()

    for oFile in os.listdir("."): #For all subfolders in the current folder...
        ext = os.path.splitext(oFile)[1].lower()
        name = os.path.splitext(oFile)[0]
        if ext == ".rc" and oFile != PATH_MERGELANG_RC:
            oLanguages[name] = oFile

    return oLanguages

def GetTranslationsFromRcFile(sRcPath):
    sLang = ""
    sSubLang = ""
    sCodePage = ""

    oTranslations = OrderedDict()

    if os.path.exists(sRcPath):
        iBlockType = NO_BLOCK
        iPosition = 0
        sCodePage = ""
        sKey1 = ""
        oTextFile = codecs.open(sRcPath, 'r', 'utf-16-le')
        for line in oTextFile:
            sLine = line.strip()

            sValue = ""
            sKey2 = ""

            curType, oMatch = FoundRegExpIndex(sLine, 
                ["(IDR_.*) MENU", "(IDD_.*) DIALOGEX", 
                "(STRINGTABLE)", "(VS_.*) VERSIONINFO",
                "(IDR_.*) ACCELERATORS", 
                "\b(BEGIN)\b", "\b(END)\b" ], 0)
            if curType != NO_BLOCK:
                if curType == END_BLOCK: #If inside stringtable...
                    if iBlockType == STRINGTABLE_BLOCK: 
                        iBlockType = NO_BLOCK
                        sKey1 = ""
                elif curType != BEGIN_BLOCK: #IGNORE FOR SPEEDUP!
                    iBlockType = curType
                    iPosition = 0
                    sKey1 = oMatch[-1]

            if sLine.find("//") == 0: #If comment line...
                sLine = "" #IGNORE FOR SPEEDUP!
            elif curType == NO_BLOCK and len(sLine) > 0: #If NOT empty line...
                if iBlockType == NO_BLOCK:
                    n, oMatch = FoundRegExpIndex(sLine,
                        ["LANGUAGE (LANG_\w*), (SUBLANG_\w*)", "code_page\(([\d]+)\)"])
                    if n == 0: #LANGUAGE
                        sLang = oMatch[0]
                        sSubLang = oMatch[1]
                    elif n == 1: #code_page...
                        sCodePage = oMatch[0]
                elif iBlockType == MENU_BLOCK:
                    n, oMatch = FoundRegExpIndex(sLine,
                        ["POPUP \"(.*)\"", "MENUITEM.*\"(.*)\".*(ID_.*)"])
                    if n == 0: #POPUP...
                        if oMatch[0].find("_POPUP_") == -1:
                            sKey2 = str(iPosition)
                            iPosition = iPosition + 1
                            sValue = oMatch[0]                        
                    elif n == 1: #MENUITEM...
                        sKey2 = oMatch[1]
                        sValue = oMatch[0]
                        
                elif iBlockType == DIALOGEX_BLOCK:
                    n, oMatch = FoundRegExpIndex(sLine,
                        ["CAPTION.*\"(.*)\"", "PUSHBUTTON.*\"(.*)\",(\w+)",
                         "[L|R|C]TEXT.*\"(.*)\",(\w+)", "[L|R]TEXT.*\"(.*)\",",
                         "CONTROL +\"(.*?)\",(\w+)", "CONTROL +\"(.*?)\",", 
                         "GROUPBOX +\"(.*?)\",(\w+)"], 0)
                    if n == 0: #CAPTION...
                        sKey2 = "CAPTION"
                        sValue = oMatch[0]
                    elif n == 1: #DEFPUSHBUTTON/PUSHBUTTON...
                        sKey2 = oMatch[1]
                        sValue = oMatch[0]
                    elif n == 2: #LTEXT/RTEXT...
                        if len(oMatch[0]) > 0 and (oMatch[0] != "Static"):
                            if (oMatch[1] != "IDC_STATIC"):
                                sKey2 = oMatch[1]
                            else:
                                sKey2 = str(iPosition) + "_TEXT"
                                iPosition = iPosition + 1                            
                            sValue = oMatch[0]                        
                    elif n == 3: #LTEXT/RTEXT (without ID)...
                        sKey2 = iPosition + "_TEXT"
                        iPosition = iPosition + 1
                        sValue = oMatch[0]
                    elif n == 4: #CONTROL...
                        if (oMatch[0] != "Dif") and (oMatch[0] != "Btn") and (oMatch[0] != "Button1"):
                            sKey2 = oMatch[1]
                            sValue = oMatch[0]
                    elif n == 5: #CONTROL (without ID)...
                        sKey2 = str(iPosition) + "_CONTROL"
                        iPosition = iPosition + 1
                        sValue = oMatch[0]
                    elif n == 6:  #GROUPBOX...
                        if (oMatch[1] != "IDC_STATIC"):
                            sKey2 = oMatch[1]
                        else:
                            sKey2 = str(iPosition) + "_GROUPBOX"
                            iPosition = iPosition + 1                        
                        sValue = oMatch[0]
                        
                elif iBlockType == STRINGTABLE_BLOCK:
                    n, oMatch = FoundRegExpIndex(sLine,
                        ["(\w+).*\"(.*)\"", "\"(.*)\""])
                    if n == 0: #String...
                        sKey2 = oMatch[0]
                        sValue = oMatch[1]
                    elif n == 1: #String (without ID)...
                        sKey2 = str(iPosition)
                        iPosition = iPosition + 1
                        sValue = oMatch[0]
                        
                elif iBlockType == VERSIONINFO_BLOCK:
                    n, oMatch = FoundRegExpIndex(sLine,
                        ["BLOCK \"([0-9A-F]+)\"", "VALUE \"(.*?)\", \"(.*?)\\?0?\"", "VALUE \"Translation\", (.*?)$"])
                    if n == 0: #StringFileInfo.Block...
                        sKey2 = "STRINGFILEINFO_BLOCK"
                        sValue = oMatch[0]
                    elif n == 1: #StringFileInfo.Value...
                        sKey2 = "STRINGFILEINFO_" + oMatch[0]
                        sValue = oMatch[1]
                    elif n == 2: #VarFileInfo.Translation...
                        sKey2 = "VARFILEINFO_TRANSLATION"
                        sValue = oMatch[0]

            if (sValue != ""):
                key = sKey1 + "." + sKey2
                if key not in oTranslations:
                    oTranslations[key] = sValue
                else:
                    #Dim newKey, oldValue, iNum
                    iNum = 1
                    newKey = key + str(iNum)
                    while newKey in oTranslations:
                        iNum = iNum + 1
                        newKey = key + str(iNum)                  
                    oTranslations[newKey] = sValue

        oTextFile.close()

        oTranslations["__LANGUAGE__"] = sLang + ", " + sSubLang
        oTranslations["__CODEPAGE__"] = sCodePage
    
    return oTranslations

def MergeTranslations(oOriginalTranslations, oLanguageTranslations):
    #Dim oMergedTranslations, sKey
    #Dim sOriginalTranslation, sLanguageTranslation

    oMergedTranslations = { }
    for sKey in oOriginalTranslations.keys(): #For all original translations...
        sOriginalTranslation = oOriginalTranslations[sKey]
        if sKey in oLanguageTranslations:
            sLanguageTranslation = oLanguageTranslations[sKey]
        else:
            sLanguageTranslation = ""

        if (sOriginalTranslation != "") and (sOriginalTranslation != sLanguageTranslation):
            if sOriginalTranslation not in oMergedTranslations:
                oMergedTranslations[sOriginalTranslation] = sLanguageTranslation

    oMergedTranslations["__CODEPAGE__"] = oLanguageTranslations["__CODEPAGE__"]
    return oMergedTranslations

def CreatePoFileWithTranslations(sMasterPotPath, sLanguagePoPath, oTranslations):
    #Dim oMasterPotFile, sMasterLine
    #Dim oLanguagePoFile, sLanguageLine
    #Dim oMatch, sMsgId, sMsgStr, sKey

    if not os.path.exists(sMasterPotPath): #if the master POT file exists...
        return
    
    sMsgId = ""
    sMsgStr = ""
    oMasterPotFile = codecs.open(sMasterPotPath, 'r', 'utf-8')
    oLanguagePoFile = codecs.open(sLanguagePoPath, 'w', 'utf-8')
    for line in oMasterPotFile: #For all lines...
        sMasterLine = line
        sLanguageLine = sMasterLine

        if sMasterLine.strip() != "": #if NOT empty line...
            n, oMatch = FoundRegExpIndex(sMasterLine,
                ["msgid \"(.*)\"", "msgstr \"\"",
                 "CP1252", "English"])
            if n == 0: #if "msgid"...
                sMsgId = oMatch[0]
                if sMsgId in oTranslations: #if translation located...
                    sMsgStr = oTranslations[sMsgId]
                
            elif n == 1: #if "msgstr"...
                if (sMsgId == "1252") and (sMsgStr == ""): #if same codepage...
                    sMsgStr = oTranslations["__CODEPAGE__"]
                
                if (sMsgStr != ""): #if translated...
                    sLanguageLine = sMasterLine.replace("msgstr \"\"", "msgstr \"" + sMsgStr + "\"")
                
            elif n == 2: #if "Codepage"...
                sLanguageLine = sMasterLine.replace("CP1252", "CP" + oTranslations["__CODEPAGE__"])
            elif n == 3: #if "English"...
                sLanguageLine = sMasterLine.replace("English", os.path.splitext(sLanguagePoPath)[0])
            
        else: #if empty line
            sMsgId = ""
            sMsgStr = ""            

        oLanguagePoFile.write(sLanguageLine)
    
    oMasterPotFile.close()
    oLanguagePoFile.close()

def FoundRegExpMatch(sString, sPattern, flags = re.IGNORECASE):
    mo = re.search(sPattern, sString, flags)
    if mo:
        g = mo.groups()
        if g:
            return g
        else:
            return []
    return None

def FoundRegExpIndex(sString, exprs, flags = re.IGNORECASE):
    i = 0  
    for expr in exprs:
        oMatch = FoundRegExpMatch(sString, expr, flags)
        if oMatch != None:
            return i, oMatch
        i = i + 1
    return -1, None

if __name__ == '__main__':
    StartTime = datetime.now()

    scriptName = os.path.basename(sys.argv[0])
    print("Warning: " + scriptName + " can take several minutes to finish!")

    if os.path.exists(PATH_ENGLISH_POT): #if the master POT file exists...
        oOriginalTranslations = GetTranslationsFromRcFile(PATH_MERGE_RC)

        #oLanguageTranslations = GetTranslationsFromRcFile("mplayerc.ru.rc")
        #oMergedTranslations = MergeTranslations(oOriginalTranslations, oLanguageTranslations)
        #CreatePoFileWithTranslations(PATH_ENGLISH_POT, "mplayerc.ru.po", oMergedTranslations)

        #Set oLanguageTranslations = GetTranslationsFromRcFile("mplayerc.sc.rc")
        #Set oMergedTranslations = MergeTranslations(oOriginalTranslations, oLanguageTranslations)
        #CreatePoFileWithTranslations(PATH_ENGLISH_POT, "mplayerc.sc.po", oMergedTranslations)

        oLanguages = GetLanguages()
        for sLanguage in oLanguages.keys():
            if os.path.exists(sLanguage + ".po"):
                os.remove(sLanguage + ".po")
        for sLanguage in oLanguages.keys(): #For all languages...
            print(sLanguage)            
            oLanguageTranslations = GetTranslationsFromRcFile(oLanguages[sLanguage])
            oMergedTranslations = MergeTranslations(oOriginalTranslations, oLanguageTranslations)
            if (len(oMergedTranslations) > 0): #if translations exists...
                CreatePoFileWithTranslations(PATH_ENGLISH_POT, sLanguage + ".po", oMergedTranslations)

    EndTime = datetime.now()
    Seconds = str((EndTime-StartTime).seconds)

    print(scriptName + " finished after " + Seconds + " seconds!")
