#! /bin/bash

targetName="Agent"
targetVer="0.0.1"
targetPackName="Agent.deb"


mv ${targetName} ${targetName}-${targetVer}
tar zcvf ${targetName}-${targetVer}.tar.gz ${targetName}-${targetVer}
rm *.deb
cd ${targetName}-${targetVer}
dpkg-buildpackage -rfakeroot
cd ..
mv *.deb ${targetPackName}
