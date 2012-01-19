@rem msbuild cefclient.vcxproj /t:clean /p:Configuration=Release /p:Platform=Win32
msbuild cefclient.vcxproj /p:Configuration=Release /p:Platform=Win32
copy Release\cefclient.exe ..\tilemill\node_modules\topcube\cefclient\cefclient.exe