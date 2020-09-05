# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2019 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Inspired by The ViaDuck Project for building OpenSSL

# creates a building environment for berkeleydb
# - working directory
# - on windows: uses msys' bash for command execution (berkeleydb's scripts need an UNIX-like environment with perl)

from subprocess import PIPE, Popen
from sys import argv, exit
import os, re

env = os.environ
l = []

os_s = argv[1]                                      # operating system
offset = 2          # 0: this script's path, 1: operating system

if os_s == "WIN32":
    offset = 4  # 2: MSYS_BASH_PATH, 3: CMAKE_MAKE_PROGRAM

    #
    bash = argv[2]
    msys_path = os.path.dirname(bash)
    mingw_path = os.path.dirname(argv[3])

    # append ; to PATH if needed
    if not env['PATH'].endswith(";"):
        env['PATH'] += ";"

    # include path of msys binaries (perl, cd etc.) and building tools (gcc, ld etc.)
    env['PATH'] = ";".join([msys_path, mingw_path])+";"+env['PATH']
    env['MAKEFLAGS'] = ''            # otherwise: internal error: invalid --jobserver-fds string `gmake_semaphore_1824'


binary_berkeleydb_dir_source = argv[offset]+"/"             # downloaded berkeleydb source dir
l.extend(argv[offset+1:])                             # routed commands

l[0] = '"'+l[0]+'"'

# ensure target dir exists for mingw cross
target_dir = binary_berkeleydb_dir_source+"/../../../usr/local/bin"
if not os.path.exists(target_dir):
    os.makedirs(target_dir)

# read environment from file if cross-compiling
if os_s == "LINUX_CROSS_ANDROID":
    expr = re.compile('^(.*?)="(.*?)"', re.MULTILINE | re.DOTALL)
    f = open(binary_berkeleydb_dir_source+"/../../../../buildenv.txt", "r")
    content = f.read()
    f.close()

    for k, v in expr.findall(content):
        # print('k: ' + k + ', v: ' + v)
        if "\n" in k.strip():
            print('Skipping multiline key')
        elif k != "PATH":
            env[k] = v.replace('"', '')
        else:
            env[k] = v.replace('"', '')+":"+env[k]

proc = None
if os_s == "WIN32":
    # we must emulate a UNIX environment to build berkeleydb using mingw
    proc = Popen(bash, env=env, cwd=binary_berkeleydb_dir_source, stdin=PIPE, universal_newlines=True)
    proc.communicate(input=" ".join(l)+" || exit $?")
else:
    proc = Popen(" ".join(l)+" || exit $?", shell=True, env=env, cwd=binary_berkeleydb_dir_source)
    proc.communicate()

exit(proc.returncode)
