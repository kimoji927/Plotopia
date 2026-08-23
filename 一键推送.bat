@echo off
chcp 65001 >nul
REM ============================================================
REM Plotopia 推送脚本 (Clash 专用版)
REM 使用前：
REM   1. 先打开 Clash（确保系统代理或 TUN 模式开启）
REM   2. 准备好 Personal Access Token（生成方法见下方提示）
REM ============================================================

echo.
echo ============================================
echo   Plotopia 一键推送 (Clash + Token)
echo ============================================
echo.

echo [1/3] 配置 Clash 代理 (127.0.0.1:7890)...
git config http.proxy http://127.0.0.1:7890
git config https.proxy http://127.0.0.1:7890
echo   代理已设置

echo.
echo [2/3] 输入 Personal Access Token
echo   生成方法: GitHub网页 -> 头像 -> Settings -> Developer settings
echo            -> Personal access tokens -> Tokens (classic)
echo            -> Generate new token -> 勾选 repo -> 生成并复制 ghp_ 开头字符串
set /p TOKEN=请粘贴 Token 后回车: 
if "%TOKEN%"=="" (
    echo   错误：Token 不能为空！
    goto :error
)

echo.
echo [3/3] 推送中...
git remote set-url origin https://kimoji927:%TOKEN%@github.com/kimoji927/Plotopia.git
git push origin main
set PUSH_RESULT=%ERRORLEVEL%
REM 推送后立刻移除 URL 中的 Token，防止泄露
git remote set-url origin https://github.com/kimoji927/Plotopia.git
REM 清理代理设置（可选，保留也行）
REM git config --unset http.proxy
REM git config --unset https.proxy

if not "%PUSH_RESULT%"=="0" goto :error

echo.
echo ============================================
echo   推送成功！
echo   项目地址: https://github.com/kimoji927/Plotopia
echo ============================================
echo.
pause
exit /b 0

:error
echo.
echo ============================================
echo   推送失败！常见原因：
echo   1. Clash 没打开 / 没开启系统代理 -> 打开 Clash 后重试
echo   2. Clash 端口不是 7890 -> 检查 Clash 设置里的端口
echo   3. Token 无效或没勾选 repo -> 重新生成
echo   4. 把上面的报错信息发给我排查
echo ============================================
echo.
pause
exit /b 1
