import atexit
import ctypes
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Dict, List, Optional, Tuple


# Structure definitions
class CKeyPosition(ctypes.Structure):
    _fields_ = [
        ("row", ctypes.c_uint8),
        ("col", ctypes.c_uint8)
    ]


class CKeyEvent(ctypes.Structure):
    _fields_ = [
        ("key", CKeyPosition),
        ("pressed", ctypes.c_bool)
    ]


class CKeyRecord(ctypes.Structure):
    _fields_ = [
        ("event", CKeyEvent)
    ]

def create_ckeyrecord(row: int, col: int, pressed: bool) -> CKeyRecord:
    """Helper to create a keyrecord structure"""
    record = CKeyRecord()
    record.event.key.row = row
    record.event.key.col = col
    record.event.pressed = pressed
    return record

class CDeferredExecInfo(ctypes.Structure):
    _fields_ = [
        ("delay_ms", ctypes.c_uint32),
        ("deadline_ms", ctypes.c_uint32),
        ("callback", ctypes.c_void_p),  # Using void pointer for function pointer
        ("cb_arg", ctypes.c_void_p),
        ("active", ctypes.c_bool),
    ]


class CHistory(ctypes.Structure):
    _fields_ = [
        ("row", ctypes.c_uint8),
        ("col", ctypes.c_uint8),
        ("keycode", ctypes.c_uint16),
        ("pressed", ctypes.c_bool),
        ("mods", ctypes.c_uint8),
        ("layer_state", ctypes.c_uint32),
        ("smtd_bypass", ctypes.c_bool),
    ]



class Keycode:
    def __init__(self, smtd, value, row, col, layer):
        self.smtd = smtd
        self.value = value
        self.row = row
        self.col = col
        self.layer = layer
        self.pressed = False
        self.defer_idx = None

    def reset(self):
        self.pressed = False
        self.defer_idx = None

    def press(self):
        if self.pressed:
            raise RuntimeError("keycode is already pressed")
        if self.smtd.get_layer_state() != self.layer:
            raise RuntimeError("keycode is not active on the current layer")
        self.pressed = True
        result, defer_idx = self.smtd.process_key_and_timeout(self, True)
        self.defer_idx = defer_idx
        return result

    def release(self):
        if not self.pressed:
            raise RuntimeError("keycode is not pressed")
        self.pressed = False
        result, defer_idx = self.smtd.process_key_and_timeout(self, False)
        self.defer_idx = defer_idx
        return result

    def prolong(self):
        if self.defer_idx is None:
            raise RuntimeError("keycode has no deferred execution")
        self.smtd.execute_deferred(self.defer_idx)
        self.defer_idx = None

    def try_prolong(self):
        if self.defer_idx is None: return
        self.smtd.execute_deferred(self.defer_idx, False)
        self.defer_idx = None

    def __str__(self):
        return self.value

    def __repr__(self):
        return self.value


class Key:
    def __init__(self, smtd, name, row, col, comment, all_keycodes):
        self.smtd = smtd
        self.name = name
        self.row = row
        self.col = col
        self.comment = comment
        self.all_keycodes = all_keycodes
        self.pressed = None
        self.released = None

    def reset(self):
        if self.pressed: self.pressed.reset()
        if self.released: self.released.reset()
        self.pressed = None
        self.released = None

    def press(self):
        if self.pressed is not None:
            raise RuntimeError("key is already pressed")
        self.released = None
        self.pressed = self.current_keycode()
        return self.pressed.press()

    def current_keycode(self):
        layer = self.smtd.get_layer_state()
        for keycode in self.all_keycodes:
            if keycode.row == self.row and keycode.col == self.col and keycode.layer == layer:
                return keycode
        raise ValueError(f"No keycode for {self} on layer {layer}")

    def release(self):
        if self.pressed is None:
            raise RuntimeError("key is not pressed")
        if self.released is not None:
            raise RuntimeError("key was already released")
        result = self.pressed.release()
        self.released = self.pressed
        self.pressed = None
        return result

    def prolong(self):
        if self.pressed: return self.pressed.prolong()
        if self.released: return self.released.prolong()
        raise "both pressed and released are None"

    def try_prolong(self):
        if self.pressed: return self.pressed.try_prolong()
        if self.released: return self.released.try_prolong()
        raise "both pressed and released are None"

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return f"Key.{self.name} # {self.comment}"

