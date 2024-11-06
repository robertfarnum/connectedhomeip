#!/bin/sh

# Find the directory where the script resides:
SCRIPT_NAME=$(basename $0)
SCRIPT_PATH=$(realpath $0)
SCRIPT_DIR=$(dirname ${SCRIPT_PATH})

PACKAGE=frontend2.tgz
INSTALLDIR=/usr/share/chip-tool-web

echo
echo " Installing web interface frontend..."

# Check for the package to install
if [ ! -f ${SCRIPT_DIR}/${PACKAGE} ]
then
	echo
	echo " ERROR: Unable to find the package ${PACKAGE}!"
	echo

	exit 1
fi

echo " Creating / verifying install dir..."
if [ ! -d ${INSTALLDIR} ]
then
	sudo mkdir -p ${INSTALLDIR}
fi

if [ ! -d ${INSTALLDIR} ]
then
	echo
	echo " ERROR: Not enough permissions to install frontend!"
	echo

	exit 1
fi

cd ${INSTALLDIR}

echo " Installing files..."
echo
sudo tar -xzvf ${SCRIPT_DIR}/${PACKAGE}

# Make sure "root" owns these files
sudo chown -R root:root frontend2

echo
echo " Done."
echo " Thank you for your patience!"
echo

exit 0
