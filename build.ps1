Write-Host "Build in process!"

# path to my vs, youll need to change or use the x64 command prompt for dev thing
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if (Test-Path $vsPath) {
    Write-Host "Building.."
    cmd /c "`"$vsPath`" && set > env.tmp"
    Get-Content env.tmp | ForEach-Object {
        if ($_ -match '^(.+?)=(.*)$') {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2])
        }
    }
    Remove-Item env.tmp -ErrorAction SilentlyContinue
    
    
    Write-Host "Compiling with cl.exe..."
    cmd /c "cl /EHsc /Fe:InsurgencyPatcher.exe main.cpp user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib 2>&1"
    
    if (Test-Path "InsurgencyPatcher.exe") {
        Write-Host "Build successful!"
        Write-Host "Executable: InsurgencyPatcher.exe"
    } else {
        Write-Host "Build failed with cl.exe"
    }
} else {
    Write-Host "Visual Studio 2022 Community not found."
    
    # actually built in checker for various more vs versions
    $vsPaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    )
    
    $found = $false
    foreach ($path in $vsPaths) {
        if (Test-Path $path) {
            Write-Host "Did you perhaps mean $path"
            $found = $true
            break
        }
    }
    
    if (-not $found) {
        Write-Host "No Visual Studio installation found. Please install Visual Studio with C++ development tools package, it is required!"
    }
}

Write-Host "Press any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