class SmtdBindings:
    """Encapsulates all SMTD library bindings and functions"""

    def __init__(self, lib: ctypes.CDLL):
        self.lib = lib

    def process_key_and_timeout(self, keycode: Keycode, pressed: bool) -> Tuple[bool, Optional[int]]:
        execs_before = self.get_deferred_execs()
        record_ptr = ctypes.pointer(create_ckeyrecord(keycode.row, keycode.col, pressed))
        result = self.lib.process_smtd(ctypes.c_uint(keycode.value), record_ptr)
        execs_after = self.get_deferred_execs()
        execs_diff = execs_after[len(execs_before):]
        if len(execs_diff) == 0:
            return result, None
        return result, execs_diff[-1]["idx"]

    def set_bypass(self, enabled: bool) -> None:
        """Set the smtd_bypass flag"""
        self.lib.TEST_set_smtd_bypass(ctypes.c_bool(enabled))

    def reset(self) -> None:
        """Reset the test state."""
        self.lib.TEST_reset()

    def fail_next_deferred_exec(self) -> None:
        """Make the next mocked defer_exec call return INVALID_DEFERRED_TOKEN."""
        self.lib.TEST_fail_next_deferred_exec()

    def get_record_history(self) -> List[Dict[str, Any]]:
        """Get the history of key records processed"""
        records = (CHistory * 100)()  # MAX_RECORD_HISTORY is 100
        count = ctypes.c_uint8(0)
        self.lib.TEST_get_record_history(records, ctypes.byref(count))

        result = []
        for i in range(count.value):
            result.append({
                "row": records[i].row,
                "col": records[i].col,
                "keycode": records[i].keycode,
                "pressed": records[i].pressed,
                "mods": records[i].mods,
                "layer_state": records[i].layer_state,
                "smtd_bypass": records[i].smtd_bypass,
            })
        return result

    def get_deferred_execs(self) -> List[Dict[str, Any]]:
        """Get the list of deferred executions scheduled in the test environment"""
        execs_array = (CDeferredExecInfo * 100)()  # MAX_DEFERRED_EXECS is 100
        count = ctypes.c_uint8(0)
        self.lib.TEST_get_deferred_execs(execs_array, ctypes.byref(count))

        result = []
        for i in range(count.value):
            result.append({
                "idx": i + 1,
                "delay_ms": execs_array[i].delay_ms,
                "deadline_ms": execs_array[i].deadline_ms,
                "active": execs_array[i].active
            })
        return result

    def execute_deferred(self, idx: int, make_asserts: bool = True) -> None:
        """Execute a specific deferred execution by its id"""
        if make_asserts and not self.get_deferred_execs()[idx - 1]["active"]:
            raise RuntimeError(f"deferred execution {idx} is inactive")
        if not self.get_deferred_execs()[idx - 1]["active"]:
            return
        self.lib.TEST_execute_deferred(ctypes.c_uint8(idx))
        if self.get_deferred_execs()[idx - 1]["active"]:
            raise RuntimeError(f"deferred execution {idx} did not complete")

    def wait(self, ms: int) -> None:
        """Advance the virtual clock, firing deferred executions that come due"""
        self.lib.TEST_advance_time(ctypes.c_uint32(ms))

    def get_mods(self) -> int:
        """Get the current modifier state"""
        return self.lib.get_mods()

    def get_layer_state(self) -> int:
        """Get the current layer state"""
        return self.lib.TEST_get_layer_state()

    def set_caps_word(self, on: bool) -> None:
        """Turn the mocked Caps Word state on or off"""
        self.lib.TEST_set_caps_word(ctypes.c_bool(on))

    def is_caps_word_on(self) -> bool:
        """Get the mocked Caps Word state"""
        return bool(self.lib.TEST_is_caps_word_on())

    def get_weak_mods(self) -> int:
        """Get the current weak modifier state"""
        return self.lib.TEST_get_weak_mods()


