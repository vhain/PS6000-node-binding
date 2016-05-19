{
  "targets" : [
    {
      "target_name": "node-ps6000",
      "sources": ["main.cpp", "main_wrap.cpp"],
      "libraries": ["<(module_root_dir)/lib/ps6000.lib"],
      "cflags": [
        "-std=c++11",
        "-stdlib=libc++"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "copies": [
        {
          "destination": "<(module_root_dir)/build/Release",
          "files": [
            "<(module_root_dir)/dll/ps6000.dll",
            "<(module_root_dir)/dll/PicoIpp.dll"
          ]
        }
      ]
    }
  ]
}
