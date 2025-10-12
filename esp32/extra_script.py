Import('env')
import os 
import subprocess
import shutil

def copy_full_path(source):
    dest_full_path = os.path.join('data', source)
    dest_dir = os.path.dirname(dest_full_path)
    print(f'Creating destination path `{dest_dir}`')
    os.makedirs(dest_dir, exist_ok=True)
    print(f'Copying `{source}` to `{dest_full_path}`')
    shutil.copy(source, dest_full_path)

def copy_strip(rootdir, path):
    source_path = os.path.join(rootdir, path)
    dest_path = os.path.join('data', 'web', path)
    dest_dir = os.path.dirname(dest_path)
    os.makedirs(dest_dir, exist_ok=True)
    shutil.copy(source_path, dest_path)

def copy_matching(rootdir, directory, filter):
    source_directory = os.path.join(rootdir, directory)
    if not os.path.isdir(source_directory):
        return
    for file in os.listdir(source_directory):
        if filter(file):
            copy_strip(rootdir, os.path.join(directory, file))

def gzip_all():
    env.Execute('find data/web -type f -exec gzip -9 {} \\;')

def before_build_filesystem(source, target, env):
    print('Removing old filesystem image...')
    env.Execute('rm -rf data/web')
    env.Execute('mkdir -p data/web')
    print('[OK]\n')
    print('Building react app')
    # subprocess.run('npm run build', shell=True, cwd='web').check_returncode()
    env.Execute('cd ../frontend; npm run build')
    copy_matching('../frontend/build', 'assets', lambda f: 'index' in f and f.endswith('.js'))
    copy_matching('../frontend/build', '', lambda f: [ext for ext in ['.html', '.min.css', 'site_logo_mid.png'] if f.endswith(ext)])



    print('[OK]\n')
    print('Compressing all files in /data')
    gzip_all()
    print('[OK]')



env.AddPreAction('$BUILD_DIR/littlefs.bin', before_build_filesystem)

