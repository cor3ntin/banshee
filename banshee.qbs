import qbs

Project {
    qbsSearchPaths: ["3rdParty/catch2/qbs"]
    minimumQbsVersion: "1.7.1"

    references: [
        "3rdParty/catch2/Catch2.qbs",
        "src/src.qbs",
        "tests/tests.qbs"
    ]

    AutotestRunner { }

}
