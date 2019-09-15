#!/bi/bash

# Fail quickly with verbose logging.
set -ex

# Assume you are running this script either in ARK_REPO or ARK_REPO/tools
ARK_REPO="$PWD"
cd "${ARK_REPO}"
# A tiny hack for convience.
[ -d 'tools' ] && cd tools

# Prepare Clang/LLVM toolchain
echo 'Prepare Clang/LLVM toolchain'
wget http://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz
tar xf ~/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz

## I don't know why the official dev team replaced the '+' in file path.
## Just follow the doc.
mv clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04 clang_llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04

# Prepare gn
echo 'Prepare gn'
mkdir gn
wget -O gn/gn 'https://archive.softwareheritage.org/browse/content/sha1_git:2dc0d5b26caef44f467de8120b26f8aad8b878be/raw/?filename=gn'
chmod +x gn/gn

# Prepare ninja
echo 'Prepare ninja'
## Follow the doc so we don't need to modify config files.
mkdir ninja_1.9.0
wget -O ninja-linux.zip 'https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-linux.zip'
unzip -d ninja_1.9.0 ninja-linux.zip
chmod +x ninja_1.9.0/ninja

# Here you go.
echo 'Here you go.'
echo 'All deps has been established.'
echo 'Now just run:'
echo ''
echo '        $ cd the-root-of-ark-repo'
echo '        $ source build/envsetup.sh'
echo '        $ make'
echo ''

# If you are really lazy like us...
read -t 5 -p "Or perhaps you want to ask me to do it automatically? [y/N] " should_build

if [ x"$should_build" = x"y" -o x"$should_build" = x"Y" ]; then
	[ -d '../tools/gn' ] && cd ..
	echo "DEBUG: now at $PWD"
	source build/envsetup.sh
	make -j $(nproc)

fi

echo 'Have a nice day.'
