/*
<javascriptresource>
<name>Color Sync...</name>
<menu>filter</menu>
<enableinfo>true</enableinfo>
<eventid>17aa86b8-0dcb-4824-b40b-49ca99ac4a71</eventid>
</javascriptresource>
*/
#target photoshop
(function(){
 var helper=new File(Folder.userData.fsName+'/ColorSync/ColorSyncLauncher.exe');
 if(!helper.exists){alert('Color Sync installation is incomplete. Reinstall the package.');return;}
 if(!helper.execute())alert('Windows could not start Color Sync.');
})();
