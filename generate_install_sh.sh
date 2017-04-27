#!/bin/bash


INSTALLPREFIX=`cat compileoptions.h |grep INSTALL_PREFIX|cut -d '"' -f2`


cat > install.sh << _EOF
#!/bin/bash

echo "Installing ZEsarUX under $INSTALLPREFIX ..."

mkdir -p $INSTALLPREFIX
mkdir -p $INSTALLPREFIX/bin
mkdir -p $INSTALLPREFIX/share/zesarux/

cp zesarux $INSTALLPREFIX/bin/
cp *.rom divmmcesx085.mmc zxuno.mmc tbblue.mmc divideesx085.ide atomlite.ide zxuno.flash $INSTALLPREFIX/share/zesarux/

#cp mantransfev3.bin macos_say_filter.sh $INSTALLPREFIX/share/zesarux/
cp mantransfev3.bin $INSTALLPREFIX/share/zesarux/

cp -r speech_filters $INSTALLPREFIX/share/zesarux/

cp -r media $INSTALLPREFIX/share/zesarux/
cp ACKNOWLEDGEMENTS Changelog HISTORY LICENSE LICENSE_MOTOROLA_CORE README FEATURES INSTALL INSTALLWINDOWS ALTERNATEROMS INCLUDEDTAPES FAQ $INSTALLPREFIX/share/zesarux/
find $INSTALLPREFIX/share/zesarux/ -type f -print0| xargs -0 chmod 444

#chmod +x $INSTALLPREFIX/share/zesarux/macos_say_filter.sh
chmod +x $INSTALLPREFIX/share/zesarux/speech_filters/*

echo "Install done"

_EOF


chmod 755 install.sh

