import qbs 1.0
import qbs.Environment
Project {

    CatchTest {
        Depends { name : "banshee-lib" }

        name : "tst_property"
        files:  [
            "property.cpp",
        ]
    }


    CatchTest {
        Depends { name : "banshee-lib" }

        name : "tst_json"
        files:  [
            "json.cpp",
        ]
    }

    CppApplication {
        files: [
            "validate.cpp",
        ]
        name : "validate"
        Depends { name : "banshee-lib" }
    }
}
