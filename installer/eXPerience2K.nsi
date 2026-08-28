Unicode true
RequestExecutionLevel admin
SetCompressor /SOLID lzma
SetCompressorDictSize 32

!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"

!define PRODUCT_NAME "eXPerience2K"
!define PRODUCT_VERSION "3.1.0.0"
!define PRODUCT_DISPLAY_VERSION "3.1.0"
!define PRODUCT_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\eXPerience2K"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\eXPerience2K.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Open eXPerience2K configuration"

Name "${PRODUCT_NAME}"
OutFile "..\dist\eXPerience2K-v3.1.0-Setup.exe"
InstallDir "$WINDIR\eXPerience2K"
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "CompanyName" "eXPerience2K project"
VIAddVersionKey /LANG=1033 "FileDescription" "Windows 2000-style conversion for Windows XP Professional"
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
  ${If} ${RunningX64}
    Goto check_x64
  ${Else}
    Goto check_x86
  ${EndIf}

  check_x64:
  ; NSIS is a 32-bit process. Windows exposes the native x64 processor through
  ; PROCESSOR_ARCHITEW6432. Fail closed on IA-64 and unknown architectures.
  ReadEnvStr $0 "PROCESSOR_ARCHITEW6432"
  StrCmp $0 "AMD64" x64_architecture_ok unsupported_os

  x64_architecture_ok:
  SetRegView 64
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\ProductOptions" "ProductType"
  StrCmp $0 "WinNT" x64_workstation_ok unsupported_os_native_view

  x64_workstation_ok:
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"
  StrCmp $0 "5.2" x64_version_ok unsupported_os_native_view

  x64_version_ok:
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentBuildNumber"
  StrCmp $0 "3790" x64_build_ok unsupported_os_native_view

  x64_build_ok:
  ReadRegDWORD $0 HKLM "SYSTEM\CurrentControlSet\Control\Windows" "CSDVersion"
  IntCmp $0 0x200 supported_os unsupported_os_native_view unsupported_os_native_view

  check_x86:
  SetRegView 32
  ReadEnvStr $0 "PROCESSOR_ARCHITECTURE"
  StrCmp $0 "x86" x86_architecture_ok unsupported_os

  x86_architecture_ok:
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\ProductOptions" "ProductType"
  StrCmp $0 "WinNT" x86_workstation_ok unsupported_os

  x86_workstation_ok:
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"
  StrCmp $0 "5.1" x86_version_ok unsupported_os

  x86_version_ok:
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentBuildNumber"
  StrCmp $0 "2600" x86_build_ok unsupported_os

  x86_build_ok:
  ReadRegDWORD $0 HKLM "SYSTEM\CurrentControlSet\Control\Windows" "CSDVersion"
  IntCmp $0 0x300 x86_suite_check unsupported_os unsupported_os

  x86_suite_check:
  ; XP Professional has no consumer or specialized ProductSuite marker. This
  ; excludes Home, Starter, Media Center, Tablet PC, and embedded derivatives.
  ClearErrors
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\ProductOptions" "ProductSuite"
  IfErrors x86_media_center_check
  StrCmp $0 "" x86_media_center_check unsupported_os

  x86_media_center_check:
  ClearErrors
  ReadRegDWORD $0 HKLM "SYSTEM\WPA\MediaCenter" "Installed"
  IfErrors x86_tablet_check
  IntCmp $0 0 x86_tablet_check unsupported_os unsupported_os

  x86_tablet_check:
  ClearErrors
  ReadRegDWORD $0 HKLM "SYSTEM\WPA\TabletPC" "Installed"
  IfErrors supported_os
  IntCmp $0 0 supported_os unsupported_os unsupported_os

  supported_os:
  SetRegView 32
  Return

  unsupported_os_native_view:
  SetRegView 32
  unsupported_os:
  MessageBox MB_OK|MB_ICONSTOP \
    "eXPerience2K 3.1.0 supports only Windows XP Professional x86 Service Pack 3 and Windows XP Professional x64 Edition Service Pack 2.$\r$\n$\r$\nWindows XP Home, Starter, Media Center, Tablet PC, Embedded, IA-64, and every Windows Server edition are not supported.$\r$\n$\r$\nhttps://github.com/Somehowfreename/eXPerience2K$\r$\n$\r$\nNo files or settings have been changed."
  Abort
FunctionEnd

Function SelectCore
  ${If} ${RunningX64}
    StrCpy $CoreExe "$INSTDIR\eXPerience2KCore-x64.exe"
  ${Else}
    StrCpy $CoreExe "$INSTDIR\eXPerience2KCore-x86.exe"
  ${EndIf}
FunctionEnd

Section "eXPerience2K application" SecFiles
  SectionIn RO
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File /r "..\payload\*.*"
  File "..\build\eXPerience2K.exe"
  File "..\build\eXPerience2KCore-x86.exe"
  File "..\build\eXPerience2KCore-x64.exe"
  File "..\build\eXPerience2KExplorerBand32.dll"
  File "..\build\eXPerience2KExplorerBand64.dll"
  SetOutPath "$INSTDIR\Tools"
  File /oname=ResourceHacker.exe "..\tools\resource-hacker\ResourceHacker.exe"
  File /oname=ResourceHacker-ReadMe.txt "..\tools\resource-hacker\ReadMe.txt"
  SetOutPath "$INSTDIR\Source"
  File "..\src\eXPerience2KCore.c"
  File "..\src\eXPerience2KConfig.c"
  File "..\src\eXPerience2KImage.cpp"
  File "..\src\eXPerience2KImage.h"
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
