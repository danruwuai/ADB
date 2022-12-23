/*&cls
@echo off
cd /d "%~dp0"
set "in=sweet_sunny_ov16a1q_front_i_lsc_OTP.txt"
set "out=LSC.txt"
cscript -nologo -e:jscript "%~f0" "%in%">"%out%"
pause&exit /b
*/
var fso=new ActiveXObject('Scripting.FileSystemObject');
var file=fso.OpenTextFile(WSH.Arguments(0),1);
var f=false;
var result=[];
var n=0;
while(!file.AtEndOfStream){
    var line=file.ReadLine().replace(/^\s*|\s*$/g,'');
    if(f){
        var arr=line.split(/\s+/);
        for(var i=0;i<arr.length;i++){
            //WSH.echo(arr[i]);
            var num=arr[i].split('.');
            result[n].push(num[0].toString());
        }
    }
    if(line=='r_gain:'){
        result.push([]);
        f=true;
    }
    if(line=='gr_gain:'){
        result.push([]);
        f=true;
        n++;
    }
    if(line=='gb_gain:'){
        result.push([]);
        f=true;
        n++;
    }
    if(line=='b_gain:'){
        result.push([]);
        f=true;
        n++;
    }
    if(line==''){
        f=false;
    }
}
file.Close();
for(var i=0;i<result[0].length;i++){
    var s=[];
    for(var j=0;j<result.length;j++){
        s.push(result[j][i])
    }
    WSH.echo(s.join(' '));
}
