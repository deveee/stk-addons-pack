#!/bin/sh

export LANG=C

cd "`dirname "$0"`"

if [ ! -d "../data" ]; then
    echo "Couldn't find data directory"
    exit 1
fi

# Clear previous assets directory
echo "Clear previous assets directory"
rm -rf assets
mkdir -p assets/data

# Copy data directory
echo "Copy data directory"
cp -a ../data/* assets/data/

# Generate files list
echo "Generate files list"
find assets/* -type f > assets/files.txt
sed -i s/'.\/assets\/'// assets/files.txt
sed -i s/'assets\/'// assets/files.txt

# It will be probably ignored by ant, but create it anyway...
touch assets/.nomedia

echo "Done."
exit 0
