Please refer to the following file root_dir/doc/README.md for more information.





Cppcheck
Clang-Tidy

*Conan* and *Vcpkg*,

clang-format

-----------------------------------------------------------------



#goto exec directory
valgrind --tool=memcheck --leak-check=full --vgdb=yes --vgdb-error=0 ./traderd -c ./../../trading.cfg
#or
valgrind --tool=memcheck --leak-check=full --error-limit=no --track-origins=yes --vgdb=yes --read-inline-info=yes --read-var-info=yes --track-origins=yes --vgdb-error=0 ./traderd -c ./../../trading.cfg
#or
valgrind --tool=memcheck --leak-check=full --error-limit=no --track-origins=yes --vgdb=yes --redzone-size=128 --read-inline-info=yes --read-var-info=yes --track-origins=yes --vgdb-error=0 --num-callers=100 --log-file="dump.log" ./traderd -c ./../../trading.cfg

                "-DCMAKE_CXX_FLAGS=-pg",
                "-DCMAKE_EXE_LINKER_FLAGS=-pg",
                "-DCMAKE_SHARED_LINKER_FLAGS=-pg"