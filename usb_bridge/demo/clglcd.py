#
# Python classes for IPC communication
# with clglcd_usb_host process
#

import mmap, time, ctypes
from ctypes import wintypes, Structure, sizeof, c_char, c_byte, c_uint32, c_int32

SUPPORTED_VERSION    = b'CLGLCD10'

#
# Named objects from CLGLCD USB host process
#
CLGLCD_MUTEX_NAME = "Global\\CLGLCD_MUTEX"
CLGLCD_SHM_NAME   = "Local\\CLGLCD_SHM"

# Adopted from: 
# http://code.activestate.com/recipes/577794-win32-named-mutex-class-for-system-wide-mutex/
# To use OpenMutex instead CreateMutex

MUTEX_ALL_ACCESS     = 0x1F0001
MUTEX_MODIFY_STATE   = 0x000001
SYNCHRONIZE          = 0x100000

ERROR_FILE_NOT_FOUND = 0x02

"""Named mutex handling (for Win32)."""
# Create ctypes wrapper for Win32 functions we need, with correct argument/return types
_CreateMutex = ctypes.windll.kernel32.CreateMutexA
_CreateMutex.argtypes = [wintypes.LPCVOID, wintypes.BOOL, wintypes.LPCSTR]
_CreateMutex.restype = wintypes.HANDLE

#ctypes.windll.kernel32.OpenMutexW.argtypes = [ctypes.wintypes.DWORD, ctypes.wintypes.BOOL, ctypes.wintypes.LPCWSTR]
_OpenMutex = ctypes.windll.kernel32.OpenMutexW
_OpenMutex.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
_OpenMutex.restype = wintypes.HANDLE

_WaitForSingleObject = ctypes.windll.kernel32.WaitForSingleObject
_WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
_WaitForSingleObject.restype = wintypes.DWORD

_ReleaseMutex = ctypes.windll.kernel32.ReleaseMutex
_ReleaseMutex.argtypes = [wintypes.HANDLE]
_ReleaseMutex.restype = wintypes.BOOL

_CloseHandle = ctypes.windll.kernel32.CloseHandle
_CloseHandle.argtypes = [wintypes.HANDLE]
_CloseHandle.restype = wintypes.BOOL

class NamedMutex(object):
    """A named, system-wide mutex that can be acquired and released."""

    def __init__(self, name, access=SYNCHRONIZE, acquired=False):
        """Create named mutex with given name, also acquiring mutex if acquired is True.
        Mutex names are case sensitive, and a filename (with backslashes in it) is not a
        valid mutex name. Raises WindowsError on error.
        
        """
        self.name = name
        self.acquired = acquired
        self.handle = None
        ret = _OpenMutex(access, False, name)
        if not ret:
            # TODO: Friendly message for ERROR_FILE_NOT_FOUND
            err = ctypes.GetLastError()
            if (err == ERROR_FILE_NOT_FOUND):
                raise Exception("Unable to open mutex. CLGLCD USB host process is not running ?")
            raise ctypes.WinError()
        self.handle = ret
        if acquired:
            self.acquire()

    def acquire(self, timeout=None):
        """Acquire ownership of the mutex, returning True if acquired. If a timeout
        is specified, it will wait a maximum of timeout seconds to acquire the mutex,
        returning True if acquired, False on timeout. Raises WindowsError on error.
        
        """
        if timeout is None:
            # Wait forever (INFINITE)
            timeout = 0xFFFFFFFF
        else:
            timeout = int(round(timeout * 1000))
        ret = _WaitForSingleObject(self.handle, timeout)
        if ret in (0, 0x80):
            # Note that this doesn't distinguish between normally acquired (0) and
            # acquired due to another owning process terminating without releasing (0x80)
            self.acquired = True
            return True
        elif ret == 0x102:
            # Timeout
            self.acquired = False
            return False
        else:
            # Waiting failed
            raise ctypes.WinError()

    def release(self):
        """Relase an acquired mutex. Raises WindowsError on error."""
        ret = _ReleaseMutex(self.handle)
        if not ret:
            raise ctypes.WinError()
        self.acquired = False

    def close(self):
        """Close the mutex and release the handle."""
        if self.handle is None:
            # Already closed
            return
        ret = _CloseHandle(self.handle)
        if not ret:
            raise ctypes.WinError()
        self.handle = None

    __del__ = close

    def __repr__(self):
        """Return the Python representation of this mutex."""
        return '{0}({1!r}, acquired={2})'.format(
                self.__class__.__name__, self.name, self.acquired)

    __str__ = __repr__

    # Make it a context manager so it can be used with the "with" statement
    def __enter__(self):
        self.acquire()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.release()


