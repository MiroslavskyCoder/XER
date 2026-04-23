#!/bin/bash
DIR="src/wrapper/skia"

sed -i '1s/^/#include <cstdint>\n/' "$DIR/skia_engine_bridge_image_filter.h"
sed -i '1s/^/#include <vector>\n/' "$DIR/skia_engine_bridge_shader.h"

