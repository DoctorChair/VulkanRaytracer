echo off

set inputPath=%1
set exe=%2
set outputPath=%3

echo Writing spir-v files to %outputPath%

for %%I in (%inputPath%\*) do (

     if %%~xI==.frag (
        echo -------------------------------------------------------------------------------------------------------------------------------------------------------------------
        echo Compiling "%inputPath%/%%~nI%%~xI"
        echo "%exe% -V %inputPath%/%%~nI%%~xI -o %outputPath%/%%~nIFRAG.spv"
        %exe% -V "%inputPath%/%%~nI%%~xI" -o "%outputPath%/%%~nIFRAG.spv"
        echo Compiled in "%outputPath%/%%~nIFRAG.spv"
        echo -------------------------------------------------------------------------------------------------------------------------------------------------------------------
    )

    if %%~xI==.vert (
        echo -------------------------------------------------------------------------------------------------------------------------------------------------------------------
        echo Compiling "%inputPath%/%%~nI%%~xI"
        echo "%exe% -V %inputPath%/%%~nI%%~xI -o %outputPath%/%%~nIVERT.spv"
        %exe% -V "%inputPath%/%%~nI%%~xI" -o "%outputPath%/%%~nIVERT.spv"
        echo Compiled in "%outputPath%/%%~nIVERT.spv"
        echo ------------------------------------------------------------------------------------------------------------------------------------------------------------------- 
    )

    if %%~xI==.comp (
        echo -------------------------------------------------------------------------------------------------------------------------------------------------------------------
        echo Compiling "%inputPath%/%%~nI%%~xI"
        echo "%exe% -V %inputPath%/%%~nI%%~xI -o %outputPath%/%%~nICOMP.spv"
        %exe% -V "%inputPath%/%%~nI%%~xI" -o "%outputPath%/%%~nICOMP.spv"
        echo Compiled in "%outputPath%/%%~nICOMP.spv"
        echo -------------------------------------------------------------------------------------------------------------------------------------------------------------------
    ) 
)