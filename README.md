# LLVM Loop Tiling Optimization Pass

This LLVM pass implements **Loop Tiling**, an optimization that breaks loops into smaller tiles to improve cache performance. It modifies the loop's structure by creating new loop headers, bodies, and exit blocks, ensuring correct control flow and updating loop counters accordingly. The pass is designed to enhance the efficiency of nested loops by optimizing memory access patterns.

## Preparing the environment

To get started, you'll need to download and set up the LLVM repository. You can do this with the following commands:

```bash
wget "http://www.prevodioci.matf.bg.ac.rs/kk/2023/vezbe/llvm-project.zip"
unzip -d llvm-project llvm-project.zip
```

Inside the extracted directory, you'll find a script named `make_llvm.sh` to build the project. Follow these steps:

1. Make the script executable:
   ```bash
   chmod +x make_llvm.sh
   ```

2. Run the script to build LLVM:
   ```bash
   ./make_llvm.sh
   ```

This will compile the LLVM project.

## Adding our pass

1. Clone the repository.
2. Move the entire `LoopTilingPass` directory to `llvmproject/llvm/lib/Transforms/` directory.
   
3. Update the `llvmproject/llvm/lib/Transforms/CMakeLists.txt` and add the following line:
	```bash
   add_subdirectory(LoopTilingPass)
   ```

 
## Running the Optimization

To compile and run your code with this optimization:

1. Place the files from the `Test` directory in the `llvmproject/build/` directory.

2. From the `llvmproject/build/` directory, execute the following commands for example:
	```bash
	./bin/clang -S -emit-llvm test1.c
	./bin/opt -S -load lib/LoopTilingPass.so -enable-new-pm=0 -loop-tiling test1.ll -o -output.ll

3. The optimized code will be available in `output.ll`.


## Members:

- Bogdan Stojadinović  - [@bogdans55](https://github.com/bogdans55)
  
- Anđela Jovanović       - [@andjixi](https://github.com/andjixi)
    
