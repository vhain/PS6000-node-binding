{
   "targets" : [
      {
         "target_name": "node-ps6000",
         "sources": ["main.cpp", "main_wrap.cpp"],
         "libraries": ["ps6000.lib"],
         "cflags": [
            "-std=c++11",
            "-stdlib=libc++"
         ],
         "include_dirs": [
             "<!(node -e \"require('nan')\")"
         ]
      }
   ]
}
