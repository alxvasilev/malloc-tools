var tools = require("./index.js");
async function main() {
    tools.malloc_stats();
    console.log("=============TRIM==============");
    tools.malloc_trim(0);
    tools.malloc_stats();

    let xml = tools.malloc_info();
    console.log("info:", xml);
    let json = await require("xml2js").parseStringPromise(xml);
    console.log("json:", JSON.stringify(json));
    console.log("arenas:", json.malloc.heap.length);
    let info = tools.mallinfo2();
    console.log("mallinfo:", JSON.stringify(info));
}
main();