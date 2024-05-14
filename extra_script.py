Import('env')
import os 
import shutil

def copy_full_path(source):
    dest_full_path = os.path.join('data', source)
    dest_dir = os.path.dirname(dest_full_path)
    print(f'Creating destination path `{dest_dir}`')
    os.makedirs(dest_dir, exist_ok=True)
    print(f'Copying `{source}` to `{dest_full_path}`')
    shutil.copy(source, dest_full_path)

def gzip_all():
    env.Execute('find data -type f -exec gzip -9 {} \\;')
    pass


def before_build_filesystem(source, target, env):
    print('Removing old filesystem image...')
    env.Execute('rm -rf data')
    env.Execute('mkdir -p data/web/static')
    print('[OK]\n')


    print('Minifying index.html...')
    env.Execute('htmlmin web/index.html data/web/index.html')
    print('[OK]\n')

    print('Minifying app.js...')
    env.Execute('uglifyjs web/static/app.js -o data/web/static/app.js')
    print('[OK]\n')

    print('Copying bootstrap files...')
    copy_full_path('web/static/bootstrap-5.3.3-dist/css/bootstrap.min.css')
    copy_full_path('web/static/bootstrap-5.3.3-dist/js/bootstrap.bundle.min.js')
    print('[OK]\n')
    print('Compressing all files in /data')
    gzip_all()
    print('[OK]')



env.AddPreAction('$BUILD_DIR/littlefs.bin', before_build_filesystem)