# Compile and load the shared library
def load_smtd_lib(path: str) -> SmtdBindings:
    """Compile and load the sm_td shared library"""
    project_root = Path(__file__).resolve().parents[2]
    source_path = (project_root / path).resolve()
    if project_root not in source_path.parents or not source_path.is_file():
        raise ValueError(f"Layout source must be a file inside {project_root}: {path}")

    # Create a unique library name based on the source path to avoid conflicts.
    path_hash = hashlib.sha256(str(source_path).encode()).hexdigest()[:12]
    ext = '.dylib' if sys.platform == 'darwin' else '.so'
    build_dir = Path(tempfile.mkdtemp(prefix="sm_td-tests-"))
    lib_path = build_dir / f"libsm_td_{path_hash}{ext}"

    compiler = os.environ.get("CC")
    if compiler:
        compiler_args = shlex.split(compiler)
        compiler_path = shutil.which(compiler_args[0])
    else:
        compiler_args = []
        compiler_path = next(
            (candidate for name in ("clang", "cc", "gcc")
             if (candidate := shutil.which(name))),
            None,
        )
    if compiler_path is None:
        raise RuntimeError("No C compiler found. Install clang or set the CC environment variable.")
    if compiler_args:
        compiler_args[0] = compiler_path
    else:
        compiler_args = [compiler_path]

    debug_enabled = os.environ.get("SMTD_DEBUG", "0").lower() not in {"", "0", "false", "no"}
    debug_flags = ["-DSMTD_TEST_DEBUG"] if debug_enabled else []

    compile_cmd = [
        *compiler_args,
        "-shared",
        "-o", str(lib_path),
        "-fPIC", str(source_path),
        f"-I{project_root}",
        "-DSMTD_UNIT_TEST",
        *debug_flags,
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Wno-sign-compare",
        "-Wno-missing-braces",
        "-Wno-unused-parameter",
        "-Wunused-variable",
        "-Werror=unused-variable",
    ]

    print(f"Compiling sm_td library: {shlex.join(compile_cmd)}")
    result = subprocess.run(compile_cmd, capture_output=True, text=True)

    if result.returncode != 0:
        raise RuntimeError(
            f"Failed to compile {source_path}:\n{result.stderr.strip()}"
        )

    # Load the compiled library
    lib: ctypes.CDLL = ctypes.CDLL(str(lib_path))

    # Register cleanup to remove the library file
    def cleanup() -> None:
        try:
            shutil.rmtree(build_dir, ignore_errors=True)
        except Exception as error:
            print(f"Failed to clean up test build directory: {error}")

    atexit.register(cleanup)

    lib.process_smtd.argtypes = [ctypes.c_uint, ctypes.POINTER(CKeyRecord)]
    lib.process_smtd.restype = ctypes.c_bool

    lib.TEST_set_smtd_bypass.argtypes = [ctypes.c_bool]
    lib.TEST_set_smtd_bypass.restype = None

    lib.TEST_reset.argtypes = []
    lib.TEST_reset.restype = None

    lib.TEST_fail_next_deferred_exec.argtypes = []
    lib.TEST_fail_next_deferred_exec.restype = None

    lib.TEST_get_record_history.argtypes = [
        ctypes.POINTER(CHistory),  # out_records
        ctypes.POINTER(ctypes.c_uint8)  # out_count
    ]
    lib.TEST_get_record_history.restype = None

    lib.TEST_get_deferred_execs.argtypes = [
        ctypes.POINTER(CDeferredExecInfo),  # out_execs
        ctypes.POINTER(ctypes.c_uint8)  # out_count
    ]
    lib.TEST_get_deferred_execs.restype = None

    lib.TEST_execute_deferred.argtypes = [ctypes.c_uint8]  # deferred_token
    lib.TEST_execute_deferred.restype = None

    lib.TEST_advance_time.argtypes = [ctypes.c_uint32]
    lib.TEST_advance_time.restype = None

    lib.get_mods.argtypes = []  # No arguments
    lib.get_mods.restype = ctypes.c_uint8  # Returns uint8_t

    lib.TEST_get_layer_state.argtypes = []
    lib.TEST_get_layer_state.restype = ctypes.c_uint8

    lib.TEST_set_caps_word.argtypes = [ctypes.c_bool]
    lib.TEST_set_caps_word.restype = None

    lib.TEST_is_caps_word_on.argtypes = []
    lib.TEST_is_caps_word_on.restype = ctypes.c_bool

    lib.TEST_get_weak_mods.argtypes = []
    lib.TEST_get_weak_mods.restype = ctypes.c_uint8

    return SmtdBindings(lib)
