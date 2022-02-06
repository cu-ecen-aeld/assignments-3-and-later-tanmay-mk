#!/bin/sh
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.
# 	  Updated by Tanmay Mahendra Kothale
#		added code required to comply with AESD
#		Assignment 3 - Part 2 requirements
#
#		Changes made by me are guarded with 
#		comments with my github username. 
#

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

##-----------tanmay-mk-----------##
#if directory is not created correctly, exit
if [ ! -d ${OUTDIR} ]
then
	echo "Directory cannot be created." 
	exit 1
fi
##-----------tanmay-mk-----------##

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

##-----------tanmay-mk-----------##
    # TODO: Add your kernel build steps here
    echo "Starting kernel build steps"
    echo "deep cleaning the kernel build tree, removing existing .config files"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    
    echo "configuring for virt arm dev board to simulate"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    
    echo "building a kernel image to boot with QEMU"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    
    echo "building any kernel modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    
    echo "building the device tree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
##-----------tanmay-mk-----------##
fi 


echo "Adding the Image in outdir"
##-----------tanmay-mk-----------##
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/
##-----------tanmay-mk-----------##

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
##-----------tanmay-mk-----------##
echo "Creating the base directories"
mkdir -p ${OUTDIR}/rootfs

if ! [ -d "${OUTDIR}/rootfs" ]
then
	echo "Error: ${OUTDIR}/rootfs could not be created"
	exit 1
fi

cd ${OUTDIR}/rootfs
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/sbin
mkdir -p var/log
##-----------tanmay-mk-----------##

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    
    # TODO:  Configure busybox
##-----------tanmay-mk-----------##
    echo "Configuring Busybox"
    make distclean
    make defconfig
##-----------tanmay-mk-----------##
else
    cd busybox
fi

# TODO: Make and install busybox
##-----------tanmay-mk-----------##
echo "Installing Busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Returning to rootfs"
cd ${OUTDIR}/rootfs
##-----------tanmay-mk-----------##

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
##-----------tanmay-mk-----------##
echo "Adding library Dependencies"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -f $SYSROOT/lib/ld-linux-aarch64.so.1 lib
cp -f $SYSROOT/lib64/libm.so.6 lib64
cp -f $SYSROOT/lib64/libresolv.so.2 lib64
cp -f $SYSROOT/lib64/libc.so.6 lib64
##-----------tanmay-mk-----------##

# TODO: Make device nodes
##-----------tanmay-mk-----------##
echo "Creating device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
##-----------tanmay-mk-----------##

# TODO: Clean and build the writer utility
##-----------tanmay-mk-----------##
cd $FINDER_APP_DIR
make clean 
make CROSS_COMPILE=${CROSS_COMPILE}
##-----------tanmay-mk-----------##

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

##-----------tanmay-mk-----------##
cp -f $FINDER_APP_DIR/autorun-qemu.sh ${OUTDIR}/rootfs/home
cp -f $FINDER_APP_DIR/conf/ -r ${OUTDIR}/rootfs/home
cp -f $FINDER_APP_DIR/finder.sh ${OUTDIR}/rootfs/home
cp -f $FINDER_APP_DIR/finder-test.sh ${OUTDIR}/rootfs/home
cp -f $FINDER_APP_DIR/writer ${OUTDIR}/rootfs/home
##-----------tanmay-mk-----------##

# TODO: Chown the root directory
##-----------tanmay-mk-----------##
echo "Chowning the rootfs directory"
cd ${OUTDIR}/rootfs
sudo chown -R root:root *
##-----------tanmay-mk-----------##

# TODO: Create initramfs.cpio.gz
##-----------tanmay-mk-----------##
echo "Creating initramfs.cpio.gz"
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip -f initramfs.cpio
##-----------tanmay-mk-----------##

##EOF
