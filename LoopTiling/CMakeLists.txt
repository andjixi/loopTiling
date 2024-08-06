add_llvm_library( LLVMLoopTiling MODULE
  LoopTiling.cpp

  PLUGIN_TOOL
  opt
  )

