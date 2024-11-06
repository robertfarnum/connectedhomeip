#!/bin/sh

# Find the directory where the script resides:
SCRIPT_NAME=$(basename $0)
SCRIPT_PATH=$(realpath $0)
SCRIPT_DIR=$(dirname ${SCRIPT_PATH})

cd ${SCRIPT_DIR}

echo
echo " Generating the web interface install package..."
echo

if [ ! -d ../frontend2 ]
then
	echo
	echo " ERROR: Unable to find package content!"
	echo

	exit 1
fi

tar -czvf frontend2.tgz ../frontend2

echo
echo " Done."
echo

exit 0
