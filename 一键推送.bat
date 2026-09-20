@echo off
REM ============================================================
REM Plotopia 一键推送脚本（直连版，不使用代理）
REM 说明：本机可直连 GitHub，无需 Clash 代理。
REM       首次使用需生成 Personal Access Token。
REM ============================================================

echo.
echo ============================================
echo   Plotopia 推送脚本
echo ============================================
echo.

echo [1/2] 请输入 Personal Access Token
echo   生成地址: https://github.com/settings/tokens
echo   选择 Tokens (classic) -^> Generate new token -^> 勾选 repo
echo.
set /p TOKEN=粘贴 Token 后按回车: 
if "%TOKEN%"=="" (
    echo 错误: Token 不能为空。
    pause
    exit /b 1
)

echo.
echo [2/2] 正在推送...
git push https://kimoji927:%TOKEN%@github.com/kimoji927/Plotopia.git main
set RESULT=%ERRORLEVEL%

if not "%RESULT%"=="0" (
    echo.
    echo ============================================
    echo   推送失败，请检查:
    echo   1. 网络能否打开 github.com
    echo   2. Token 是否有效、是否勾选了 repo 权限
    echo   3. 把报错信息发给 AI 助手排查
    echo ============================================
    pause
    exit /b 1
)

echo.
echo ============================================
echo   推送成功！
echo   项目地址: https://github.com/kimoji927/Plotopia
echo ============================================
pause
exit /b 0