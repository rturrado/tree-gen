import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import copy


class TreeGenConan(ConanFile):
    name = "tree-gen"
    license = "Apache-2.0"
    url = "http://localhost:8081/artifactory/api/conan/rturradocenter"
    homepage = "https://github.com/rturrado/tree-gen"
    description = ("tree-gen is a C++ and Python code generator "
                  "for tree-like structures common in parser and compiler codebases")
    topics = ("code generation", "tree-like structures", "parser", "compiler")
    version = "0.1"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "cmake/*", "generator/*", "include/*", "src/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["tree-lib"]
