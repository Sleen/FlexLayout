{
	"targets": [
		{
			"include_dirs": [
				"node_modules/nan"
			],
			"includes": [
				"node_modules/nbind/src/nbind.gypi"
			],
			"conditions": [
				[
					"asmjs==1",
					{
						"ldflags": [
							"--memory-init-file",
							"0",
							"-s",
							"PRECISE_F32=1",
							"-s",
							"TOTAL_MEMORY=134217728"
						],
						"xcode_settings": {
							"OTHER_LDFLAGS": [
								"<@(_ldflags)"
							]
						}
					}
				]
			],
			"sources": [
				"binding.cpp"
			]
		}
	],
	"includes": [
		"node_modules/nbind/src/nbind-common.gypi"
	]
}