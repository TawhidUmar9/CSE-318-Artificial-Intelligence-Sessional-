#! /usr/bin/bash

# Corrected g++ command
g++ -O3 -march=native -flto=auto -std=c++17 -o main test.cpp
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
 
echo "Running tests..."
## Loop for depth value 2 to 4 with criterion IG IGR NWIG
for depth in {2..4}; do
    for criterion in IG IGR NWIG; do
        # Pass arguments positionally to match the C++ code
        ./main "$criterion" "$depth"
    done
done

echo "Tests finished."