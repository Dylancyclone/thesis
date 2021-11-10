#!/usr/bin/env sh

# Save original execution location
old_path=$(pwd)

# Go to the root of the project
parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; cd .. ; pwd -P )
cd "$parent_path"

showHelp()
{
    echo
    echo '   HELP'
    echo
    echo '    ' $(basename $0) '<STAGE> <OPTION>'
    echo
    echo '     Builds the software side of the thesis project.'
    echo
    echo
    echo '   STAGES'
    echo
    echo '     1, make'
    echo '          Make the C code.'
    echo
    echo '     2, package'
    echo '          Package the application into a .pkg.tar.zst'
    echo
    echo '     3, build'
    echo '          Build bootable arm image.'
    echo
    echo
    echo '   OPTIONS'
    echo
    echo '     -h, --help'
    echo '          Displays this syntax message.'
    echo
    # echo '     -e, --exclusive'
    # echo '          Run only the stage specified'
    echo
    echo
    echo '   EXAMPLES'
    echo
    echo '    ' $(basename $0)
    echo '    ' $(basename $0) 'build'
    # echo '    ' $(basename $0) 'build -e'
    echo
    echo
}

runMake()
{
    mkdir ./moonlight/build
    cd ./moonlight/build
    cmake ../
    make
    sudo make install
    sudo ldconfig /usr/local/lib
    cd "$parent_path"
}

runPackage()
{
    cd ./pkgbuild
    tar -cf moonlight.tar ../moonlight/
    makepkg -sLfc
    rm moonlight.tar
    cd "$parent_path"
}

runBuild()
{
    pkgname=$(sed -n 's/^pkgname=//p' ./pkgbuild/PKGBUILD)
    pkgver=$(sed -n 's/^pkgver=//p' ./pkgbuild/PKGBUILD)
    pkgrel=$(sed -n 's/^pkgrel=//p' ./pkgbuild/PKGBUILD)
    # Copy local package to cache to be installed into new image
    sudo cp ./pkgbuild/$pkgname-$pkgver-$pkgrel-aarch64.pkg.tar.zst /var/cache/pacman/pkg/
    # Build image
    sudo buildarmimg -d rpi4 -e thesis -i $pkgname-$pkgver-$pkgrel-aarch64.pkg.tar.zst
    # Remove local package from cache
    sudo rm /var/cache/pacman/pkg/$pkgname-$pkgver-$pkgrel-aarch64.pkg.tar.zst -f
    # Move build image to build folder
    sudo mv /var/cache/manjaro-arm-tools/img/* ./build/
    cd "$parent_path"
}

if [ -z $1 ]
then
    echo
    echo '   Please select a stage'
    echo
    echo '    1. make'
    echo '    2. package'
    echo '    3. build'
    echo
    read -p '   Enter a number: ' stage
else
    stage=$1
fi

case $stage in
    
    "-h" | "--help" )
        showHelp
        exit 0
    ;;
    
    "1" | "make" )
        runMake
    ;;
    
    "2" | "package" )
        runPackage
    ;;
    
    "3" | "build" )
        # runPackage
        runBuild
    ;;
    
    * )
        showHelp
        exit 1
    ;;
    
esac

# Go back to the original path
cd "$old_path"

exit 0
