using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;
using System.Web.Script.Serialization;
static class Launcher {
 static object Call(object target,string name,params object[] args){return target.GetType().InvokeMember(name,BindingFlags.InvokeMethod,null,target,args);}
 static void Log(string state,string message){try{string dir=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"ColorSync");Directory.CreateDirectory(dir);File.WriteAllText(Path.Combine(dir,"last-run.json"),new JavaScriptSerializer().Serialize(new {state=state,message=message,time=DateTime.UtcNow.ToString("o")}));}catch{}}
 [STAThread]static int Main(){
  bool owner;using(var gate=new Mutex(true,"Local\\ColorSync-"+Environment.UserName,out owner)){
   if(!owner){MessageBox.Show("Color Sync is already running. Close its Photoshop dialog first.","Color Sync");return 2;}
   object app=null,descriptor=null;
   try{
    string payload=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"ColorSync.psjs");
    if(!File.Exists(payload))throw new FileNotFoundException("ColorSync.psjs is missing. Reinstall the complete package.");
    // Connect only to the already running Photoshop instance; do not start one.
    app=Marshal.GetActiveObject("Photoshop.Application");
    descriptor=Activator.CreateInstance(Type.GetTypeFromProgID("Photoshop.ActionDescriptor",true));
    int pathKey=Convert.ToInt32(Call(app,"StringIDToTypeID","javaScript"));
    int eventId=Convert.ToInt32(Call(app,"StringIDToTypeID","AdobeScriptAutomation Scripts"));
    Call(descriptor,"PutPath",pathKey,payload);
    Log("running",payload);
    // The JSX entry has returned before Photoshop accepts the queued COM call.
    Call(app,"ExecuteAction",eventId,descriptor,1);
    Log("finished",payload);return 0;
   }catch(Exception e){while(e.InnerException!=null)e=e.InnerException;Log("error",e.Message);MessageBox.Show(e.Message,"Color Sync could not start",MessageBoxButtons.OK,MessageBoxIcon.Error);return 1;}
   finally{if(descriptor!=null)Marshal.FinalReleaseComObject(descriptor);if(app!=null)Marshal.FinalReleaseComObject(app);gate.ReleaseMutex();}
  }
 }
}
