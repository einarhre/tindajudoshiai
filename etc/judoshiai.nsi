Unicode true

!ifndef SHIAI_VER_NUM
  !define SHIAI_VER_NUM "unknown"
!endif

!ifndef TGTEXT
  !define TGTEXT ""
!endif

!ifndef BUILD_KIND
  !define BUILD_KIND ""
!endif

!ifndef RELEASEDIR
  !error "RELEASEDIR must be defined"
!endif

!ifndef OUTFILE
  !define OUTFILE "judoshiai-setup.exe"
!endif

Name "JudoShiai ${SHIAI_VER_NUM}"
OutFile "${OUTFILE}"

InstallDir "$LOCALAPPDATA\JudoShiai${TGTEXT}"
InstallDirRegKey HKCU "Software\JudoShiai${TGTEXT}" "InstallDir"

RequestExecutionLevel user

SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show

!include MUI2.nsh

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\judoshiai.exe"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "JudoShiai" SecMain
  SectionIn RO

  SetOutPath "$INSTDIR"
  File /r "${RELEASEDIR}/judoshiai/*"

  WriteRegStr HKCU "Software\JudoShiai${TGTEXT}" "InstallDir" "$INSTDIR"
  WriteUninstaller "$INSTDIR\uninstall.exe"

  CreateDirectory "$SMPROGRAMS\JudoShiai${TGTEXT}"

  CreateShortcut "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoShiai.lnk" "$INSTDIR\bin\judoshiai.exe"
  CreateShortcut "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoTimer.lnk" "$INSTDIR\bin\judotimer.exe"
  CreateShortcut "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoInfo.lnk" "$INSTDIR\bin\judoinfo.exe"
  CreateShortcut "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoWeight.lnk" "$INSTDIR\bin\judoweight.exe"
  CreateShortcut "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoJudogi.lnk" "$INSTDIR\bin\judojudogi.exe"
  CreateShortcut "$SMPROGRAMS\JudoShiai${TGTEXT}\Uninstall.lnk" "$INSTDIR\uninstall.exe"

  ; Per-user .shi association. This does not need administrator rights.
  WriteRegStr HKCU "Software\Classes\.shi" "" "JudoShiaiDatabaseFile"
  WriteRegStr HKCU "Software\Classes\JudoShiaiDatabaseFile" "" "JudoShiai Database File"
  WriteRegStr HKCU "Software\Classes\JudoShiaiDatabaseFile\DefaultIcon" "" "$INSTDIR\bin\judoshiai.exe,0"
  WriteRegStr HKCU "Software\Classes\JudoShiaiDatabaseFile\shell\open\command" "" '"$INSTDIR\bin\judoshiai.exe" "%1"'

  ; Add uninstall entry to Windows Apps/Programs list for the current user.
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JudoShiai${TGTEXT}" "DisplayName" "JudoShiai ${SHIAI_VER_NUM}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JudoShiai${TGTEXT}" "DisplayVersion" "${SHIAI_VER_NUM}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JudoShiai${TGTEXT}" "Publisher" "JudoShiai"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JudoShiai${TGTEXT}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JudoShiai${TGTEXT}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JudoShiai${TGTEXT}" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JudoShiai${TGTEXT}" "NoRepair" 1

  System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd

Section /o "Desktop shortcuts" SecDesktop
  CreateShortcut "$DESKTOP\JudoShiai.lnk" "$INSTDIR\bin\judoshiai.exe"
  CreateShortcut "$DESKTOP\JudoTimer.lnk" "$INSTDIR\bin\judotimer.exe"
  CreateShortcut "$DESKTOP\JudoInfo.lnk" "$INSTDIR\bin\judoinfo.exe"
  CreateShortcut "$DESKTOP\JudoWeight.lnk" "$INSTDIR\bin\judoweight.exe"
  CreateShortcut "$DESKTOP\JudoJudogi.lnk" "$INSTDIR\bin\judojudogi.exe"
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\JudoShiai.lnk"
  Delete "$DESKTOP\JudoTimer.lnk"
  Delete "$DESKTOP\JudoInfo.lnk"
  Delete "$DESKTOP\JudoWeight.lnk"
  Delete "$DESKTOP\JudoJudogi.lnk"

  Delete "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoShiai.lnk"
  Delete "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoTimer.lnk"
  Delete "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoInfo.lnk"
  Delete "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoWeight.lnk"
  Delete "$SMPROGRAMS\JudoShiai${TGTEXT}\JudoJudogi.lnk"
  Delete "$SMPROGRAMS\JudoShiai${TGTEXT}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\JudoShiai${TGTEXT}"

  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JudoShiai${TGTEXT}"
  DeleteRegKey HKCU "Software\JudoShiai${TGTEXT}"

  DeleteRegKey HKCU "Software\Classes\JudoShiaiDatabaseFile"
  DeleteRegValue HKCU "Software\Classes\.shi" ""

  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR"

  System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd
