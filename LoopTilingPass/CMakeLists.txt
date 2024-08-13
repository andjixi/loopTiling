add_llvm_library( LoopTilingPass MODULE
  LoopTilingPass.cpp

  PLUGIN_TOOL
  opt
  )
