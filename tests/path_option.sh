#!/usr/bin/env sh

# Before running the script make sure a linscreen daemon with a matching version
# is running

# The first argument to this script is a path to the linscreen executable
[ -n "$1" ] && linscreen="$1" || linscreen='linscreen'

# TODO Before proper stderr logging is implemented, you will have to look at the
# system notifications

rm -rf /tmp/linscreen_path_test 2>/dev/null
mkdir -p /tmp/linscreen_path_test
cd /tmp/linscreen_path_test

echo ">> Nonexistent directory. This command should give an invalid path error."
"$linscreen" screen -p blah/blah

sleep 2
echo ">> The output file is specified relative to PWD"
"$linscreen" screen -p relative.png

sleep 2
echo ">> Absolute paths work too"
"$linscreen" screen -p /tmp/linscreen_path_test/absolute.png

sleep 2
mkdir subdir
echo ">> Redundancy in the path will be removed"
"$linscreen" screen -p /tmp/linscreen_path_test/subdir/..///redundancy_removed.png

sleep 2
echo ">> If the destination is a directory, the file name is generated from strf from the config"
"$linscreen" screen -p ./

sleep 2
echo ">> If the output file has no suffix, it will be added (png)"
"$linscreen" screen -p /tmp/linscreen_path_test/without_suffix

sleep 2
echo ">> Other suffixes are supported, and the image format will match it"
"$linscreen" screen -p /tmp/linscreen_path_test/jpg_suffix.jpg

sleep 2
echo ">> If the destination path exists, it will have _NUM appended to the base name"
"$linscreen" screen -p /tmp/linscreen_path_test/absolute.png

sleep 2
echo ">> Same thing again but without specifying a suffix"
"$linscreen" screen -p /tmp/linscreen_path_test/absolute

sleep 2
