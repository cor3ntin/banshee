import qbs

Project {
    Product {
        name : "afio"
        Export {
            Depends { name: "cpp" }
            cpp.includePaths: [
                "/home/cor3ntin/dev/afio/build/include",
                "/home/cor3ntin/dev/afio/include"
            ]
        }
    }

    StaticLibrary {
        name : "banshee-lib"
        Depends { name: "cpp" }
        Depends { name: "afio" }
        Group {
            name   :"Headers"
            prefix :"../include/"
            files  : ["**/*.h"]

            cpp.includePaths: [
                "../include/"
            ]
        }

        Export {
            Depends { name: "cpp" }
            Depends { name: "afio" }
            cpp.includePaths: [
                "../include/"
            ]
        }
    }


    Application {
        name : "banshee-test"
        Depends { name: "banshee-lib" }
        Depends { name: "cpp" }

        Group {
            name   :"Sources"
            files  : ["main.cpp"]
        }
        cpp.cxxFlags: [
            "-g3"
        ]
        cpp.driverFlags: [
            "-std=c++17",
            "-fcoroutines-ts" ,
            "-stdlib=libc++"
        ]

        Group {
            name   :"Headers"
            prefix :"../include/"
            files  : ["**/*.h"]

            cpp.includePaths: [
                "../include/"
            ]
        }

        cpp.includePaths: [
            "../include",
            "../3rdParty/include"
        ]
        cpp.cxxLanguageVersion : "c++17"

        Export {
            Depends { name: "cpp" }
            cpp.driverFlags: [
                "-std=c++17",
                "-fcoroutines-ts" ,
                "-stdlib=libc++",

            ]

            cpp.includePaths: [
                "../include",
                "../3rdParty/include"
            ]
        }
    }
}
