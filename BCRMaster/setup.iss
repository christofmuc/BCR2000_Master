; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "BCRMaster"
#define MyAppVersion "1.0.4"
#define MyAppPublisher "Christof Ruch Beratungs UG (haftungsbeschraenkt)"
#define MyAppURL "https://github.com/christofmuc/BCR2000_Master/"
#define MyAppExeName "BCRMaster.exe"
#define VCRedistFileName "vc_redist.x64.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{63A67AB7-AC93-48F0-B5D5-658AC7A10476}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=${CMAKE_CURRENT_SOURCE_DIR}/redist/agpl-3.0.txt
; Remove the following line to run in administrative install mode (install for all users.)
PrivilegesRequired=lowest
OutputDir=${CMAKE_CURRENT_BINARY_DIR}
OutputBaseFilename=bcrmaster_setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "${CMAKE_CURRENT_BINARY_DIR}\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "${VCREDIST_PATH}\{#VCRedistFileName}"; DestDir: {tmp}; Flags: dontcopy
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
; VC++ redistributable runtime. Extracted by VC2017RedistNeedsInstall(), if needed.

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
Filename: "{tmp}\{#VCRedistFileName}"; StatusMsg: "Installing required VS2017 C++ Runtime Libraries"; Parameters: "/quiet"; Check: VC2017RedistNeedsInstall ; Flags: waituntilterminated

[Code]
function VC2017RedistNeedsInstall: Boolean;
var 
  Version: String;
begin
  if (RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Version', Version)) then
  begin
    // Is the installed version at least the same I used for development?
    Log('VC Redist Version check : found ' + Version);
    Result := (CompareStr(Version, 'v14.16.27012.1')<0);
    if (not Result) then
    begin
      Log('Skipping installation of new VC Redist Version');
    end
  end
  else 
  begin
    // Not even an old version installed
    Result := True;
  end;
  if (Result) then
  begin
    ExtractTemporaryFile('{#VCRedistFileName}');
  end;
end;
