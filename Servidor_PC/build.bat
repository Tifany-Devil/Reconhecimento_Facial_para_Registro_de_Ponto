@echo off
echo [COMPILANDO] cadastro_http.cpp

REM Caminhos para os includes e libs da libcurl
set INCLUDE_CURL=.\cURL\include
set LIB_CURL=.\cURL\lib

REM Compilação com Mongoose, libcurl e biblioteca de sockets do Windows (ws2_32)
g++ cadastro_http.cpp mongoose.c -o cadastro_http.exe -I%INCLUDE_CURL% -L%LIB_CURL% -lcurl -lws2_32

IF %ERRORLEVEL% NEQ 0 (
    echo [ERRO] Falha na compilação!
) ELSE (
    echo [OK] Compilação concluída com sucesso.
)
pause
