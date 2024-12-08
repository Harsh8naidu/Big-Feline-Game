@REM Set the path to the executable (change this path if needed)
set EXE_PATH=.\x64\Debug\CSC8503.exe

@REM This will run the server
start cmd /k %EXE_PATH% server

@REM This will run the client
start "" %EXE_PATH%