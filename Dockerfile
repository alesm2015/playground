# Dockerfile
FROM fedora:latest


# Install development tools
RUN dnf -y update

RUN dnf -y install gcc-c++ git cmake make gdb vim boost-devel telnet doxygen graphviz valgrind cppcheck cppcheck-htmlreport

RUN dnf clean all

WORKDIR /workspace


#CMD ["/bin/bash"]
CMD ["/bin/bash", "-c", "mkdir build && cd build && cmake -DBUILD_UNIT_TESTS=TRUE -DBUILD_STATIC_ANALYSIS=TRUE -DBUILD_DOCUMENTATION=TRUE -DCMAKE_BUILD_TYPE=Debug .. && make"]

