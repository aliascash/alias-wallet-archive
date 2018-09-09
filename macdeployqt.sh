# This script uses macdeployqt to add the required libs to spectrecoin package.
# - Fixes non @executable openssl references.
# - Replaces openssl 1.0.0 references with 1.1

# Change paths before executing
QT_PATH=/Users/teknex/Qt/5.9.6
OPENSSL_PATH=/usr/local/Cellar/openssl@1.1/1.1.0i

$QT_PATH/clang_64/bin/macdeployqt src/bin/spectrecoin.app

echo -e "\nRemove openssl 1.0.0 libs:"
rm -v src/bin/spectrecoin.app/Contents/Frameworks/libssl.1.0.0.dylib
rm -v src/bin/spectrecoin.app/Contents/Frameworks/libcrypto.1.0.0.dylib

echo -e "\nReplace openssl 1.0.0 lib references with 1.1:"
for f in src/bin/spectrecoin.app/Contents/Frameworks/*.dylib; do 
install_name_tool -change @executable_path/../Frameworks/libssl.1.0.0.dylib @executable_path/../Frameworks/libssl.1.1.dylib $f;
install_name_tool -change @executable_path/../Frameworks/libcrypto.1.0.0.dylib @executable_path/../Frameworks/libcrypto.1.1.dylib $f;
done


echo -e "\ninstall_name_tool -change $OPENSSL_PATH/lib/libcrypto.1.1.dylib @executable_path/../Frameworks/libcrypto.1.1.dylib src/bin/spectrecoin.app/Contents/Frameworks/libssl.1.1.dylib ..."
install_name_tool -change $OPENSSL_PATH/lib/libcrypto.1.1.dylib @executable_path/../Frameworks/libcrypto.1.1.dylib src/bin/spectrecoin.app/Contents/Frameworks/libssl.1.1.dylib
otool -l src/bin/spectrecoin.app/Contents/Frameworks/libssl.1.1.dylib | grep dylib



echo -e "\nPlease check for non included lib references:"
for f in src/bin/spectrecoin.app/Contents/Frameworks/*.dylib; do otool -l $f | grep dylib | grep -v @; done


