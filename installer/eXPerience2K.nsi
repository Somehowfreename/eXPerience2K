Unicode true
RequestExecutionLevel admin
SetCompressor /SOLID lzma
SetCompressorDictSize 32

!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"

!define PRODUCT_NAME "eXPerience2K"
!define PRODUCT_VERSION "2.4.1.0"
!define PRODUCT_DISPLAY_VERSION "2.4.1"
!define PRODUCT_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\eXPerience2K"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\eXPerience2K.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Open eXPerience2K configuration"

Name "${PRODUCT_NAME}"
OutFile "..\dist\eXPerience2K-v2.4.1-Setup.exe"
InstallDir "$WINDIR\eXPerience2K"
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "CompanyName" "eXPerience2K project"
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
    Goto unsupported_os
  ${EndIf}

  ; NSIS is a 32-bit process. On the one supported platform, Windows exposes
  ; the native processor through PROCESSOR_ARCHITEW6432 as AMD64. This also
  ; fails closed on x86 and IA-64 systems.
  ReadEnvStr $0 "PROCESSOR_ARCHITEW6432"
  StrCmp $0 "AMD64" architecture_ok unsupported_os

  architecture_ok:
  ; Read the native registry view, then require the exact NT 5.2 workstation
  ; identity used by Windows XP Professional x64 Edition SP2. ProductType
  ; WinNT excludes Server 2003 and Server 2003 R2, whose values are ServerNT
  ; or LanmanNT. CSDVersion 0x200 is the language-independent SP2 marker.
  SetRegView 64
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\ProductOptions" "ProductType"
  StrCmp $0 "WinNT" workstation_ok unsupported_os_native_view

  workstation_ok:
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"
  StrCmp $0 "5.2" version_ok unsupported_os_native_view

  version_ok:
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentBuildNumber"
  StrCmp $0 "3790" build_ok unsupported_os_native_view

  build_ok:
  ReadRegDWORD $0 HKLM "SYSTEM\CurrentControlSet\Control\Windows" "CSDVersion"
  IntCmp $0 0x200 supported_os unsupported_os_native_view unsupported_os_native_view

  supported_os:
  ; Preserve the registry-view behavior of the previously verified installer.
  SetRegView 32
  Return

  unsupported_os_native_view:
  SetRegView 32
  unsupported_os:
  MessageBox MB_OK|MB_ICONSTOP \
    "Only Windows XP Professional x64 Edition Service Pack 2 is currently supported by eXPerience2K.$\r$\n$\r$\nOther x64 editions of Windows XP, all Windows Server editions, and all x86 editions of Windows XP are not supported at this time. Windows XP Professional x86 support is planned for the next major update.$\r$\n$\r$\nPlease keep checking for an update:$\r$\nhttps://github.com/Somehowfreename/eXPerience2K$\r$\n$\r$\nNo files or settings have been changed."
  Abort
FunctionEnd

Function SelectCore
  StrCpy $CoreExe "$INSTDIR\eXPerience2KCore-x64.exe"
FunctionEnd

Section "eXPerience2K application" SecFiles
  SectionIn RO
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File /r "..\payload\*.*"
  File "..\build\eXPerience2K.exe"
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
  WriteRegStr HKLM "${PRODUCT_KEY}" "Publisher" "eXPerience2K project"
  WriteRegStr HKLM "${PRODUCT_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${PRODUCT_KEY}" "DisplayIcon" "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_KEY}" "UninstallString" '"$INSTDIR\uninst.exe"'
  WriteRegDWORD HKLM "${PRODUCT_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${PRODUCT_KEY}" "NoRepair" 1
  WriteRegStr HKLM "Software\eXPerience2K" "InstallDir" "$INSTDIR"

  Call SelectCore
  CreateDirectory "$SMPROGRAMS\eXPerience2K"
  CreateShortCut "$SMPROGRAMS\eXPerience2K\eXPerience2K.lnk" \
    "$INSTDIR\eXPerience2K.exe"
  CreateShortCut "$SMPROGRAMS\eXPerience2K\Verify Installation.lnk" \
    "$CoreExe" 'verify "$INSTDIR" "$INSTDIR\verification.tsv"'
  CreateShortCut "$SMPROGRAMS\eXPerience2K\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  IfFileExists "$INSTDIR\eXPerience2K.exe" 0 restore_helper_missing
    DetailPrint "Restoring original user settings, login presentation, and protected system resources..."
    nsExec::ExecToLog '"$INSTDIR\eXPerience2K.exe" /restore-all'
    Pop $0
    ${If} $0 != 0
      MessageBox MB_OK|MB_ICONSTOP \
        "eXPerience2K could not safely restore every managed change (error $0). The application and its backups have been preserved. Restart Windows and try uninstalling again."
      Abort
    ${EndIf}
    Goto restoration_complete
  restore_helper_missing:
    MessageBox MB_OK|MB_ICONSTOP \
      "The eXPerience2K restoration helper is missing. The application and its backups will be preserved so the installation can be repaired before uninstalling."
    Abort
  restoration_complete:
  SetRebootFlag true
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" \
    "eXPerience2K Resource Reloader"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" \
    "eXPerience2K Resource Reloader"
  Delete "$SMSTARTUP\eXPerience2K Reloader.lnk"
  Delete "$SMSTARTUP\eXPerience2K Reloader.lnk"
  RMDir /r "$SMPROGRAMS\eXPerience2K"
  DeleteRegKey HKLM "${PRODUCT_KEY}"
  DeleteRegKey HKLM "Software\eXPerience2K"
  DeleteRegKey HKCU "Software\eXPerience2K"
  DeleteRegKey HKLM "Software\eXPerience2K64"
  DeleteRegKey HKCU "Software\eXPerience2K64"
  DeleteRegKey HKLM "Software\eXPerience2K"
  DeleteRegKey HKCU "Software\eXPerience2K"
  RMDir /r /REBOOTOK "$INSTDIR"
  SetRebootFlag true
SectionEnd
