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
wget --continue -O tmp.clang.tar.xz http://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz
tar -xJvf tmp.clang.tar.xz
rm tmp.clang.tar.xz

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
wget --continue -O tmp.ninja-linux.zip 'https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-linux.zip'
unzip -d ninja_1.9.0 tmp.ninja-linux.zip
rm tmp.ninja-linux.zip
chmod +x ninja_1.9.0/ninja

# Here you go.
echo 'Here you go.'
echo 'All deps has been established.'
echo 'Now just run:'
echo ''
echo '        $ sudo apt-get update\'
echo '        $ sudo apt-get -y install openjdk-8-jdk git-core gnupg \\'
echo '            flex bison gperf build-essential zip curl \\'
echo '            zlib1g-dev libc6-dev-i386 lib32ncurses5-dev \\'
echo '            x11proto-core-dev libx11-dev lib32z-dev ccache \\'
echo '            libgl1-mesa-dev libxml2-utils xsltproc unzip \\'
echo '            lib32z1-dev qemu g++-multilib gcc-multilib \\'
echo '            libglib2.0-dev libpixman-1-dev linux-libc-dev:i386\'
echo '        $ sudo apt-get -y install gcc-5-aarch64-linux-gnu g++-5-aarch64-linux-gnu\'
echo '        $ cd the-root-of-ark-repo'
echo '        $ source build/envsetup.sh'
echo '        $ make'
echo ''

# If you are really lazy like us...
read -t 20 -p "Or perhaps you want to ask me to do it automatically? [y/N] " should_build

if [ x"$should_build" = x"y" -o x"$should_build" = x"Y" ]; then
	# update is needed when you face a checksum mismatch or file not found error.
	sudo apt-get update
	sudo apt-get -y install openjdk-8-jdk git-core gnupg \
		flex bison gperf build-essential zip curl \
		zlib1g-dev libc6-dev-i386 lib32ncurses5-dev \
		x11proto-core-dev libx11-dev lib32z-dev ccache \
		libgl1-mesa-dev libxml2-utils xsltproc unzip \
		lib32z1-dev qemu g++-multilib gcc-multilib \
		libglib2.0-dev libpixman-1-dev linux-libc-dev:i386
	sudo apt-get -y install gcc-5-aarch64-linux-gnu g++-5-aarch64-linux-gnu
	# A tiny hack for convience.
	[ -d '../tools/gn' ] && cd ..
	echo "DEBUG: now at $PWD"
	source build/envsetup.sh
	make -j $(nproc)
fi

echo 'Done. All Good. Have a nice day. :-)'
