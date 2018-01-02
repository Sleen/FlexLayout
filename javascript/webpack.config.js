module.exports = {
    entry: "./entry-browser",
    output: {
        library: "FlexLayout",
        libraryTarget: "umd",
        path: require("path").resolve(__dirname, "out"),
        filename: "FlexLayout.js"
    },
    node: {
        fs: "empty",
        module: "empty"
    },
    performance: {
        hints: false
    }
};
