param(
  [string]$DeviceIp = "192.168.1.120",
  [string]$MapPath = "$PSScriptRoot\window-state-map.json",
  [int]$IntervalMs = 1200,
  [switch]$SendWindowTitle
)

$ErrorActionPreference = "Stop"

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class ActiveWindow {
  [DllImport("user32.dll")]
  public static extern IntPtr GetForegroundWindow();

  [DllImport("user32.dll")]
  public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int count);
}
"@

function Get-ActiveWindowInfo {
  $handle = [ActiveWindow]::GetForegroundWindow()
  if ($handle -eq [IntPtr]::Zero) {
    return $null
  }

  [uint32]$processId = 0
  [void][ActiveWindow]::GetWindowThreadProcessId($handle, [ref]$processId)

  $titleBuilder = New-Object System.Text.StringBuilder 512
  [void][ActiveWindow]::GetWindowText($handle, $titleBuilder, $titleBuilder.Capacity)

  $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
  if (-not $process) {
    return $null
  }

  [pscustomobject]@{
    Process = $process.ProcessName
    Title = $titleBuilder.ToString()
  }
}

function Test-RuleMatch {
  param(
    [pscustomobject]$Rule,
    [pscustomobject]$Window
  )

  if ($Rule.process -and $Rule.process.Count -gt 0) {
    $processMatched = $false
    foreach ($processName in $Rule.process) {
      if ($Window.Process -ieq $processName) {
        $processMatched = $true
        break
      }
    }

    if (-not $processMatched) {
      return $false
    }
  }

  if ($Rule.titleContains -and $Rule.titleContains.Count -gt 0) {
    foreach ($keyword in $Rule.titleContains) {
      if ($Window.Title.IndexOf($keyword, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
        return $true
      }
    }

    return $false
  }

  return $true
}

function Resolve-State {
  param(
    [pscustomobject]$Config,
    [pscustomobject]$Window
  )

  foreach ($rule in $Config.rules) {
    if (Test-RuleMatch -Rule $rule -Window $Window) {
      return $rule.state
    }
  }

  return $Config.defaultState
}

function Send-DeskPetState {
  param([string]$State)

  $body = @{ state = $State } | ConvertTo-Json -Compress
  Invoke-RestMethod -Uri "http://$DeviceIp/state" -Method Post -ContentType "application/json; charset=utf-8" -Body $body | Out-Null
}

function Send-DeskPetText {
  param([string]$Text)

  $body = @{ content = $Text } | ConvertTo-Json -Compress
  Invoke-RestMethod -Uri "http://$DeviceIp/text" -Method Post -ContentType "application/json; charset=utf-8" -Body ([System.Text.Encoding]::UTF8.GetBytes($body)) | Out-Null
}

if (-not (Test-Path -LiteralPath $MapPath)) {
  throw "Window state map not found: $MapPath"
}

$config = Get-Content -LiteralPath $MapPath -Raw -Encoding UTF8 | ConvertFrom-Json
$lastState = $null
$lastTitle = $null

Write-Host "DeskPet window monitor started. Device: $DeviceIp"
Write-Host "Press Ctrl+C to stop."

while ($true) {
  try {
    $window = Get-ActiveWindowInfo
    if ($window) {
      $state = Resolve-State -Config $config -Window $window
      if ($state -ne $lastState) {
        Send-DeskPetState -State $state
        Write-Host ("[{0}] {1} -> {2}" -f (Get-Date -Format "HH:mm:ss"), $window.Process, $state)
        $lastState = $state
      }

      if ($SendWindowTitle -and $window.Title -and $window.Title -ne $lastTitle) {
        Send-DeskPetText -Text $window.Title
        $lastTitle = $window.Title
      }
    }
  } catch {
    Write-Warning $_.Exception.Message
  }

  Start-Sleep -Milliseconds $IntervalMs
}
