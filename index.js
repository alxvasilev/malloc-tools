var native;
try {
    native = require("./build/Release/malloc-tools.node");
} catch (ex) {
    native = require("./build/Debug/malloc-tools.node");
}

module.exports = native;
