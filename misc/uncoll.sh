# example of usage of bmuncoll
#
# usage: uncoll full_path_to_collection_file

# existing temporary working directory
TEMP=/tmp

# external program to call
EXTPGM=ls

cd $TEMP
mkdir bmuncoll
cd bmuncoll
bmuncoll -x $1

if [ $? -eq 0 ]; then
  for f in *; do
    echo ""
    echo "Processing $f... "
    $EXTPGM $f
    if [ $? -eq 0 ]; then
      echo "ok."
      rm $f
    else
      echo "failed!"
    fi
  done
else
  echo ""
  echo "*** ERROR ***"
fi

cd ..
rmdir bmuncoll
