add_llvm_library( LLVMLoopTilingPass MODULE
  LoopTilingPass.cpp

  PLUGIN_TOOL
  opt
  )

