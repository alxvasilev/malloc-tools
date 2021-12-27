var tools = require("./index.js");
var xml2js;
try {
    xml2js = require("xml2js");
} catch (ex)
{
}

async function main() {
    tools.malloc_stats();
    console.log("=============TRIM==============");
    tools.malloc_trim(0);
    tools.malloc_stats();

    let info = tools.mallinfo2();
    console.log("mallinfo:", JSON.stringify(info));

    let xml = tools.malloc_info();
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
main();