ownLocation="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd ${ownLocation}/..
BUILD_DIR_ARM8=build-spectre_android-Android_Qt_5_15_1_Clang_Multi_Abi_ARMv8-Release
BUILD_DIR_ARM7=build-spectre_android-Android_Qt_5_15_1_Clang_Multi_Abi_ARMv7-Release
BUILD_DIR_x86_64=build-spectre_android-Android_Qt_5_15_1_Clang_Multi_Abi_x86_64-Release

mkdir -p ${BUILD_DIR_ARM8}/android-build/libs/x86_64
cp -f ${BUILD_DIR_x86_64}/android-build/libs/x86_64/* ${BUILD_DIR_ARM8}/android-build/libs/x86_64/
mkdir -p ${BUILD_DIR_ARM8}/android-build/libs/armeabi-v7a
cp -f ${BUILD_DIR_ARM7}/android-build/libs/armeabi-v7a/* ${BUILD_DIR_ARM8}/android-build/libs/armeabi-v7a/
cp -f scripts/android_deployment_settings.json ${BUILD_DIR_ARM8}/android_deployment_settings.json