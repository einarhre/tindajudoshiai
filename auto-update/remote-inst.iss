[Setup]
AppName=RemoteInstall
AppVerName=fill in the correct value
AppPublisher=Oh2ncp
AppPublisherURL=http://oh2ncp.kolumbus.fi
AppSupportURL=http://oh2ncp.kolumbus.fi
AppUpdatesURL=http://oh2ncp.kolumbus.fi
DefaultDirName={pf}\JudoShiai
DefaultGroupName=JudoShiai
OutputDir=OBJDIR
OutputBaseFilename=remote-install
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
PrivilegesRequired=none
SetupLogging=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
;Name: "swedish"; MessagesFile: "compiler:Languages\Swedish.isl"
;Name: "estonian"; MessagesFile: "compiler:Languages\Estonian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
english.BeveledLabel=English
finnish.BeveledLabel=Suomi
;swedish.BeveledLabel=Svenska
;estonian.BeveledLabel=Eesti
spanish.BeveledLabel=Español

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "OBJDIR\auto-update.exe"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "RELDIR\judoshiai\filecount.txt"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\RemoteInstall"; Filename: "{app}\bin\auto-update.exe"
Name: "{commondesktop}\RemoteInstall"; Filename: "{app}\bin\auto-update.exe"; Tasks: desktopicon
Name: "{group}\JudoTimer"; Filename: "{app}\bin\judotimer.exe"
Name: "{group}\JudoInfo"; Filename: "{app}\bin\judoinfo.exe"
Name: "{group}\JudoWeight"; Filename: "{app}\bin\judoweight.exe"
Name: "{group}\JudoJudogi"; Filename: "{app}\bin\judojudogi.exe"
Name: "{group}\Remove installation"; Filename: "{app}\unins000.exe"
Name: "{commondesktop}\JudoTimer"; Filename: "{app}\bin\judotimer.exe"; Tasks: desktopicon
Name: "{commondesktop}\JudoInfo"; Filename: "{app}\bin\judoinfo.exe"; Tasks: desktopicon
Name: "{commondesktop}\JudoWeight"; Filename: "{app}\bin\judoweight.exe"; Tasks: desktopicon
Name: "{commondesktop}\JudoJudogi"; Filename: "{app}\bin\judojudogi.exe"; Tasks: desktopicon

;[Run]
;Filename: "{app}\bin\auto-update.exe"; Parameters: "{code:GetIpAddr} ""{app}"""; Description: "{cm:LaunchProgram,AutoUpdate}"; Flags: nowait postinstall


[Code]
var
  lblIpAddr: TLabel;
  tbIpAddr: TEdit;
  lblMsg: TLabel;
  FileLines: TArrayOfString;
  ProgressPage: TOutputProgressWizardPage;

function StrSplit(Text: String; Separator: String): TArrayOfString;
var
  i, p: Integer;
  Dest: TArrayOfString; 
begin
  i := 0;
  repeat
    SetArrayLength(Dest, i+1);
    p := Pos(Separator,Text);
    if p > 0 then begin
      Dest[i] := Copy(Text, 1, p-1);
      Text := Copy(Text, p + Length(Separator), Length(Text));
      i := i + 1;
    end else begin
      Dest[i] := Text;
      Text := '';
    end;
  until Length(Text)=0;
  Result := Dest
end;

function JudoShiaiAddr_NextButtonClick({Page: TWizardPage}): Boolean;
var
  s: string;
  ResultCode: Integer;
  len, prevlen: Integer;
  oneline: AnsiString;
  filecount: Integer;
begin
  s := Trim(tbIpAddr.Text);
  if (s = '') then
  begin
    MsgBox('Please fill in a valid IP address', mbError, MB_OK);
    Result := false;
  end else if (s = '0.0.0.0') then
  begin
    MsgBox('Please fill in a valid IP address', mbError, MB_OK);
    Result := false;
  end else
  begin
    lblMsg.Caption := 'Starting ' +
         ExpandConstant('{app}\bin\auto-update.exe') + ' ' +
	       ExpandConstant('{code:GetIpAddr} "{app}"') + ' home:' +
	       ExpandConstant('{app}');

    if (Exec(ExpandConstant('{app}\bin\auto-update.exe'),
        ExpandConstant('{code:GetIpAddr} "{app}"'),
        ExpandConstant('{app}'), SW_SHOW,
        ewNoWait {ewWaitUntilTerminated}, ResultCode)) then
    begin
      Sleep(500);
      prevlen := 0;
      len := -1;
      if FileExists(ExpandConstant('{app}\progr.txt')) then
      begin
        if LoadStringFromFile(ExpandConstant('{app}\filecount.txt'), oneline) then filecount := StrToInt(oneline)
        else filecount := 1000;
        ProgressPage.SetText('Getting JudoShiai files...', '');
        ProgressPage.SetProgress(0, 0);
        ProgressPage.Show;
        try
          repeat
            prevlen := len;
            if LoadStringFromFile(ExpandConstant('{app}\progr.txt'), oneline) then len := StrToInt(oneline)
            else len := 1;
            if len <= filecount then ProgressPage.SetProgress(len, filecount);
            Sleep(250);
          until (len >= 1000000);
        finally
          ProgressPage.Hide;
        end;
      end;
      Result := True
    end else Result := False;
  end;
  Result := True;
end;

procedure JudoShiaiAddr_Activate(Page: TWizardPage);
var
  s: string;
begin
  s := Trim(tbIpAddr.Text);
  if (s = '') then
  begin
    tbIpAddr.Text := '127.0.0.1';
  end;
end;

function JudoShiaiAddr_CreatePage(PreviousPageId: Integer): Integer;
var
  Page: TWizardPage;
begin
  Page := CreateCustomPage(
    PreviousPageId, 'Remote installation', 'Install from remote JudoShiai');

  lblIpAddr := TLabel.Create(Page);
  with lblIpAddr do
  begin
    Parent := Page.Surface;
    Caption := 'IP address of JudoShiai:';
    Left := ScaleX(8);
    Top := ScaleY(8);
    Width := ScaleX(167);
    Height := ScaleY(13);
  end;

  tbIpAddr := TEdit.Create(Page);
  with tbIpAddr do
  begin
    Parent := Page.Surface;
    Left := ScaleX(8);
    Top := ScaleY(24);
    Width := ScaleX(401);
    Height := ScaleY(21);
    TabOrder := 0;
  end;

  lblMsg := TLabel.Create(Page);
  with lblMsg do
  begin
    Parent := Page.Surface;
    Caption := ' ';
    Left := ScaleX(8);
    Top := ScaleY(48);
    Width := ScaleX(167);
    Height := ScaleY(13);
  end;

  ProgressPage := CreateOutputProgressPage('JudoShiai files download','');

  with Page do
  begin
    OnActivate := @JudoShiaiAddr_Activate;
    {OnNextButtonClick := @JudoShiaiAddr_NextButtonClick;}
  end;

  Result := Page.ID;
end;

function GetIPAddr(param: String): String;
begin
  Result := Trim(tbIpAddr.Text);
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = wpSelectTasks then
  begin
    JudoShiaiAddr_CreatePage(CurPageID);
    Result := True;
  end else if CurPageID = wpReady then
  begin
    JudoShiaiAddr_NextButtonClick();
    Result := True;
  end else
    Result := True;
end;

procedure InitializeWizard();
begin
end;
