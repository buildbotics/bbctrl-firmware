import os
import tarfile
import re
import platform
import time
import textwrap
import stat
import shutil
from SCons.Script import *


exclude_pats = [r'\.svn', r'.*~']
exclude = None
ns = None
always_build_resources = False
next_id = 0
col = 0


def start_file(path):
    f = open(path, 'wb')

    note = ('WARNING: This file was auto generated.  Please do NOT '
            'edit directly or check in to source control.')

    f.write(
        '/' + ('*' * 75) + '\\\n   ' +
        '\n   '.join(textwrap.wrap(note)) + '\n' +
        '\\' + ('*' * 75) + '/\n'
        '\n'
        '#include <cbang/util/Resource.h>\n\n'
        'using namespace cb;\n\n'
        )

    if ns:
        for namespace in ns.split('::'):
            f.write('namespace %s {\n' % namespace)

    return f


def end_file(f):
    if ns:
        for namespace in ns.split('::'):
            f.write('} // namespace %s\n' % namespace)

    f.close()


def write_string(output, s, newline = 0):
    global col

    l = len(s)

    if newline or col + l > 80:
        output.write('\n')
        col = 0

    i = s.rfind('\n')
    if i != -1: col = l - (i + 1)
    else: col += l

    output.write(s)


def is_excluded(path):
    return exclude != None and exclude.search(path) != None


def write_resource(output, data_dir, path, children = None, exclude = None):
    global next_id

    name = os.path.basename(path)
    if is_excluded(path): return

    is_dir = os.path.isdir(path)
    id = next_id
    next_id += 1
    length = 0

    if is_dir:
        typeStr = 'Directory'
        child_resources = []

        for filename in os.listdir(path):
            write_resource(output, data_dir, os.path.join(path, filename),
                           child_resources, exclude)

        write_string(output, 'const Resource *children%d[] = {' % id)

        for res in child_resources:
            write_string(output, '&resource%d,' % res)

        write_string(output, '0};\n')

    else:
        out_path = '%s/data%d.cpp' % (data_dir, id)
        print 'Writing resource: %s to %s' % (path, out_path)

        typeStr = 'File'
        f = open(path, 'rb')

        prototype = 'extern const unsigned char data%d[]' % id

        write_string(output, '%s;\n' % prototype)

        out = start_file(out_path)

        write_string(out, prototype + ' = {')

        while True:
            count = 0
            for c in f.read(102400):
                write_string(out, '%d,' % ord(c))
                count += 1

            if count == 0: break
            length += count

        write_string(out, '0};\n')

        end_file(out)

    if children != None: children.append(id)

    output.write('extern const %sResource resource%d("%s", ' %
                 (typeStr, id, name))

    if is_dir: output.write('children%d' % id)
    else: output.write('(const char *)data%d, %d' % (id, length))

    output.write(');\n')


def update_time(path, exclude):
    if not os.path.exists(path): return 0
    if exclude is not None and exclude.search(path) is not None: return 0

    if os.path.isdir(path):
        updated = 0

        for name in os.listdir(path):
            t = os.stat(path)[stat.ST_MTIME]
            if updated < t: updated = t
            t = update_time(os.path.join(path, name), exclude)
            if updated < t: updated = t

        return updated

    else: return os.stat(path)[stat.ST_MTIME]


def resources_build(target, source, env):
    global exclude, next_id, col, ns
    next_id = col = 0

    target = str(target[0])

    # Check update times
    if not always_build_resources:
        if os.path.exists(target):
            updated = False
            lastUpdate = os.stat(target)[stat.ST_MTIME]

            for src in source:
                if lastUpdate < update_time(str(src), exclude):
                    updated = True
                    break

            if not updated: return

    data_dir = os.path.splitext(target)[0] + ".data"
    if os.path.exists(data_dir): shutil.rmtree(data_dir)
    os.mkdir(data_dir)

    # Write resources
    f = start_file(target)

    next_id = 0
    for src in source:
        write_resource(f, data_dir, str(src), exclude = exclude)

    end_file(f)


def get_targets(path, data_dir, count = [0]):
    if is_excluded(path): return []

    id = count[0]
    count[0] += 1

    if os.path.isdir(path):
        targets = []

        for name in os.listdir(path):
            targets += get_targets(os.path.join(path, name), data_dir, count)

        return targets

    else:
        target = '%s/data%d.cpp' % (data_dir, id) 
        return [File(target)]


def modify_targets(target, source, env):
    name = str(target[0])
    data_dir = os.path.splitext(name)[0] + ".data"
    target += get_targets(str(source[0]), data_dir, [0])
    print map(str, target)
    return target, source


def resources_message(target, source, env):
    return 'building resource file "%s"' % str(target[0])


def configure(conf, namespace, no_default_excludes = False, excludes = [],
              always_build = False):
    global exclude, exclude_pats, ns, always_build_resources

    env = conf.env

    ns = namespace
    always_build_resources = always_build

    if not no_default_excludes: excludes = list(excludes) + exclude_pats

    pattern = None
    for ex in excludes:
        if pattern == None: pattern = ''
        else: pattern += '|'
        pattern += '(%s)' % ex

    exclude = re.compile(pattern)

    bld = env.Builder(action = resources_build,
                      source_factory = SCons.Node.FS.Entry,
                      source_scanner = SCons.Defaults.DirScanner,
                      emitter = modify_targets)
    env.Append(BUILDERS = {'Resources' : bld})

    return True
