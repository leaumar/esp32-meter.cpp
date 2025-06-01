`Get-ChildItem -Path ./lib, ./src -Recurse -Include *.cpp, *.h | ForEach-Object { clang-format -i $_.FullName }`
