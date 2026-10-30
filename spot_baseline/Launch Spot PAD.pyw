"""PAD with the project-local Munsters Spot SDK; --check-build tests its real build path."""
import multiprocessing
import os
from pathlib import Path
import sys

def configure():
    project=Path(__file__).resolve().parent
    installed=Path(r'D:\Program Files\Pinball Asset Decryptor')
    sdk=project/'analysis'/'stock_spot'/'sdk'
    if not (sdk/'pad_munsters.h').is_file():
        raise RuntimeError('The project-local Spot SDK is missing: '+str(sdk))
    sys.path.insert(0,str(installed))
    from pinball_decryptor.plugins.stern import mode_runtime,mode_write
    mode_runtime._SDK_DIR=str(sdk)
    mode_runtime.sdk_dir=lambda: str(sdk)
    from pinball_decryptor.plugins.stern import mode_project
    mode_project.PORTS_DIR=str(sdk/"ports")
    # Rig installation scripts remain in the application, beside its original SDK.
    mode_write.tools_dir=lambda: str(installed/'tools'/'spike2_emu')
    # Code-only GUI Try it uses its own compiler command, separate from Write.
    from pinball_decryptor.webui import modes_tryit
    original_compile=modes_tryit.TryItMixin.compile_cmd
    def compile_with_local_sdk(self,out,sources):
        cmd=list(original_compile(self,out,sources))
        old=self._linux_path(str(installed/'tools'/'spike2_emu'/'modes'/'sdk'/'build_mode.sh'))
        new=self._linux_path(str(sdk/'build_mode.sh'))
        if cmd.count(old)!=1:
            raise RuntimeError('Cannot identify the GUI compiler script in argv: '+repr(cmd))
        return [new if arg==old else arg for arg in cmd]
    modes_tryit.TryItMixin.compile_cmd=compile_with_local_sdk
    os.environ['PAD_CODE_CACHE']='0'
    os.chdir(project)
    return project,sdk,mode_runtime,mode_write

if __name__ == '__main__':
    multiprocessing.freeze_support()
    project,sdk,runtime,writer=configure()
    if '--check-gui-build' in sys.argv:
        from pinball_decryptor.webui import modes_tryit,emulate_rig
        import subprocess,json
        class GuiCompiler(modes_tryit.TryItMixin):
            def _rig_cmd(self,script,*args):
                return emulate_rig.rig_cmd(script,*args)
        panel=GuiCompiler()
        output=project/'analysis'/'stock_spot'/'gui_compiler_check.so'
        source=project/'modes'/'spot_baseline'/'spot_baseline.c'
        cmd=panel.compile_cmd(str(output),[str(source)])
        print(json.dumps(cmd,indent=2),flush=True)
        result=subprocess.run(cmd,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
        print(result.stdout+result.stderr,flush=True)
        if result.returncode:sys.exit(result.returncode)
        print('GUI-specific Try it compiler verified: '+str(output),flush=True)
        sys.exit(0)
    if '--check-build' in sys.argv:
        output=project/'analysis'/'stock_spot'/'launcher_check.so'
        source=project/'modes'/'spot_baseline'/'spot_baseline.c'
        print('SPOT SDK: '+str(sdk),flush=True)
        print('SPOT PORT: '+str(runtime.port_file('munsters_pro','1.28.0')),flush=True)
        writer.compile_code_object([str(source)],str(output),log=lambda msg,*a: print(msg,flush=True))
        print('Try it compiler path verified: '+str(output),flush=True)
        sys.exit(0)
    from pinball_decryptor.app import App
    original_title=App._refresh_title
    def title_with_sdk(self,*args,**kwargs):
        result=original_title(self,*args,**kwargs)
        title=self.root.title()
        if '[SPOT SDK]' not in title:self.root.title(title+' [SPOT SDK]')
        return result
    App._refresh_title=title_with_sdk
    from pinball_decryptor.webui import host
    original_build=host.build
    def build_with_sdk(*args,**kwargs):
        ctx,server=original_build(*args,**kwargs)
        ctx.loop.post(ctx.window.append_log,'[spot-sdk] Local Munsters SDK enabled: '+str(sdk),'info')
        return ctx,server
    host.build=build_with_sdk
    sys.exit(host.main(sys.argv[1:]))


