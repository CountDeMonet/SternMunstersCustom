Option Explicit
Dim shell, fso, project, launcher, command
Set shell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")
project = fso.GetParentFolderName(WScript.ScriptFullName)
launcher = fso.BuildPath(project, "Launch Spot PAD bootstrap.pyw")
command = Chr(34) & "D:\Program Files\Pinball Asset Decryptor\python\pythonw.exe" & Chr(34) & " " & Chr(34) & launcher & Chr(34)
shell.Run command, 0, False

