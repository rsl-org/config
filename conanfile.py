from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class rslconfigRecipe(ConanFile):
    name = "rsl-config"
    version = "0.1"
    package_type = "library"

    # Optional metadata
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of mypkg package here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "coverage": [True, False],
        "examples": [True, False]
    }
    default_options = {"shared": False, "fPIC": True,
                       "coverage": False, "examples": False}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "include/*"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("rsl-util/0.1")
        if not self.conf.get("tools.build:skip_test", default=False):
            self.test_requires("rsl-test/0.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(
            variables={
                "ENABLE_COVERAGE": self.options.coverage,
                "BUILD_EXAMPLES": self.options.examples,
                "BUILD_TESTING": not self.conf.get("tools.build:skip_test", default=False)
            })
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.components["config"].set_property("cmake_target_name", "rsl::config")
        self.cpp_info.components["config"].includedirs = ["include"]
        self.cpp_info.components["config"].libdirs = ["lib"]
        self.cpp_info.components["config"].libs = ["rsl_config"]
