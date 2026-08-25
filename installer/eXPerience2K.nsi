Unicode true
RequestExecutionLevel admin
SetCompressor /SOLID lzma
SetCompressorDictSize 32

!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"

!define PRODUCT_NAME "eXPerience2K64"
!define PRODUCT_VERSION "2.4.1.0"
!define PRODUCT_DISPLAY_VERSION "2.4.1"
!define PRODUCT_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\eXPerience2K64"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\eXPerience2K64.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Open eXPerience2K64 configuration"

Name "${PRODUCT_NAME}"
OutFile "..\dist\eXPerience2K64-v2.4.1-Setup.exe"
InstallDir "$WINDIR\eXPerience2K64"
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "CompanyName" "eXPerience2K64 project"
VIAddVersionKey /LANG=1033 "FileDescription" "Windows 2000-style conversion for XP x64"
VIAddVersionKey /LANG=1033 "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Project code and third-party visual resources retain their respective terms."

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\payload\License.txt"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Var CoreExe

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP \
      "Windows XP x86 is not currently supported by eXPerience2K64. x86 support is in the works and should be available soon. Please keep an eye on the eXPerience2K64 GitHub repository for updates.$\r$\n$\r$\nNo files or settings have been changed."
    Abort
  ${EndIf}
FunctionEnd

Function SelectCore
  StrCpy $CoreExe "$INSTDIR\eXPerience2KCore-x64.exe"
FunctionEnd

Section "eXPerience2K64 application" SecFiles
  SectionIn RO
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File /r "..\payload\*.*"
  File "..\build\eXPerience2K64.exe"
  File "..\build\eXPerience2KCore-x64.exe"
  File "..\build\eXPerience2KExplorerBand64.dll"
  SetOutPath "$INSTDIR\Tools"
  File /oname=ResourceHacker.exe "..\tools\resource-hacker\ResourceHacker.exe"
  File /oname=ResourceHacker-ReadMe.txt "..\tools\resource-hacker\ReadMe.txt"
  SetOutPath "$INSTDIR\Source"
  File "..\src\eXPerience2KCore.c"
  File "..\src\eXPerience2KConfig.c"
  File "..\src\eXPerience2KExplorerBand.cpp"
  SetOutPath "$INSTDIR"

  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${PRODUCT_KEY}" "DisplayVersion" "${PRODUCT_DISPLAY_VERSION}"
  WriteRegStr HKLM "${PRODUCT_KEY}" "Publisher" "eXPerience2K64 project"
  WriteRegStr HKLM "${PRODUCT_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${PRODUCT_KEY}" "DisplayIcon" "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_KEY}" "UninstallString" '"$INSTDIR\uninst.exe"'
  WriteRegDWORD HKLM "${PRODUCT_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${PRODUCT_KEY}" "NoRepair" 1
  WriteRegStr HKLM "Software\eXPerience2K64" "InstallDir" "$INSTDIR"

  Call SelectCore
  CreateDirectory "$SMPROGRAMS\eXPerience2K64"
  CreateShortCut "$SMPROGRAMS\eXPerience2K64\eXPerience2K64.lnk" \
    "$INSTDIR\eXPerience2K64.exe"
  CreateShortCut "$SMPROGRAMS\eXPerience2K64\Verify Installation.lnk" \
    "$CoreExe" 'verify "$INSTDIR" "$INSTDIR\verification.tsv"'
  CreateShortCut "$SMPROGRAMS\eXPerience2K64\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  IfFileExists "$INSTDIR\eXPerience2K64.exe" 0 restore_helper_missing
    DetailPrint "Restoring original user settings, login presentation, and protected system resources..."
    nsExec::ExecToLog '"$INSTDIR\eXPerience2K64.exe" /restore-all'
    Pop $0
    ${If} $0 != 0
      MessageBox MB_OK|MB_ICONSTOP \
        "eXPerience2K64 could not safely restore every managed change (error $0). The application and its backups have been preserved. Restart Windows and try uninstalling again."
      Abort
    ${EndIf}
    Goto restoration_complete
  restore_helper_missing:
    MessageBox MB_OK|MB_ICONSTOP \
      "The eXPerience2K64 restoration helper is missing. The application and its backups will be preserved so the installation can be repaired before uninstalling."
    Abort
  restoration_complete:
  SetRebootFlag true
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" \
    "eXPerience2K64 Resource Reloader"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" \
    "eXPerience2K Resource Reloader"
  Delete "$SMSTARTUP\eXPerience2K64 Reloader.lnk"
  Delete "$SMSTARTUP\eXPerience2K Reloader.lnk"
  RMDir /r "$SMPROGRAMS\eXPerience2K64"
  DeleteRegKey HKLM "${PRODUCT_KEY}"
  DeleteRegKey HKLM "Software\eXPerience2K64"
  DeleteRegKey HKCU "Software\eXPerience2K64"
  DeleteRegKey HKLM "Software\eXPerience2K"
  DeleteRegKey HKCU "Software\eXPerience2K"
  RMDir /r /REBOOTOK "$INSTDIR"
  SetRebootFlag true
SectionEnd
