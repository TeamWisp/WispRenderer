@echo off
set "str=%~1"
"C:\Program Files\cURL\bin\curl.exe" -X POST --data "{ \"content\": \"%str%\", \"username\": \"Jenkins\" }" -H "Content-Type: application/json" https://discordapp.com/api/webhooks/488672255321309185/GdU9rve6wW5rcbk6xcDx4TX8AKez5Yfej8kfKco17qRvHTlTuB6bdziQIDDYBHW8HuES
@echo on