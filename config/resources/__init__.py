import os
import tarfile
import re
import platform
import time
import textwrap
import stat
import shutil
from SCons.Script import *

next_id = 0
col = 0

class ResourceContext:
    def __init__(self): pass


def start_file(ctx, path):
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

    if ctx.ns:
        for namespace in ctx.ns.split('::'):
            f.write('namespace %s {\n' % namespace)

    return f


def end_file(ctx, f):
    if ctx.ns:
        for namespace in ctx.ns.split('::'):
            f.write('} // namespace %s\n' % namespace)

    f.close()


def write_string(ctx, output, s, newline = 0):
    l = len(s)

    if newline or ctx.col + l > 80:
        output.write('\n')
        ctx.col = 0

    i = s.rfind('\n')
    if i != -1: ctx.col = l - (i + 1)
    else: ctx.col += l

    output.write(s)


def is_excluded(exclude, path):
    return exclude != None and exclude.search(path) != None


def write_resource(ctx, output, data_dir, path, children = None,
                   exclude = None):
    name = os.path.basename(path)
    if is_excluded(ctx.exclude, path): return

    is_dir = os.path.isdir(path)
    id = ctx.next_id
    ctx.next_id += 1
    length = 0

    if is_dir:
        typeStr = 'Directory'
        child_resources = []

        for filename in os.listdir(path):
            write_resource(ctx, output, data_dir, os.path.join(path, filename),
                           child_resources, exclude)

        write_string(ctx, output, 'const Resource *children%d[] = {' % id)

        for res in child_resources:
            write_string(ctx, output, '&resource%d,' % res)

        write_string(ctx, output, '0};\n')

    else:
        out_path = '%s/data%d.cpp' % (data_dir, id)
        print 'Writing resource: %s to %s' % (path, out_path)

        typeStr = 'File'
        f = open(path, 'rb')

        prototype = 'extern const unsigned char data%d[]' % id

        write_string(ctx, output, '%s;\n' % prototype)

        out = start_file(ctx, out_path)

        write_string(ctx, out, prototype + ' = {')

        while True:
            count = 0
            for c in f.read(102400):
                write_string(ctx, out, '%d,' % ord(c))
                count += 1

            if count == 0: break
            length += count

        write_string(ctx, out, '0};\n')

        end_file(ctx, out)

    if children != None: children.append(id)

    output.write('extern const %sResource resource%d("%s", ' %
                 (typeStr, id, name))

    if is_dir: output.write('children%d' % id)
    else: output.write('(const char *)data%d, %d' % (id, length))

    output.write(');\n')


def update_time(ctx, path):
    if not os.path.exists(path): return 0
    exclude = ctx.exclude
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


def get_exclude(env):
    pattern = None
    for ex in env.get('RESOURCES_EXCLUDES'):
        if pattern == None: pattern = ''
        else: pattern += '|'
        pattern += '(%s)' % ex

    return re.compile(pattern)


def resources_build(target, source, env):
    ctx = ResourceContext()
    ctx.env = env
    ctx.ns = env.get('RESOURCES_NS')
    ctx.exclude = get_exclude(env)
    ctx.next_id = 0
    ctx.col = 0

    target = str(target[0])

    # Check update times
    if not env.get('RESOURCES_ALWAYS_BUILD'):
        if os.path.exists(target):
            updated = False
            lastUpdate = os.stat(target)[stat.ST_MTIME]

            for src in source:
                if lastUpdate < update_time(ctx, str(src)):
                    updated = True
                    break

            if not updated: return

    data_dir = os.path.splitext(target)[0] + ".data"
    if os.path.exists(data_dir): shutil.rmtree(data_dir)
    os.mkdir(data_dir)

    # Write resources
    f = start_file(ctx, target)

    next_id = 0
    for src in source:
        write_resource(ctx, f, data_dir, str(src))

    end_file(ctx, f)


def get_targets(exclude, path, data_dir, count = [0]):
    if is_excluded(exclude, path): return []

    id = count[0]
    count[0] += 1

    if os.path.isdir(path):
        targets = []

        for name in os.listdir(path):
            targets += \
                get_targets(exclude, os.path.join(path, name), data_dir, count)

        return targets

    else:
        target = '%s/data%d.cpp' % (data_dir, id) 
        return [File(target)]


def modify_targets(target, source, env):
    exclude = get_exclude(env)
    name = str(target[0])
    data_dir = os.path.splitext(name)[0] + ".data"
    target += get_targets(exclude, str(source[0]), data_dir, [0])
    print map(str, target)
    return target, source


def resources_message(target, source, env):
    return 'building resource file "%s"' % str(target[0])


def generate(env):
    env.SetDefault(RESOURCES_NS = '')
    env.SetDefault(RESOURCES_EXCLUDES = [r'\.svn', r'.*~'])
    env.SetDefault(RESOURCES_ALWAYS_BUILD = True)

    bld = env.Builder(action = resources_build,
                      source_factory = SCons.Node.FS.Entry,
                      source_scanner = SCons.Defaults.DirScanner,
                      emitter = modify_targets)
    env.Append(BUILDERS = {'Resources' : bld})


def exists():
    return True
