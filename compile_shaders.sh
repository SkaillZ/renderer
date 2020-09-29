for f in shaders/*; do
    if [[ $f != *".spv" ]] && [[ $f != *".glsl" ]]; then # Skip compiled files
        echo "Compiling $f"
        glslc "$f" -o "$f.spv"
    fi
done
