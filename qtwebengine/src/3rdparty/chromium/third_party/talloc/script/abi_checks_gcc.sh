#!/bin/bash

make clean

mkdir abi
ABI_CHECKS="-aux-info abi/\$@.X"
make ABI_CHECK="$ABI_CHECKS" CC="/usr/bin/gcc"

for i in abi/*.X; do cat $i | grep 'talloc\.h'; done | sort | uniq | awk -F "extern " '{ print $2 }' | sort > abi/signatures

cat > abi/exports << EOF
{
    global:
EOF
cat abi/signatures | awk -F '(' '{ print $1 }' | awk -F ' ' '{ print "           "$NF";" }' | tr -d '*' | sort >> abi/exports
# need to manually add talloc free for backward ABI compat
echo '           talloc_free;' >> abi/exports
cat >> abi/exports << EOF

    local: *;
};
EOF

rm -fr abi/*.X

diff -u talloc.signatures abi/signatures
if [ "$?" != "0" ]; then
    echo "WARNING: Possible ABI Change!!"
fi

diff -u talloc.exports abi/exports
if [ "$?" != "0" ]; then
    echo "WARNING: Export file may be outdated!!"
fi
