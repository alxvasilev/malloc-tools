{
	"targets": [
  	{
      		"target_name": "malloc-tools",
      		"sources": [ "malloc-tools.cpp" ],
      		"include_dirs": ["<!(node -p \"require('node-addon-api').include_dir\")"],
      		"defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS", "NODE_ADDON_API_ENABLE_MAYBE" ],
      		"conditions": [
  			["OS=='mac'", {
      				"cflags+": ["-fvisibility=hidden"],
      				"xcode_settings": {
        				"GCC_SYMBOLS_PRIVATE_EXTERN": "YES", # -fvisibility=hidden
      				}
  			}]
		]
    	}]
}
