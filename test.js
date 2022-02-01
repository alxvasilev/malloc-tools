var tools = require("./index.js");
var xml2js;
try {
    xml2js = require("xml2js");
} catch (ex)
{
}

async function testMalloc(malloc) {
    console.log("==== malloc stats ====");
    malloc.stats();
    console.log("=============TRIM==============");
    malloc.trim(0);
    malloc.stats();

    let info = malloc.mallinfo2();
    console.log("mallinfo:", JSON.stringify(info));

    let xml = malloc.info();
    console.log("info:", xml);
    if (xml2js) {
        let json = await xml2js.parseStringPromise(xml);
        console.log("json:", JSON.stringify(json));
        console.log("arenas:", json.malloc.heap.length);
    }
    else
    {
        console.warn("To parse the malloc_info output, please install xml2js module");
    }
}

function jeLogSizeParam(name) {
    console.log(`${name}: ${(Math.round(tools.jemalloc.ctlGetSize(name) / (1024 * 102.4))/10)}MB`);
}
function jeLogBoolParam(name) {
    console.log(`${name}: ${tools.jemalloc.ctlGetBool(name)}`);
}
function testJemalloc(jemalloc) {
    console.log("============= jemalloc stats ===============");
    console.log("version:", jemalloc.ctlGetString("version"));
    console.log(`arenas.narenas: ${jemalloc.ctlGetUnsigned("arenas.narenas")}`);
    jeLogSizeParam("stats.mapped");
    jeLogSizeParam("stats.allocated");
    jeLogSizeParam("stats.resident");
    jeLogSizeParam("stats.active");
    jeLogSizeParam("stats.metadata");
    jeLogBoolParam("config.stats");
    jeLogBoolParam("config.prof");
    jeLogBoolParam("config.debug");
    jeLogBoolParam("background_thread");
    console.log("max_background_threads:", jemalloc.ctlGetSize("max_background_threads"));
    jemalloc.ctlSetSSize("arenas.dirty_decay_ms", 100);
}

async function main() {
    console.log("==== common API ====");
    console.log("glibc version:", tools.glibcVersion);
    console.log("allocator=" + tools.allocator);
    console.log("Heap usage:", JSON.stringify(tools.getHeapUsage()));

    if (tools.allocator === "malloc") {
        await testMalloc(tools.malloc);
    } else if (tools.allocator === "jemalloc") {
        testJemalloc(tools.jemalloc);
    } else {
        console.error("Unknown allocator in use:", tools.allocator);
    }
    console.log("Done");
}
main();