class CLGLCD_SHM(Structure):
    """ Shared memory structure for communication
    with clglcd_usb_host process"""
    _fields_ = [
        # USB Host ID 
        ("version", c_char * 8),
        ("usb_host_ts", c_uint32),
        ("reserved1", c_byte * 52),
        # Touchpanel
        ("touch_ts", c_uint32),
        ("touch_x", c_int32),
        ("touch_y", c_int32),
        ("touch_p", c_uint32),
        ("raw_touch_x", c_int32),
        ("raw_touch_y", c_int32),
        ("raw_touch_z1", c_int32),
        ("raw_touch_z2", c_int32),
        ("raw_touch_cnt", c_int32),
        ("reserved2", c_byte * 28),
        # Reserved
        ("reserved3", c_byte * 64),
        # Frame buffer control
        ("active_ts", c_uint32),
        ("active_frame", c_int32),
        ("reserved4", c_byte * 56),
        # Frame buffers
        ("frame", ((c_byte * (240 * 320)) * 2))
    ]

class IPC:

    def __init__(self, open = True):
        self.mutex = None
        self.shm_map  = None
        self.shm = None
        self.version = None
        self.current_frame = -1
        if (open): self.open()

    def open(self):
        self.mutex = NamedMutex(CLGLCD_MUTEX_NAME)
        if not self.mutex.acquire(1):
            raise ctypes.WinError("Unable to acquire CLGLCD mutex. Is device used by other process?")
        self.shm_map = mmap.mmap(0, sizeof(CLGLCD_SHM), CLGLCD_SHM_NAME) 
        if not self.shm_map:
            raise ctypes.WinError()
        self.shm = CLGLCD_SHM.from_buffer(self.shm_map)

        self.version = self.shm.version
        if self.version < SUPPORTED_VERSION:
            raise RuntimeError("Unsupported CLGLCD version. %s" % seld.shm.version)
        if (int(time.time()) - self.shm.usb_host_ts) > 1:
            raise RuntimeError("CLGLCD SHM Timestamp is stale (%d)" % self.shm.usb_host_ts)

    def _check_host_ts(self):
        if (int(time.time()) - self.shm.usb_host_ts) > 1:
            raise RuntimeError("CLGLCD SHM Timestamp got stale (%d)" % self.shm.usb_host_ts)               
        
    def show(self, image_bytes):
        """ Displays bitmap on screen. Typically you
        can use 320x240 Grayscale ('L') PIL image
        and its .tobytes() method """        
        self._check_host_ts()
        if self.current_frame < 0: self.current_frame = 0
        self.current_frame ^= 1
        self.shm.frame[self.current_frame][:] = image_bytes
        self.shm.active_frame = self.current_frame
        self.shm.active_ts = int(time.time())        

    def wait_for_release(self, timeout = 0):
        """ Wait for release of touchpanel or timeout. 
        Timeout is in (fratcion) of secconds. Default zero
        means wait sometime in year 2038. """
        target = 2 ** 31  if timeout == 0 else time.time() + timeout
        while (self.shm.touch_p > 1) and (time.time() < target): 
            time.sleep(0.01)
            self._check_host_ts()
        return (self.shm.touch_p < 1) 

    def wait_for_touch(self, timeout = 0, release_timeout = 2):
        """ Wait for touch on the touchpanel or timeout. 
        Timeout is in (fratcion) of secconds. Default zero
        means wait sometime in year 2038. See wait_for_release
        for release timeout. """
        touched = False
        target = time.time() + timeout if timeout > 0 else 2 ** 31
        while (self.shm.touch_p < 1) and (time.time() < target): 
            time.sleep(0.01)
            self._check_host_ts()
        touched = (self.shm.touch_p > 1)          
        self.wait_for_release(release_timeout)
        return touched

    def touch_point(self):
        """ Return tuple of current X, Y, and Pressure values
        from the touchpanel """
        return (self.shm.touch_x, self.shm.touch_y, self.shm.touch_p)

    def raw_touch(self):
        """ Return tuple of current X, Y, and Pressure values
        from the touchpanel """
        return (self.shm.raw_touch_x, self.shm.raw_touch_y, 
                self.shm.raw_touch_z1, self.shm.raw_touch_z2, self.shm.raw_touch_cnt)


    def is_touched(self, release_timeout = 2):
        """ Return true if there is touch on the touchpanel
        See wait_for_release for release timeout. """
        if not self.shm.touch_p > 0: return False
        self.wait_for_release(release_timeout)
        return True

    def sleep(self, timeout):
        """ Sleep will keeping USB host notified we are still alive """
        t = time.time()
        self.shm.active_ts = t          
        target = t + timeout
        while (t < timeout):
            time.sleep(min(1, timeout - t))
            t = time.time()
            self.shm.active_ts = t
            self._check_host_ts()

    def close(self):
        if self.shm:
            self.shm.active_frame = -1
            self.shm.active_ts = -1
            self.shm = None
            if self.shm_map:
                self.shm_map.close()
        if self.mutex:
            if self.mutex.acquired: self.mutex.release()
            self.mutex.close()
            self.mutex = None

    __del__ = close

    def __repr__(self):
        return '{0}({1!r}, device_version={2})'.format(
                self.__class__.__name__, self.name, self.version)

    __str__ = __repr__

    # Context manager 
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

# EOF