param(
    [ValidateSet("build", "watch", "clean", "distclean")]
    [string]$Task = "build"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

switch ($Task) {
    "build"     { latexmk -r latexmkrc main.tex }
    "watch"     { latexmk -pvc -r latexmkrc main.tex }
    "clean"     { latexmk -r latexmkrc -c main.tex }
    "distclean" { latexmk -r latexmkrc -C main.tex }
}