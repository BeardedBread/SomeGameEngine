#!/bin/bash

sed 's|res/||g' assets.info.raw > assets.info
./rres_packer
mv ../web/res/myresources.rres ../web/res/myresources.rres.old
cp ./myresources.rres ../web/res/
