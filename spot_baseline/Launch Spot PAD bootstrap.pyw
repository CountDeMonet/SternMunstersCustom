"""Visible error reporting for the no-console Spot PAD launcher."""
from pathlib import Path
import ctypes
import runpy
import sys
import traceback
if __name__ == '__main__':
    project=Path(__file__).resolve().parent
    try:
        runpy.run_path(str(project/'Launch Spot PAD.pyw'),run_name='__main__')
    except SystemExit as e:
        if e.code not in (None,0):
            error=project/'analysis'/'stock_spot'/'launcher_error.log'
            error.write_text('Launcher exited with status '+str(e.code),encoding='utf-8')
            ctypes.windll.user32.MessageBoxW(None,'PAD could not start. Details: '+str(error),'Spot PAD startup error',16)
    except BaseException:
        error=project/'analysis'/'stock_spot'/'launcher_error.log'
        error.write_text(traceback.format_exc(),encoding='utf-8')
        ctypes.windll.user32.MessageBoxW(None,'PAD could not start. Details: '+str(error),'Spot PAD startup error',16)
