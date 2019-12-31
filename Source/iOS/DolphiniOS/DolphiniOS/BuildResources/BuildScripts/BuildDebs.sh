#!/bin/bash

set -e

ROOT_SRC_DIR=$PROJECT_DIR/../
EXPORT_UUID=`uuidgen`
EXPORT_PATH="/tmp/DolphiniOS-$EXPORT_UUID"
DOLPHIN_EXPORT_PATH="$EXPORT_PATH/dolphin_deb_root/"
CONTROL_FILE=$ROOT_SRC_DIR/DolphiniOS/DolphiniOS/BuildResources/DebFiles/control.in
APPLICATION_DESTINATION_PATH=$DOLPHIN_EXPORT_PATH/Applications/DolphiniOS.app
BUILD_NUMBER=$(/usr/libexec/PlistBuddy -c "Print CFBundleVersion" "$PROJECT_DIR/DolphiniOS/Info.plist")

if [ $BUILD_FOR_PATREON == "YES" ]; then
  CONTROL_FILE=$ROOT_SRC_DIR/DolphiniOS/DolphiniOS/BuildResources/DebFiles/control-patreon.in
  APPLICATION_DESTINATION_PATH=$DOLPHIN_EXPORT_PATH/Applications/DiOSPatreon.app
fi

# As recommended by saurik, don't copy dot files
export COPYFILE_DISABLE
export COPY_EXTENDED_ATTRIBUTES_DISABLE

# Make directories
mkdir -p $DOLPHIN_EXPORT_PATH
mkdir $DOLPHIN_EXPORT_PATH/Applications
mkdir -p $DOLPHIN_EXPORT_PATH/Library/LaunchDaemons
mkdir -p $DOLPHIN_EXPORT_PATH/usr/libexec
mkdir $DOLPHIN_EXPORT_PATH/DEBIAN

exec > $EXPORT_PATH/archive.log 2>&1

# Copy resources
cp -r "$ARCHIVE_PATH/Products/Applications/DolphiniOS.app" $APPLICATION_DESTINATION_PATH
cp $ROOT_SRC_DIR/csdbgd/csdbgd $DOLPHIN_EXPORT_PATH/usr/libexec
cp $ROOT_SRC_DIR/csdbgd/me.oatmealdome.csdbgd.plist $DOLPHIN_EXPORT_PATH/Library/LaunchDaemons
cp $ROOT_SRC_DIR/DolphiniOS/DolphiniOS/BuildResources/DebFiles/preinst.sh $DOLPHIN_EXPORT_PATH/DEBIAN/preinst
cp $ROOT_SRC_DIR/DolphiniOS/DolphiniOS/BuildResources/DebFiles/postinst.sh $DOLPHIN_EXPORT_PATH/DEBIAN/postinst
cp $ROOT_SRC_DIR/DolphiniOS/DolphiniOS/BuildResources/DebFiles/prerm.sh $DOLPHIN_EXPORT_PATH/DEBIAN/prerm
cp $ROOT_SRC_DIR/DolphiniOS/DolphiniOS/BuildResources/DebFiles/postrm.sh $DOLPHIN_EXPORT_PATH/DEBIAN/postrm
chmod -R 755 $DOLPHIN_EXPORT_PATH/DEBIAN/*
sed "s/VERSION_NUMBER/$MARKETING_VERSION-$BUILD_NUMBER/g" $CONTROL_FILE > $DOLPHIN_EXPORT_PATH/DEBIAN/control

# Resign the application
codesign -f -s "OatmealDome Software" --entitlements $ROOT_SRC_DIR/DolphiniOS/DolphiniOS/BuildResources/Entitlements_JB.plist $APPLICATION_DESTINATION_PATH
codesign -f -s "OatmealDome Software" --entitlements $ROOT_SRC_DIR/DolphiniOS/DolphiniOS/BuildResources/Entitlements_JB.plist $APPLICATION_DESTINATION_PATH/Frameworks/*
codesign -f -s "OatmealDome Software" --entitlements $ROOT_SRC_DIR/csdbgd/Entitlements.plist $DOLPHIN_EXPORT_PATH/usr/libexec/csdbgd

# Remove the mobileprovision file
rm $APPLICATION_DESTINATION_PATH/embedded.mobileprovision

# Create the deb file
dpkg-deb -b $DOLPHIN_EXPORT_PATH
mv $EXPORT_PATH/dolphin_deb_root.deb $EXPORT_PATH/DolphiniOS.deb

open $EXPORT_PATH
