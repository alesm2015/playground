#Simple project playground

sudo dnf install boost-devel
sudo dnf install telnet

#Doxygen specific
sudo dnf install doxygen
sudo dnf install graphviz

#Advanced code verification
sudo dnf install valgrind 
sudo dnf install cppcheck
sudo dnf install cppcheck-htmlreport



-----------------------------------------------------------------
#!/bin/bash

cppcheck --project=./build-debug-local/compile_commands.json -f --config-exclude=./build-debug-local -DPROTOBUF_VERSION=3019001 --report-progress --verbose --enable=warning --enable=style --enable=performance --enable=portability --inconclusive --force --addon=threadsafety.py --addon=findcasts.py --addon=misc.py --inline-suppr --check-level=exhaustive --xml 2> report.xml
cppcheck-htmlreport --file=report.xml --report-dir=build_report --source-dir=.


#goto exec directory
valgrind --tool=memcheck --leak-check=full --vgdb=yes --vgdb-error=0 ./traderd -c ./../../trading.cfg
#or
valgrind --tool=memcheck --leak-check=full --error-limit=no --track-origins=yes --vgdb=yes --read-inline-info=yes --read-var-info=yes --track-origins=yes --vgdb-error=0 ./traderd -c ./../../trading.cfg
#or
valgrind --tool=memcheck --leak-check=full --error-limit=no --track-origins=yes --vgdb=yes --redzone-size=128 --read-inline-info=yes --read-var-info=yes --track-origins=yes --vgdb-error=0 --num-callers=100 --log-file="dump.log" ./traderd -c ./../../trading.cfg

                "-DCMAKE_CXX_FLAGS=-pg",
                "-DCMAKE_EXE_LINKER_FLAGS=-pg",
                "-DCMAKE_SHARED_LINKER_FLAGS=-pg"